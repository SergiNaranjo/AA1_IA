#ifndef ENGINE_GAME_HPP
#define ENGINE_GAME_HPP
#include <vector>
#include <string>
#include "World.hpp"
#include "Animation.hpp"
#include "../core/texture_pool.hpp"
#include "../iso/isoEngine.hpp"
#include "Army.h"

// FILE: the runtime - owns the isometric engine, texture pool and World; runs
// input -> update -> render. Only place that knows both the AI-facing World
// and the isometric renderer (worldToScreen/screenToWorld convert between them).
// EXTEND: processInput()/update() is where new input-driven test controls go
// (see cycleWeapon/triggerAttack/cycleSelectedAgent for the existing pattern).
//
// Typical usage:
//     Game game(64, 64, 64);
//     Agent a; a.position = game.world().tileCenter(32, 32);
//     game.world().agents.push_back(a);
//     game.run();

// EXTEND: new weapon variant - add it here, then to weaponSuffix() in
// Game.cpp (maps it to the sprite pack's filename suffix). Nothing else
// needs touching: ensureWeaponTextures() loads a weapon's sheets the first
// time one is actually used. Purely cosmetic (selects which sheets Animator
// plays) - a student's FSM/decision-maker eventually drives this instead
// of a key (see cycleWeapon below).
enum class Weapon { Unarmed, Sword, Bow };

class Game
{
public:
    // Summary: build an empty map (no file) of the given size, in tiles.
    // Output: a ready-to-run Game; typically push_back agents next.
    Game(int mapWidthTiles, int mapHeightTiles, int tileSize);
    // Summary: load size + tree/block/wall objects from a .map file (format
    // in MapLoader.hpp). Errors: a missing/malformed file logs a warning and
    // falls back to a 64x64 empty map - it does not throw or abort.
    Game(const std::string &mapPath, int tileSize);
    ~Game();

    World &world() { return world_; }

    Army& army() { return army_; }
    
    // Summary: the main loop - processInput/update/render every frame until
    // Esc or window-close. Blocks until then; call this last.
    void run();

    // Scroll the camera so the given world-space point is centred on screen.
    // Typically called once after placing the initial agent(s).
    void centerCameraOn(const Vector2 &worldPos);

private:
    void init(int mapWidthTiles, int mapHeightTiles, int tileSize);
    // EXTEND: new keybindings/test controls go in here (see the else-if
    // chain in Game.cpp, and cycleWeapon/triggerAttack for the pattern).
    void processInput();
    void update(float dt);
    // EXTEND: draw order here is back-to-front by depth; F1/I debug drawing
    // (targets, influence map, markers) is appended at the end - see Game.cpp.
    void render();
    void drawGround();
    void drawAgent(size_t index);
    void drawObject(size_t index);

    // Convert a world-space position to a screen-space point (isometric).
    void worldToScreen(const Vector2 &world, int &screenX, int &screenY) const;
    // Inverse of worldToScreen: the world-space point currently under a given
    // screen pixel.
    Vector2 screenToWorld(int screenX, int screenY) const;
    // The world-space point currently at the centre of the screen.
    Vector2 screenCenterWorldPos() const;
    // Set the selected agent's target to the tile under the mouse.
    void updateMouseTarget();
    // Keep WASD/edge-scroll from panning the map entirely off-screen.
    void clampCameraToMap();
    // Pure version of the maths isoEngineCenterMap uses internally: what
    // mapScroll2Dpos would centre this world point, without touching iso_.
    point2DT mapScrollForCenterOn(const Vector2 &worldPos) const;

    // Animation: keep one Animator + facing per agent, in step with world_.agents.
    // IMPORTANT INVARIANT: animators_/facing_/weapons_/attacking_ below are
    // index-aligned with world_.agents - call ensureAgentVisuals() before
    // indexing any of them with an agent index, or a newly-added agent has
    // no matching entry yet (out-of-bounds). update()/render() already do this.
    void     ensureAgentVisuals();
    Animator makeCharacterAnimator(Weapon weapon);
    void     updateAnimations(float dt);
    // Summary: load one weapon's sprite sheets into the texture pool, once.
    // Called by makeCharacterAnimator, so switching weapon (Tab) or adding an
    // agent pulls in whatever art it needs by itself.
    // WATCH OUT: loading every weapon up front was ~700 MB of GPU texture
    // memory before the first frame ever appeared - slow enough on a weak
    // integrated GPU to look like a hang, and an outright failure on a
    // machine short of memory. One weapon at a time is ~1/3 of that.
    void     ensureWeaponTextures(Weapon weapon);
    // Summary: draw a progress bar and pump the event queue mid-load, so the
    // window is visible and responsive (and Esc/Alt+F4 work) while the sprite
    // sheets stream in instead of showing a black "Not responding" window.
    // Inputs: what - short label for the log; progress - 0..1.
    void     showLoadingScreen(const std::string &what, float progress);

    // TAB: cycle the selected agent's weapon (rebuilds its Animator against
    // the new weapon's sheets). SPACE: play a one-shot attack on the
    // selected agent (no-op while unarmed - there's no Attack_Unarmed
    // sheet). Test-harness controls, standing in for what a student's
    // FSM/decision-maker will eventually trigger per-agent instead of a key.
    void cycleWeapon();
    void triggerAttack();

    // Left/Right arrows: cycle which agent is selected - Click sets that
    // agent's target, and (only visible while F1 debug mode is on, but the
    // selection itself works regardless) F1's overlay shows that agent's
    // influence map, with a marker on the selected agent. direction is +1
    // (next) or -1 (previous).
    void cycleSelectedAgent(int direction);

    TexturePool  texturePool_;
    isoEngineT  *iso_ = nullptr;
    World        world_;
    Army army_{ world_ };
    std::vector<Animator> animators_;    // one per agent (index-aligned)
    std::vector<int>      facing_;        // last facing per agent (0..15)
    std::vector<Weapon>   weapons_;       // current weapon per agent
    std::vector<bool>     attacking_;     // mid one-shot attack per agent
    std::vector<Weapon>   loadedWeapons_; // whose sheets ensureWeaponTextures has loaded
    Texture     *treeTex_    = nullptr;
    Texture     *tilesetTex_  = nullptr;   // isotiles.png, for tile-set object sprites
    bool         running_ = true;
    bool         debugMode_ = false;   // F1: steering target/line + object tile markers
    bool         showGrid_  = false;   // G: code-drawn corner-dot tile grid
    size_t       selectedAgentIndex_ = 0;   // Left/Right: which agent Click targets / F1 inspects
};

#endif // ENGINE_GAME_HPP
