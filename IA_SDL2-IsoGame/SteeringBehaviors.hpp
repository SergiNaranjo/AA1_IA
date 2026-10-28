#ifndef ENGINE_STEERINGBEHAVIORS_HPP
#define ENGINE_STEERINGBEHAVIORS_HPP
#include <algorithm>
#include <cmath>
#include <engine/Agent.hpp>
#include <engine/World.hpp>
#include <SteeringBehaviour.hpp>

namespace steering_detail {

    // (velocidad_deseada - velocidad_actual) escalada para que el maximo sea maxForce
    inline Vector2 towardsDesiredVelocity(const Agent& a, const Vector2& desired)
    {
        return ((desired - a.velocity) * (a.maxForce / a.maxSpeed)).truncated(a.maxForce);
    }

} // namespace steering_detail

class Seek : public SteeringBehavior
{
public:
    Steering GetSteering(const Agent& a, const SteeringContext&) const override
    {
        if (!a.hasTarget) return {};
        Vector2 desired = (a.target - a.position).normalized() * a.maxSpeed;
        return { steering_detail::towardsDesiredVelocity(a, desired) };
    }
};

class Arrive : public SteeringBehavior
{
public:
    explicit Arrive(float slowRadius = 160.0f, float stopRadius = 6.0f)
        : slowRadius_(slowRadius), stopRadius_(stopRadius) {
    }

    Steering GetSteering(const Agent& a, const SteeringContext&) const override
    {
        if (!a.hasTarget) return {};
        Vector2 toTarget = a.target - a.position;
        float dist = toTarget.length();
        Vector2 desired(0.0f, 0.0f);
        if (dist > stopRadius_) {
            float speed = a.maxSpeed * std::min(1.0f, dist / slowRadius_);
            desired = toTarget / dist * speed;
        }
        return { steering_detail::towardsDesiredVelocity(a, desired) };
    }

private:
    float slowRadius_;
    float stopRadius_;
};

class Separation : public SteeringBehavior
{
public:
    explicit Separation(float radius = 40.0f) : radius_(radius) {}

    Steering GetSteering(const Agent& a, const SteeringContext& ctx) const override
    {
        Vector2 sum(0.0f, 0.0f);
        for (const Agent* n : ctx.neighbors) {
            Vector2 away = a.position - n->position;
            float d = away.length();
            if (d >= radius_) continue;
            Vector2 dir = (d > 1e-4f) ? away / d : Vector2(1.0f, 0.0f);
            sum += dir * (1.0f - d / radius_);
        }
        return { (sum * a.maxForce).truncated(a.maxForce) };
    }

private:
    float radius_;
};

class Alignment : public SteeringBehavior
{
public:
    Steering GetSteering(const Agent& a, const SteeringContext& ctx) const override
    {
        if (ctx.neighbors.empty()) return {};
        Vector2 avg(0.0f, 0.0f);
        for (const Agent* n : ctx.neighbors) avg += n->velocity;
        avg = avg / (float)ctx.neighbors.size();
        return { steering_detail::towardsDesiredVelocity(a, avg) };
    }
};

class Cohesion : public SteeringBehavior
{
public:
    explicit Cohesion(float radius = 200.0f) : radius_(radius) {}

    Steering GetSteering(const Agent& a, const SteeringContext& ctx) const override
    {
        if (ctx.neighbors.empty()) return {};
        Vector2 center(0.0f, 0.0f);
        for (const Agent* n : ctx.neighbors) center += n->position;
        center = center / (float)ctx.neighbors.size();

        Vector2 toCenter = center - a.position;
        float dist = toCenter.length();
        if (dist < 1e-4f) return {};
        Vector2 desired = toCenter / dist * (a.maxSpeed * std::min(1.0f, dist / radius_));
        return { steering_detail::towardsDesiredVelocity(a, desired) };
    }

private:
    float radius_;
};

// Bigotes en la direccion (velocidad + hacia el target); si tocan un tile bloqueado,
// fuerza normal a la cara del tile + fuerza tangente para rodearlo.
class ObstacleAvoidance : public SteeringBehavior
{
public:
    explicit ObstacleAvoidance(float lookAhead = 96.0f) : lookAhead_(lookAhead) {}

    Steering GetSteering(const Agent& a, const SteeringContext& ctx) const override
    {
        if (ctx.world == nullptr) return {};
        const World& world = *ctx.world;

        Vector2 dir(0.0f, 0.0f);
        if (a.velocity.length() > 20.0f) dir += a.velocity.normalized();
        if (a.hasTarget) dir += (a.target - a.position).normalized();
        dir = dir.normalized();
        if (dir.lengthSquared() < 0.5f) return {};

        // 3 bigotes (centro, izq., dcha. a +-radius): ve lo mismo que Agent::canOccupy
        const Vector2 perp(-dir.y, dir.x);
        const float lateral[3] = { 0.0f, a.radius, -a.radius };
        const float step = std::max(8.0f, world.tileSize * 0.5f);
        for (float d = 0.0f; d <= lookAhead_; d += step) {
            for (float side_offset : lateral) {
                Vector2 p = a.position + dir * (d + a.radius) + perp * side_offset;
                int tx, ty;
                world.worldToTile(p, tx, ty);
                if (world.isWalkable(tx, ty)) continue;

                Vector2 delta = p - world.tileCenter(tx, ty);
                Vector2 normal = (std::fabs(delta.x) > std::fabs(delta.y))
                    ? Vector2(delta.x > 0.0f ? 1.0f : -1.0f, 0.0f)
                    : Vector2(0.0f, delta.y > 0.0f ? 1.0f : -1.0f);

                Vector2 tangent(-normal.y, normal.x);
                float side = tangent.x * a.velocity.x + tangent.y * a.velocity.y;
                if (std::fabs(side) < 5.0f && a.hasTarget) {
                    Vector2 toTarget = a.target - a.position;
                    side = tangent.x * toTarget.x + tangent.y * toTarget.y;
                }
                if (side < 0.0f) tangent = tangent * -1.0f;

                float strength = std::max(0.3f, 1.0f - d / lookAhead_);
                Vector2 force = normal * (a.maxForce * strength) + tangent * (a.maxForce * 0.6f);
                return { force.truncated(a.maxForce) };
            }
        }
        return {};
    }

private:
    float lookAhead_;
};

#endif // ENGINE_STEERINGBEHAVIORS_HPP