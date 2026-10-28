#ifndef ENGINE_ARMY_H
#define ENGINE_ARMY_H
#include <memory>
#include <vector>
#include "World.hpp"
#include "SteeringBehaviors.hpp"

struct ArmyConfig
{
    struct FlockWeights { float separation, alignment, cohesion; };

    float spacing = 48.0f;   // separacion entre huecos (debe ser > separationRadius)
    float separationRadius = 40.0f;
    float neighborRadius = 256.0f;

    float approachRadius = 320.0f;  // Seek -> Arrive
    float arriveSlowRadius = 160.0f;
    float arriveStopRadius = 6.0f;

    float obstacleLookAhead = 96.0f;   // evitacion suave (en el blend)
    float obstacleEmergencyLookAhead = 40.0f;   // evitacion de emergencia (prioridad 1)
    float obstacleWeight = 1.0f;

    FlockWeights marching{ 1.6f, 0.5f, 0.4f };
    FlockWeights approaching{ 1.6f, 0.2f, 0.1f };
    FlockWeights idle{ 1.6f, 0.0f, 0.0f };

    float flockingWeight = 1.0f;
    float goalWeight = 1.0f;

    float settleDistance = 20.0f;
    float settleSpeed = 20.0f;
};

class Army
{
public:
    enum class State { Idle, Marching, Approaching };

    explicit Army(World& world, int groupId = 0, ArmyConfig config = ArmyConfig());

    void addAgent(size_t worldIndex);
    void addAllAgents();
    void setObjective(const Vector2& objective);   // clic del raton
    void update(float dt);                         // antes de World::update

    State   state() const { return state_; }
    bool    hasObjective() const { return hasObjective_; }
    Vector2 objective() const { return objective_; }
    Vector2 centroid() const;
    size_t  size() const { return members_.size(); }

private:
    void setState(State s);
    void applyFlockWeights(const ArmyConfig::FlockWeights& w);
    void assignFormation();
    bool settled() const;
    Vector2 nearestFreeSlot(const Vector2& p, const std::vector<Vector2>& taken) const;

    World& world_;
    int                groupId_;
    ArmyConfig         config_;
    std::vector<size_t> members_;      // indices en world_.agents

    bool    hasObjective_ = false;
    Vector2 objective_;
    State   state_ = State::Idle;

    std::shared_ptr<CompositeBehavior> root_, blend_, flocking_, goal_;
    std::shared_ptr<Seek>              seek_;
    std::shared_ptr<Arrive>            arrive_;
    std::shared_ptr<Separation>        separation_;
    std::shared_ptr<Alignment>         alignment_;
    std::shared_ptr<Cohesion>          cohesion_;
};

#endif // ENGINE_ARMY_H

