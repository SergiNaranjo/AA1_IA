#ifndef ENGINE_STEERINGBEHAVIOR_HPP
#define ENGINE_STEERINGBEHAVIOR_HPP
#include <algorithm>
#include <memory>
#include <vector>
#include <engine/Vector2.hpp>
#include <SteeringBehaviour.hpp>

class Agent;
class World;

// Resultado de un comportamiento: fuerza lineal deseada (aceleracion, u/s^2).
struct Steering
{
    Vector2 linear;
};

// Lo que un comportamiento puede consultar ademas del propio agente.
struct SteeringContext
{
    const World* world = nullptr;
    std::vector<const Agent*> neighbors;   // companeros del mismo grupo dentro de neighborRadius
};

class SteeringBehavior
{
public:
    virtual ~SteeringBehavior() = default;
    virtual Steering GetSteering(const Agent& agent, const SteeringContext& ctx) const = 0;
};

// Composite:
//  - WeightedBlending: suma(peso_i * steering_i).
//  - PrioritizedList : devuelve el steering del primer hijo (orden de Add) con |fuerza| > epsilon.
class CompositeBehavior : public SteeringBehavior
{
public:
    enum class Mode { WeightedBlending, PrioritizedList };

    explicit CompositeBehavior(Mode mode = Mode::WeightedBlending, float priorityEpsilon = 1.0f)
        : mode_(mode), priorityEpsilon_(priorityEpsilon) {
    }

    void Add(std::shared_ptr<SteeringBehavior> b, float weight = 1.0f)
    {
        if (b) children_.push_back({ std::move(b), weight });
    }

    bool Remove(const std::shared_ptr<SteeringBehavior>& b)
    {
        auto it = findChild(b);
        if (it == children_.end()) return false;
        children_.erase(it);
        return true;
    }

    bool Contains(const std::shared_ptr<SteeringBehavior>& b) const
    {
        return findChild(b) != children_.end();
    }

    void SetWeight(const std::shared_ptr<SteeringBehavior>& b, float weight)
    {
        auto it = findChild(b);
        if (it != children_.end()) it->weight = weight;
    }

    void Clear() { children_.clear(); }
    void SetMode(Mode m) { mode_ = m; }
    Mode GetMode() const { return mode_; }
    size_t Count() const { return children_.size(); }

    Steering GetSteering(const Agent& agent, const SteeringContext& ctx) const override
    {
        Steering result;
        if (mode_ == Mode::WeightedBlending) {
            for (const Child& c : children_) {
                if (c.weight == 0.0f) continue;
                result.linear += c.behavior->GetSteering(agent, ctx).linear * c.weight;
            }
        }
        else {
            for (const Child& c : children_) {
                Steering s = c.behavior->GetSteering(agent, ctx);
                if (s.linear.length() > priorityEpsilon_) return s;
            }
        }
        return result;
    }

private:
    struct Child
    {
        std::shared_ptr<SteeringBehavior> behavior;
        float weight;
    };

    std::vector<Child>::iterator findChild(const std::shared_ptr<SteeringBehavior>& b)
    {
        return std::find_if(children_.begin(), children_.end(),
            [&](const Child& c) { return c.behavior == b; });
    }
    std::vector<Child>::const_iterator findChild(const std::shared_ptr<SteeringBehavior>& b) const
    {
        return std::find_if(children_.begin(), children_.end(),
            [&](const Child& c) { return c.behavior == b; });
    }

    Mode               mode_;
    float              priorityEpsilon_;
    std::vector<Child> children_;
};

#endif // ENGINE_STEERINGBEHAVIOR_HPP