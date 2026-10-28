// FILE: Agent's movement (kinematics) + its built-in seek steering (implementation).
// FITS IN: called every frame via World::update -> Agent::update; the only
// AI decision made today is "seek target" - see update()/seek() below.
// EXTEND: this is where your own steering behaviours plug in.
#include "Agent.hpp"
#include "World.hpp"
#include <algorithm>
#include <SteeringBehaviors.hpp>

namespace {

// Summary: whether a body of `radius`, centred at `center`, is clear of
// blocked tiles. Inputs: world (read-only), center/radius in world units.
// Output: bool; no side effects. Errors: none.
// WATCH OUT: checked at 4 edge points, not the whole circle, so the agent
// can't corner-cut through a blocked tile's edge on a diagonal move - don't
// reduce these points without re-checking that guarantee still holds.
bool canOccupy(const World &world, const Vector2 &center, float radius)
{
    const Vector2 offsets[4] = {
        Vector2(radius, 0.0f), Vector2(-radius, 0.0f),
        Vector2(0.0f, radius), Vector2(0.0f, -radius),
    };
    for (const Vector2 &off : offsets) {
        Vector2 p = center + off;
        int tx, ty;
        world.worldToTile(p, tx, ty);
        if (!world.isWalkable(tx, ty)) {
            return false;
        }
    }
    return true;
}

} // namespace

Vector2 Agent::seek(const Vector2 &seekTarget) const
{
    // Desired velocity: full speed straight at the target.
    Vector2 desired = (seekTarget - position).normalized() * maxSpeed;
    // Steering force = desired velocity minus current velocity, clamped.
    return (desired - velocity).truncated(maxForce);
}

void Agent::think(const World& world)
{
    hasPendingSteering_ = false;
    if (behavior == nullptr) {
        return;
    }
    SteeringContext ctx;
    ctx.world = &world;
    if (groupId >= 0) {
        for (const Agent& other : world.agents) {
            if (&other == this || other.groupId != groupId) continue;
            if (distance(position, other.position) <= neighborRadius) {
                ctx.neighbors.push_back(&other);
            }
        }
    }
    pendingSteering_ = behavior->GetSteering(*this, ctx).linear;
    hasPendingSteering_ = true;
}

void Agent::update(float dt, const World& world)
{
    Vector2 steering;
    if (hasPendingSteering_) {
        steering = pendingSteering_.truncated(maxForce);
        hasPendingSteering_ = false;
        // Parada limpia: casi quieto y sin fuerza apreciable -> quieto del todo.
        if (velocity.length() < 3.0f && steering.length() < 40.0f) {
            velocity = Vector2(0.0f, 0.0f);
            return;
        }
    }
    else {
        // Comportamiento por defecto del framework: seek al target.
        Vector2 seekTarget = hasTarget ? target : position;
        const float arriveRadius = 8.0f;
        if (distance(position, seekTarget) <= arriveRadius) {
            velocity = Vector2(0.0f, 0.0f);
            return;
        }
        steering = seek(seekTarget);
    }

    // Integrate: acceleration -> velocity (frame-rate independent).
    velocity = (velocity + steering * dt).truncated(maxSpeed);

    // Move axis-by-axis and reject a component that would walk into a
    // blocked tile (trees, blocks, walls, ...). Doing this per-axis instead
    // of as one combined move lets the agent slide along an obstacle instead
    // of stopping dead when only one component of the motion is blocked.
    Vector2 delta = velocity * dt;
    Vector2 candidate = position;

    candidate.x += delta.x;
    if (canOccupy(world, candidate, radius)) {
        position.x = candidate.x;
    } else {
        velocity.x = 0.0f;
    }

    candidate = position;
    candidate.y += delta.y;
    if (canOccupy(world, candidate, radius)) {
        position.y = candidate.y;
    } else {
        velocity.y = 0.0f;
    }

    // Keep the facing angle in sync with movement (used for drawing later).
    // 1e-4f: "is velocity basically zero" guard, so atan2 doesn't get called
    // on (~0,~0) and isn't the same 1e-6f used in Vector2 (different purpose:
    // this is a squared-length "moving at all" test, not a divide-by-zero one).
    if (velocity.lengthSquared() > 1e-4f) {
        heading = std::atan2(velocity.y, velocity.x);
    }
}

// See Agent.hpp for the full contract (initInfluence/setInfluence/influenceAt/
// clearInfluence) - implementation notes only below.

void Agent::initInfluence(const World &world)
{
    influenceWidth_  = world.widthTiles;
    influenceHeight_ = world.heightTiles;
    influence_.assign((size_t)influenceWidth_ * influenceHeight_, 0.0f);
}

void Agent::setInfluence(int tileX, int tileY, float value)
{
    if (tileX < 0 || tileY < 0 || tileX >= influenceWidth_ || tileY >= influenceHeight_) {
        return;
    }
    influence_[(size_t)tileY * influenceWidth_ + tileX] = value;
}

float Agent::influenceAt(int tileX, int tileY) const
{
    if (tileX < 0 || tileY < 0 || tileX >= influenceWidth_ || tileY >= influenceHeight_) {
        return 0.0f;
    }
    return influence_[(size_t)tileY * influenceWidth_ + tileX];
}

void Agent::clearInfluence()
{
    std::fill(influence_.begin(), influence_.end(), 0.0f);
}
