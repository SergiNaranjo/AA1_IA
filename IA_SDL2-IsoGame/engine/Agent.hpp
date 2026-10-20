#ifndef ENGINE_AGENT_HPP
#define ENGINE_AGENT_HPP
#include <vector>
#include "Vector2.hpp"

class World; // forward declaration (Agent.cpp includes World.hpp)

// FILE: a moving agent - the thing students' AI actually drives.
// FITS IN: pure kinematics/AI, no SDL/rendering/isometric maths (Game draws it).
// EXTEND: Agent::update below is where steering happens - for now it always
// seeks the agent's own target; this is where your own behaviours
// (flee/arrive/wander/path-follow) go.
class Agent
{
public:
    Vector2 position;                 // world-space position
    Vector2 velocity;                 // world-space velocity (units / second)
    // WATCH OUT: tuned relative to World::tileSize (currently 64 units/tile).
    // Keep these proportional to tileSize so behaviour (tiles/second, tiles
    // of radius) stays the same if the tile size ever changes again.
    float   maxSpeed = 400.0f;        // maximum speed (units / second)
    float   maxForce = 2800.0f;       // maximum steering acceleration (units / second^2)
    float   radius   = 16.0f;         // body radius, for later collision / arrival
    float   heading  = 0.0f;          // facing angle in radians (derived from velocity)

    // Current steering target in world-space, and whether one is set. Each
    // agent's own - not shared - so e.g. Game can point different agents at
    // different tiles (see Game::updateMouseTarget, which sets the selected
    // agent's target on click).
    Vector2 target;
    bool    hasTarget = false;

    // Summary: advance the agent by dt seconds (the per-frame AI tick).
    // Inputs: dt = seconds since last call; world = read-only tile grid/obstacles.
    // Side effects: mutates position/velocity/heading in place. Errors: none -
    // an agent with no target just holds still.
    // EXTEND: always seeks the target for now (see seek() below); this is
    // where your steering and decision-making (FSM, decision tree) code goes.
    void update(float dt, const World &world);

    // --- Influence map (per-agent scalar field over the tile grid) --------
    // Summary: a per-agent per-tile float layer for AI code to read/write:
    // influence, a cost field, a visited-heatmap, whatever an exercise needs.
    // IMPORTANT INVARIANT: call initInfluence() once (sized to World's tile
    // grid) before set/at - skip it and the map silently stays 0x0, so
    // setInfluence/influenceAt just no-op/return 0 instead of crashing.
    // EXTEND: add more layers the same way (another vector + width/height)
    // if an exercise needs, say, a cost field alongside a visited-heatmap.
    void  initInfluence(const World &world);
    void  setInfluence(int tileX, int tileY, float value);
    float influenceAt(int tileX, int tileY) const;   // 0 if out of bounds or uninitialised
    void  clearInfluence();

private:
    // Summary: Reynolds "seek" - steering force toward moving at full speed
    // straight at seekTarget. Output: a force (acceleration); no side effects.
    // Errors: none (normalized() below handles seekTarget == position safely).
    // EXTEND: the only steering behaviour provided - add flee/arrive/wander
    // alongside it (or replace it).
    Vector2 seek(const Vector2 &seekTarget) const;

    int                influenceWidth_  = 0;
    int                influenceHeight_ = 0;
    std::vector<float> influence_;
};

#endif // ENGINE_AGENT_HPP



/*

Creo que INeighbourProvider debe estar en cada agente o en un supermanejador de agentes
¿Qué hace framework CollisionAvoidance?

La escena tiene un puñado de personajes que pueden formar un grupo o pueden ir por separado, no tiene un ejercito ya hecho
    (no hay lider)
¿Hay que trasladar variables del Agent al AgentManager?
Habrá que instanciar AgentManagers ya que pueden haber múltiples grupos en la escena...

*/