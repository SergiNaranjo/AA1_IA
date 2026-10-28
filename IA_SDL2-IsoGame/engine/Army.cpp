#include "Army.h"
#include <algorithm>
#include <cmath>

Army::Army(World& world, int groupId, ArmyConfig config)
    : world_(world), groupId_(groupId), config_(config)
{
    seek_ = std::make_shared<Seek>();
    arrive_ = std::make_shared<Arrive>(config_.arriveSlowRadius, config_.arriveStopRadius);
    separation_ = std::make_shared<Separation>(config_.separationRadius);
    alignment_ = std::make_shared<Alignment>();
    cohesion_ = std::make_shared<Cohesion>(config_.neighborRadius);

    flocking_ = std::make_shared<CompositeBehavior>(CompositeBehavior::Mode::WeightedBlending);
    flocking_->Add(separation_);
    flocking_->Add(alignment_);
    flocking_->Add(cohesion_);

    goal_ = std::make_shared<CompositeBehavior>(CompositeBehavior::Mode::WeightedBlending);
    goal_->Add(seek_);

    blend_ = std::make_shared<CompositeBehavior>(CompositeBehavior::Mode::WeightedBlending);
    blend_->Add(flocking_, config_.flockingWeight);
    blend_->Add(goal_, config_.goalWeight);
    blend_->Add(std::make_shared<ObstacleAvoidance>(config_.obstacleLookAhead), config_.obstacleWeight);

    root_ = std::make_shared<CompositeBehavior>(CompositeBehavior::Mode::PrioritizedList);
    root_->Add(std::make_shared<ObstacleAvoidance>(config_.obstacleEmergencyLookAhead));   // 1a prioridad
    root_->Add(blend_);                                                                    // 2a prioridad

    setState(State::Idle);
}

void Army::addAgent(size_t worldIndex)
{
    if (worldIndex >= world_.agents.size()) return;
    if (std::find(members_.begin(), members_.end(), worldIndex) != members_.end()) return;
    members_.push_back(worldIndex);
    Agent& a = world_.agents[worldIndex];
    a.behavior = root_.get();
    a.groupId = groupId_;
    a.neighborRadius = config_.neighborRadius;
}

void Army::addAllAgents()
{
    for (size_t i = 0; i < world_.agents.size(); ++i) addAgent(i);
}

Vector2 Army::centroid() const
{
    Vector2 c(0.0f, 0.0f);
    if (members_.empty()) return c;
    for (size_t idx : members_) c += world_.agents[idx].position;
    return c / (float)members_.size();
}

void Army::setObjective(const Vector2& objective)
{
    if (members_.empty()) return;
    objective_ = objective;
    hasObjective_ = true;
    assignFormation();
    setState(distance(centroid(), objective_) < config_.approachRadius ? State::Approaching
        : State::Marching);
}

void Army::update(float /*dt*/)
{
    if (members_.empty() || !hasObjective_) return;

    if (state_ == State::Marching && distance(centroid(), objective_) < config_.approachRadius) {
        assignFormation();               // reasignar huecos con las posiciones actuales
        setState(State::Approaching);    // Seek -> Arrive
    }
    else if (state_ == State::Approaching && settled()) {
        setState(State::Idle);           // formacion completa: solo Separation
    }
}

bool Army::settled() const
{
    for (size_t idx : members_) {
        const Agent& a = world_.agents[idx];
        if (distance(a.position, a.target) > config_.settleDistance) return false;
        if (a.velocity.length() > config_.settleSpeed) return false;
    }
    return true;
}

void Army::setState(State s)
{
    state_ = s;
    switch (s) {
    case State::Marching:
        goal_->Remove(arrive_);
        if (!goal_->Contains(seek_)) goal_->Add(seek_);
        applyFlockWeights(config_.marching);
        break;
    case State::Approaching:
        goal_->Remove(seek_);
        if (!goal_->Contains(arrive_)) goal_->Add(arrive_);
        applyFlockWeights(config_.approaching);
        break;
    case State::Idle:
        goal_->Remove(seek_);
        if (!goal_->Contains(arrive_)) goal_->Add(arrive_);
        applyFlockWeights(config_.idle);
        break;
    }
}

void Army::applyFlockWeights(const ArmyConfig::FlockWeights& w)
{
    flocking_->SetWeight(separation_, w.separation);
    flocking_->SetWeight(alignment_, w.alignment);
    flocking_->SetWeight(cohesion_, w.cohesion);
}

// Bloque filas x columnas orientado hacia la marcha, fila 0 delante.
// Cada soldado va al hueco que le toca segun su posicion para que no se crucen.
void Army::assignFormation()
{
    const size_t n = members_.size();
    Vector2 c = centroid();
    Vector2 forward = (objective_ - c).normalized();
    if (forward.lengthSquared() < 0.5f) forward = Vector2(0.0f, 1.0f);
    Vector2 right(-forward.y, forward.x);

    const int cols = (int)std::ceil(std::sqrt((float)n));
    const int rows = (int)std::ceil((float)n / (float)cols);

    std::vector<size_t> order(members_);
    auto fwdOf = [&](size_t idx) {
        Vector2 d = world_.agents[idx].position - c;
        return d.x * forward.x + d.y * forward.y;
        };
    auto latOf = [&](size_t idx) {
        Vector2 d = world_.agents[idx].position - c;
        return d.x * right.x + d.y * right.y;
        };
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return fwdOf(a) > fwdOf(b); });

    std::vector<Vector2> taken;
    size_t next = 0;
    for (int row = 0; row < rows; ++row) {
        int inRow = std::min<int>(cols, (int)(n - next));
        std::sort(order.begin() + next, order.begin() + next + inRow,
            [&](size_t a, size_t b) { return latOf(a) < latOf(b); });
        float fwd = ((rows - 1) * 0.5f - (float)row) * config_.spacing;
        for (int col = 0; col < inRow; ++col) {
            float lat = ((float)col - (inRow - 1) * 0.5f) * config_.spacing;
            Vector2 slot = nearestFreeSlot(objective_ + forward * fwd + right * lat, taken);
            taken.push_back(slot);
            Agent& a = world_.agents[order[next + col]];
            a.target = slot;
            a.hasTarget = true;
        }
        next += inRow;
    }
}

// Hueco valido: cabe el cuerpo (4 puntos de borde, como Agent::canOccupy) y no pisa otro hueco.
Vector2 Army::nearestFreeSlot(const Vector2& p, const std::vector<Vector2>& taken) const
{
    const float r = world_.agents.empty() ? 16.0f : world_.agents[members_.front()].radius;
    auto ok = [&](const Vector2& q) {
        const Vector2 probes[4] = { Vector2(r, 0.0f), Vector2(-r, 0.0f), Vector2(0.0f, r), Vector2(0.0f, -r) };
        for (const Vector2& o : probes) {
            int tx, ty;
            world_.worldToTile(q + o, tx, ty);
            if (!world_.isWalkable(tx, ty)) return false;
        }
        for (const Vector2& other : taken) {
            if (distance(q, other) < config_.spacing * 0.9f) return false;
        }
        return true;
        };
    if (ok(p)) return p;

    int tx0, ty0;
    world_.worldToTile(p, tx0, ty0);
    Vector2 best = p;
    float bestD = 1e30f;
    for (int dy = -6; dy <= 6; ++dy) {
        for (int dx = -6; dx <= 6; ++dx) {
            Vector2 c = world_.tileCenter(tx0 + dx, ty0 + dy);
            float d = distance(c, p);
            if (d < bestD && world_.isWalkable(tx0 + dx, ty0 + dy) && ok(c)) {
                best = c;
                bestD = d;
            }
        }
    }
    return best;
}