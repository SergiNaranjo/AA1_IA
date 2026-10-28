// FILE: Game implementation - main loop, rendering, input, animation wiring.
// FITS IN: see Game.hpp for the class contract; this file is the "how".
// EXTEND: see the EXTEND-tagged spots below - processInput's key chain,
// drawDecal's switch, weaponSuffix's switch, and drawObject's ObjectType
// switch are the most common places to hook in new behaviour.
#include "Game.hpp"
#include "DebugDraw.hpp"
#include "MapLoader.hpp"
#include "../core/renderer.hpp"
#include "../core/texture.hpp"
#include "../core/deltatimer.hpp"
#include "../core/logger.hpp"
#include <SDL2/SDL.h>
#include <string>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <algorithm>

namespace {

// Cheap integer hash (no state, no lookup table) used to derive per-tile
// ground variation deterministically from tile coordinates alone - the same
// tile always looks the same, with nothing stored per-tile.
uint32_t hashTile(int x, int y, uint32_t seed)
{
    uint32_t h = (uint32_t)(x * 374761393) ^ (uint32_t)(y * 668265263) ^ seed;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// Ground is a single flat colour, code-drawn (no texture): a dark olive,
// with the grid dots a clearly lighter step of the same hue so they stay
// legible against the ground's own per-tile jitter.
const SDL_Color kGroundBase = { 0x48, 0x53, 0x3F, 255 };
const SDL_Color kGridColor  = { 0x6B, 0x7C, 0x5E, 255 };

Uint8 clampByte(int v) { return (Uint8)std::max(0, std::min(255, v)); }

// One asset's worth of flatness fixed by maths: +/-3 luminance per tile so
// the ground isn't a single dead-flat colour, with zero extra art.
SDL_Color jitteredGroundColor(int tx, int ty)
{
    int jitter = (int)(hashTile(tx, ty, 0) % 7) - 3;   // -3..+3
    return SDL_Color{ clampByte(kGroundBase.r + jitter),
                       clampByte(kGroundBase.g + jitter),
                       clampByte(kGroundBase.b + jitter), 255 };
}

// A handful of decals (cracks / worn patches) scattered across the map by
// the same per-tile hash - variety without a second art asset.
// `decalModulus` is chosen so roughly 8 tiles map-wide get one, regardless
// of map size.
void drawDecal(int tx, int ty, int sx, int sy, float halfW, float halfH, int decalModulus)
{
    if (hashTile(tx, ty, 0xD3CA1u) % decalModulus != 0) {
        return;
    }
    SDL_Renderer *r = Renderer::Instance()->getRenderer();
    int kind = (int)(hashTile(tx, ty, 0xF00Du) % 2);
    // EXTEND: add a new decal kind here (bump the % 2 above to match the new
    // case count) - each case just needs to draw around (sx, sy) at a scale
    // relative to halfW/halfH so it stays proportional to the tile size.
    switch (kind) {
        case 0: {   // crack: a short dark zigzag
            SDL_SetRenderDrawColor(r, 0x2E, 0x36, 0x28, 255);
            int x0 = sx - (int)(halfW * 0.3f), y0 = sy;
            int x1 = sx,                       y1 = sy - (int)(halfH * 0.25f);
            int x2 = sx + (int)(halfW * 0.3f), y2 = sy + (int)(halfH * 0.15f);
            SDL_RenderDrawLine(r, x0, y0, x1, y1);
            SDL_RenderDrawLine(r, x1, y1, x2, y2);
            break;
        }
        default: {   // worn patch: a small pale diamond
            SDL_Color patch = { 0x5F, 0x6B, 0x50, 255 };
            DebugDraw::filledDiamond(sx, sy, (int)(halfW * 0.4f), (int)(halfH * 0.4f), patch);
            break;
        }
    }
}

// A single grid corner dot, faded out with distance from the cursor so the
// grid reads as a subtle cursor-local aid rather than a permanent overlay.
void drawGridDot(int sx, int sy, int mouseX, int mouseY)
{
    float dx = (float)(sx - mouseX);
    float dy = (float)(sy - mouseY);
    float dist = std::sqrt(dx * dx + dy * dy);
    const float fadeRadius = 700.0f;
    float t = 1.0f - std::min(dist / fadeRadius, 1.0f);   // 1 near cursor, 0 far away
    Uint8 alpha = (Uint8)(t * 255.0f);
    if (alpha == 0) {
        return;
    }
    SDL_Renderer *r = Renderer::Instance()->getRenderer();
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, kGridColor.r, kGridColor.g, kGridColor.b, alpha);
    SDL_Rect dot = { sx - 2, sy - 2, 4, 4 };
    SDL_RenderFillRect(r, &dot);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

// Character art: the "FREE Animated Isometric Hero" pack (data/x180p_Spritesheets).
// 16 facing directions at a 22.5-degree step, each direction its own pair of
// files - a Body sheet and a separate Shadow sheet sharing the same frame
// grid - per animation set. Frames are packed in a fixed-size grid (not a
// single row): 180x180 cells, row-major, however many rows/cols the sheet
// holds for that animation.
//
// Only Idle, WalkForward and Attack are wired up; Death/Jump/Run/Draw/Hit/
// Strafe/WalkBack are also in data/ if you want to add them. The facing index (0..15) doubles as the angle
// suffix via angleSuffix() below, and matches the pack's own convention:
// 0 = up/north, going clockwise.
const int   kNumDirections = 16;
const int   kSpriteFrameSize = 180;
const float kMovingSpeed = 5.0f;   // below this, the agent is "idle"

// Weapon variant -> the pack's filename suffix ("Idle_<suffix>", ...).
// There is no Attack_Unarmed sheet, so Unarmed has no attack animation.
// EXTEND: adding a Weapon (Game.hpp) means a case here (must match a real
// data/x180p_Spritesheets/<Set>_<Suffix> folder) and adding it to the
// preload loop in Game::init below.
const char *weaponSuffix(Weapon w)
{
    switch (w) {
        case Weapon::Sword: return "Sword";
        case Weapon::Bow:   return "Bow";
        default:            return "Unarmed";
    }
}
bool weaponHasAttack(Weapon w) { return w != Weapon::Unarmed; }

// Sprite anchor within a 180x180 frame: where the character's feet/shadow
// touch the ground. Measured from the Shadow layer's ground-contact point,
// which sits at a fixed pixel offset for every frame of a given animation
// (idle) and only wobbles a few px with the baked walk-cycle gait - stable
// enough for a single constant.
const float kSpriteAnchorX = 90.0f;   // horizontal centre of the 180px-wide cell
const float kSpriteAnchorY = 142.0f;  // ground line, ~38px up from the cell's bottom edge

// "000".."337": the pack's angle suffix for facing index 0..15.
std::string angleSuffix(int dir)
{
    char buf[4];
    std::snprintf(buf, sizeof(buf), "%03d", (int)(dir * (360.0f / kNumDirections)));
    return std::string(buf);
}

// Slice a sheet into its frame grid: fixed-size cells, row-major.
std::vector<SDL_Rect> gridFrames(const Texture *sheet, int frameSize)
{
    std::vector<SDL_Rect> frames;
    if (sheet == nullptr) {
        return frames;
    }
    int cols = sheet->width  / frameSize;
    int rows = sheet->height / frameSize;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            SDL_Rect r;
            setupRect(&r, col * frameSize, row * frameSize, frameSize, frameSize);
            frames.push_back(r);
        }
    }
    return frames;
}

// Map a screen-space direction vector to one of the kNumDirections facing
// codes (0 = up/north, clockwise), matching the sprite pack's own angles.
int dirFromScreenVec(float sx, float sy)
{
    // Screen space has +x right and +y down. Up is screen direction (0,-1).
    float deg = std::atan2(sy, sx) * 180.0f / 3.14159265f; // 0 = right
    float shifted = deg + 90.0f;                           // Up -> 0
    if (shifted < 0.0f) {
        shifted += 360.0f;
    }
    float step = 360.0f / kNumDirections;
    return (int)((shifted + step * 0.5f) / step) % kNumDirections;
}

} // namespace

Game::Game(int mapWidthTiles, int mapHeightTiles, int tileSize)
{
    init(mapWidthTiles, mapHeightTiles, tileSize);
}

Game::Game(const std::string &mapPath, int tileSize)
{
    MapData map;
    loadMapFile(mapPath, map);   // on failure: defaults (64x64, no objects)
    init(map.width, map.height, tileSize);
    for (const MapObjectSpec &spec : map.objects) {
        world_.addObject(spec.type, spec.tileX, spec.tileY, spec.tileIndex);
    }
}

void Game::init(int mapWidthTiles, int mapHeightTiles, int tileSize)
{
    // Isometric engine + tile-set-only isoMap (the actual tile grid is World's).
    iso_ = isoEngineNewIsoEngine();
    iso_->isoMap = isoMapCreateEmptyMap(tileSize);

    // Tiles. The tile set doubles as the sprite source for one-tile objects
    // (cube, fences, ...) placed via 'block'/'tile' map directives.
    texturePool_.AddTexture("data/isotiles.png");
    tilesetTex_ = texturePool_.GetTexture("isotiles.png");
    isoMapLoadTileSet(iso_->isoMap, tilesetTex_, 64, 80);

    // Character sprite sheets for the weapon agents start with. The other
    // weapons' sheets load the first time something asks for them (Tab, or a
    // new agent) - see ensureWeaponTextures, and the WATCH OUT on its
    // declaration in Game.hpp for why they aren't all loaded here.
    ensureWeaponTextures(Weapon::Unarmed);

    // Tall billboard object sprite (trees).
    texturePool_.AddTexture("data/isotree.png");
    treeTex_ = texturePool_.GetTexture("isotree.png");

    isoEngineSetGameMode(iso_, GAME_MODE_OVERVIEW);

    // WATCH OUT: isoMapCreateEmptyMap halves tileSize internally, so
    // iso_->isoMap->tileSize is NOT the value passed into this function -
    // World must read the post-halving value back out, not the parameter.
    world_ = World(mapWidthTiles, mapHeightTiles, iso_->isoMap->tileSize);

    // Start with the mouse away from the screen edges, otherwise wherever the
    // OS cursor happened to be when the fullscreen window opened can trigger
    // edge-scroll immediately and the view drifts before you've touched anything.
    SDL_WarpMouseInWindow(Renderer::Instance()->getWindow(), WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

    DeltaTimer::Instance()->deltaTimerInit();
}

Game::~Game()
{
    if (iso_ != nullptr) {
        isoEngineFreeIsoEngine(iso_);
    }
}

void Game::showLoadingScreen(const std::string &what, float progress)
{
    SDL_Renderer *r = Renderer::Instance()->getRenderer();

    // Pump the event queue. Without this, Windows sees a program that hasn't
    // touched its message queue in seconds, greys the window out and labels
    // it "Not responding" - which is exactly what a slow asset load used to
    // look like: a black window that ignored Esc and couldn't be closed.
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running_ = false;   // run() then returns immediately
        }
        else if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) &&
                 event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            running_ = false;
        }
    }

    SDL_SetRenderDrawColor(r, 0x1e, 0x1e, 0x1e, 0xFF);
    SDL_RenderClear(r);

    // A bar across the middle of whatever size the window turned out to be.
    float clamped = std::max(0.0f, std::min(progress, 1.0f));
    int barW = (int)(WINDOW_WIDTH * 0.5f);
    int barH = 18;
    int barX = (WINDOW_WIDTH - barW) / 2;
    int barY = (WINDOW_HEIGHT - barH) / 2;

    SDL_Rect frame = { barX - 2, barY - 2, barW + 4, barH + 4 };
    SDL_SetRenderDrawColor(r, 0x6B, 0x7C, 0x5E, 0xFF);
    SDL_RenderDrawRect(r, &frame);

    SDL_Rect fill = { barX, barY, (int)(barW * clamped), barH };
    SDL_SetRenderDrawColor(r, 0x8F, 0xB3, 0x6E, 0xFF);
    SDL_RenderFillRect(r, &fill);

    SDL_RenderPresent(r);

    (void)what;   // label is for the log line in ensureWeaponTextures
}

void Game::ensureWeaponTextures(Weapon weapon)
{
    // Already loaded? Nothing to do - this is called on every animator
    // rebuild, not just the first one.
    for (Weapon loaded : loadedWeapons_) {
        if (loaded == weapon) {
            return;
        }
    }
    loadedWeapons_.push_back(weapon);

    // Idle + WalkForward (+ Attack where the weapon has one), Body + Shadow,
    // all 16 facing directions:
    // data/x180p_Spritesheets/<Set>_<Weapon>/<Set>_<Weapon>_<Layer>_<angle>.png
    std::vector<std::string> sets = { std::string("Idle_") + weaponSuffix(weapon),
                                      std::string("WalkForward_") + weaponSuffix(weapon) };
    if (weaponHasAttack(weapon)) {
        sets.push_back(std::string("Attack_") + weaponSuffix(weapon));
    }

    std::string label = std::string("Loading ") + weaponSuffix(weapon) + " sprites";
    Logger::Instance()->writeToLog(label + "...", "runlog.txt");

    int total = (int)sets.size() * kNumDirections * 2;
    int done  = 0;
    for (const std::string &set : sets) {
        std::string dir = "data/x180p_Spritesheets/" + set + "/";
        for (int d = 0; d < kNumDirections; ++d) {
            std::string angle = angleSuffix(d);
            texturePool_.AddTexture(dir + set + "_Body_"   + angle + ".png");
            texturePool_.AddTexture(dir + set + "_Shadow_" + angle + ".png");
            done += 2;
            // Every 8th sheet: redraw the bar and let the OS see us alive.
            // Not every sheet - with vsync on, each present costs a frame.
            if (done % 8 == 0) {
                showLoadingScreen(label, (float)done / (float)total);
            }
        }
    }
}

Animator Game::makeCharacterAnimator(Weapon weapon)
{
    // Build a walk + idle + (if the weapon has one) attack animation for each
    // of the 16 directions. Each animation's frame grid is read straight off
    // its sheet's pixel size (gridFrames), so per-animation frame counts (16
    // for idle, 24 for walk/attack) just work without hardcoding them here.
    // EXTEND: Death/Jump/Run/Draw/Hit/Strafe/WalkBack - add more sets here
    // once the engine has a reason to play them, then select them in
    // updateAnimations (below) the same way "walk"/"idle"/"attack" are.
    // The sheets this animator is about to look up may not be in the pool
    // yet (only the starting weapon's are preloaded) - pull them in first.
    ensureWeaponTextures(weapon);

    Animator anim;
    std::string idleSet   = std::string("Idle_")        + weaponSuffix(weapon);
    std::string walkSet   = std::string("WalkForward_") + weaponSuffix(weapon);
    std::string attackSet = std::string("Attack_")      + weaponSuffix(weapon);

    for (int d = 0; d < kNumDirections; ++d) {
        std::string angle = angleSuffix(d);

        Texture *walkBody   = texturePool_.GetTexture(walkSet + "_Body_"   + angle + ".png");
        Texture *walkShadow = texturePool_.GetTexture(walkSet + "_Shadow_" + angle + ".png");
        if (walkBody != nullptr) {
            Animation walk;
            walk.sheet       = walkBody;
            walk.shadowSheet = walkShadow;
            walk.frameSeconds = 0.03f;
            walk.loop = true;
            walk.frames = gridFrames(walkBody, kSpriteFrameSize);
            anim.add("walk" + std::to_string(d), walk);
        }

        Texture *idleBody   = texturePool_.GetTexture(idleSet + "_Body_"   + angle + ".png");
        Texture *idleShadow = texturePool_.GetTexture(idleSet + "_Shadow_" + angle + ".png");
        if (idleBody != nullptr) {
            Animation idle;
            idle.sheet       = idleBody;
            idle.shadowSheet = idleShadow;
            idle.frameSeconds = 0.08f;
            idle.loop = true;
            idle.frames = gridFrames(idleBody, kSpriteFrameSize);
            anim.add("idle" + std::to_string(d), idle);
        }

        if (weaponHasAttack(weapon)) {
            Texture *attackBody   = texturePool_.GetTexture(attackSet + "_Body_"   + angle + ".png");
            Texture *attackShadow = texturePool_.GetTexture(attackSet + "_Shadow_" + angle + ".png");
            if (attackBody != nullptr) {
                Animation attack;
                attack.sheet       = attackBody;
                attack.shadowSheet = attackShadow;
                attack.frameSeconds = 0.025f;
                attack.loop = false;   // one-shot: play() resets to frame 0, then holds the last frame
                attack.frames = gridFrames(attackBody, kSpriteFrameSize);
                anim.add("attack" + std::to_string(d), attack);
            }
        }
    }
    return anim;
}

void Game::ensureAgentVisuals()
{
    // Keep one animator + facing + weapon per agent, created lazily as agents appear.
    while (animators_.size() < world_.agents.size()) {
        animators_.push_back(makeCharacterAnimator(Weapon::Unarmed));
        facing_.push_back(8);   // default facing = Down/south (180 degrees)
        weapons_.push_back(Weapon::Unarmed);
        attacking_.push_back(false);
    }
}

void Game::updateAnimations(float dt)
{
    ensureAgentVisuals();
    for (size_t i = 0; i < world_.agents.size(); ++i) {
        // A one-shot attack owns the animation until it plays out; don't let
        // the usual idle/walk motion logic below re-pick a state mid-swing.
        if (attacking_[i]) {
            animators_[i].update(dt);
            if (animators_[i].finished()) {
                attacking_[i] = false;
            } else {
                continue;
            }
        }

        const Agent &a = world_.agents[i];
        float speed = a.velocity.length();

        std::string motion;
        if (speed < kMovingSpeed) {
            motion = "idle";           // keep the last facing while stopped
        } else {
            motion = "walk";
            // Face the way we move, in screen space (project velocity to iso).
            facing_[i] = dirFromScreenVec(a.velocity.x - a.velocity.y,
                                          (a.velocity.x + a.velocity.y) * 0.5f);
        }
        animators_[i].play(motion + std::to_string(facing_[i]));
        animators_[i].update(dt);
    }
}

void Game::cycleWeapon()
{
    ensureAgentVisuals();
    if (world_.agents.empty() || selectedAgentIndex_ >= world_.agents.size()) {
        return;
    }
    size_t i = selectedAgentIndex_;

    // EXTEND: this switch is the cycle order (Unarmed -> Sword -> Bow -> ...);
    // add a case here for a new Weapon (see the enum in Game.hpp).
    Weapon next;
    switch (weapons_[i]) {
        case Weapon::Unarmed: next = Weapon::Sword;   break;
        case Weapon::Sword:   next = Weapon::Bow;     break;
        default:              next = Weapon::Unarmed; break;
    }
    weapons_[i] = next;
    animators_[i] = makeCharacterAnimator(next);   // rebuild against the new weapon's sheets
    attacking_[i] = false;
}

void Game::triggerAttack()
{
    ensureAgentVisuals();
    if (world_.agents.empty() || selectedAgentIndex_ >= world_.agents.size()) {
        return;
    }
    size_t i = selectedAgentIndex_;

    if (!weaponHasAttack(weapons_[i]) || attacking_[i]) {
        return;   // unarmed has no attack sheet; don't restart mid-swing
    }
    std::string name = "attack" + std::to_string(facing_[i]);
    if (!animators_[i].has(name)) {
        return;
    }
    animators_[i].play(name);
    attacking_[i] = true;
}

void Game::cycleSelectedAgent(int direction)
{
    if (world_.agents.empty()) {
        return;
    }
    int n = (int)world_.agents.size();
    int idx = ((int)selectedAgentIndex_ + direction) % n;
    if (idx < 0) {
        idx += n;
    }
    selectedAgentIndex_ = (size_t)idx;
}

// WATCH OUT: worldToScreen and screenToWorld below must stay exact inverses
// of each other (every on-screen thing - ground, agents, mouse-picking - goes
// through one or the other). If you change the maths in one, mirror it in
// the other, or clicking a tile stops matching what's drawn there.
void Game::worldToScreen(const Vector2 &world, int &screenX, int &screenY) const
{
    point2DT p;
    p.x = world.x * iso_->zoomLevel + iso_->scrollX;
    p.y = world.y * iso_->zoomLevel + iso_->scrollY;
    isoEngineConvert2dToIso(&p);
    screenX = (int)p.x;
    screenY = (int)p.y;
}

Vector2 Game::screenToWorld(int screenX, int screenY) const
{
    // Inverse of worldToScreen: run the camera's scroll offset through the
    // iso->2D inverse to find which world point currently sits under the
    // given screen pixel.
    point2DT camOffset;
    camOffset.x = iso_->scrollX;
    camOffset.y = iso_->scrollY;
    isoEngineConvert2dToIso(&camOffset);

    point2DT centered;
    centered.x = (float)screenX - camOffset.x;
    centered.y = (float)screenY - camOffset.y;
    isoEngineConvertIsoTo2D(&centered);

    return Vector2(centered.x / iso_->zoomLevel, centered.y / iso_->zoomLevel);
}

Vector2 Game::screenCenterWorldPos() const
{
    return screenToWorld((int)(WINDOW_WIDTH * 0.5f), (int)(WINDOW_HEIGHT * 0.5f));
}

void Game::centerCameraOn(const Vector2 &worldPos)
{
    point2DT p;
    p.x = worldPos.x;
    p.y = worldPos.y;
    isoEngineCenterMap(iso_, &p, NULL);
}

point2DT Game::mapScrollForCenterOn(const Vector2 &worldPos) const
{
    // Same formula isoEngineCenterMap uses (iso/isoEngine.cpp), minus the
    // GAME_MODE_OBJECT_FOCUS branch we never use, and without touching iso_.
    point2DT p;
    p.x = worldPos.x;
    p.y = worldPos.y;
    isoEngineConvert2dToIso(&p);

    float zoom    = iso_->zoomLevel;
    float offsetX = WINDOW_WIDTH  / zoom * 0.5f;
    float offsetY = WINDOW_HEIGHT / zoom * 0.5f;

    point2DT scroll;
    scroll.x = std::floor(p.x * zoom) * 0.5f - offsetX * zoom * 0.5f;
    scroll.y = -std::floor(p.y * zoom) + offsetY * zoom;
    return scroll;
}

void Game::clampCameraToMap()
{
    // Bound mapScroll2Dpos itself (the same continuous accumulator WASD and
    // edge-scroll both add to) instead of snapping to a tile after the fact:
    // snapping post-hoc fought edge-scroll every single frame the mouse sat
    // on a screen edge - it kept re-pushing past the limit and getting
    // yanked back, which is what showed up as camera "flicker".
    //
    // mapScrollForCenterOn maps a world point through a 45-degree rotation,
    // so the allowed tile rectangle isn't an axis-aligned box in scroll
    // space - bound it using all 4 corners, not just 2 opposite ones.
    const int margin = 4;   // tiles of "overscroll" allowed past the edge
    int xs[2] = { -margin, world_.widthTiles  - 1 + margin };
    int ys[2] = { -margin, world_.heightTiles - 1 + margin };

    float loX = 0.0f, hiX = 0.0f, loY = 0.0f, hiY = 0.0f;
    bool first = true;
    for (int xi : xs) {
        for (int yi : ys) {
            point2DT s = mapScrollForCenterOn(world_.tileCenter(xi, yi));
            if (first) {
                loX = hiX = s.x;
                loY = hiY = s.y;
                first = false;
            } else {
                loX = std::min(loX, s.x); hiX = std::max(hiX, s.x);
                loY = std::min(loY, s.y); hiY = std::max(hiY, s.y);
            }
        }
    }

    float clampedX = std::max(loX, std::min(hiX, iso_->mapScroll2Dpos.x));
    float clampedY = std::max(loY, std::min(hiY, iso_->mapScroll2Dpos.y));

    if (clampedX != iso_->mapScroll2Dpos.x || clampedY != iso_->mapScroll2Dpos.y) {
        iso_->mapScroll2Dpos.x = clampedX;
        iso_->mapScroll2Dpos.y = clampedY;
        isoEngineConvertCartesianCameraToIsometric(iso_, &iso_->mapScroll2Dpos);
    }
}

std::vector<Vector2> getGridPositions(Vector2 center, size_t numOfAgents, float spacing = 32.0f)
{
    std::vector<Vector2> positions;
    positions.reserve(numOfAgents);

    if (numOfAgents == 0) return positions;

    // std::ceil ??????????
    int cols = static_cast<int>(std::ceil(std::sqrt(numOfAgents)));
    int rows = static_cast<int>(std::ceil(static_cast<float>(numOfAgents) / cols));

    // Center the grid over the target point
    float offsetX = (cols - 1) * spacing * 0.5f;
    float offsetY = (rows - 1) * spacing * 0.5f;

    for (size_t i = 0; i < numOfAgents; ++i) {
        int r = i / cols;
        int c = i % cols;

        Vector2 pos;
        pos.x = center.x + (c * spacing) - offsetX;
        pos.y = center.y + (r * spacing) - offsetY;
        positions.push_back(pos);
    }

    return positions;
}

void Game::updateMouseTarget()
{
    if (world_.agents.empty() || selectedAgentIndex_ >= world_.agents.size()) {
        return;
    }

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    Vector2 worldPos = screenToWorld(mouseX, mouseY);

    int tileX, tileY;
    world_.worldToTile(worldPos, tileX, tileY);
    if (world_.inBounds(tileX, tileY)) {

        // AA1: el clic es el punto de llegada de TODO el ejercito
        army_.setObjective(world_.tileCenter(tileX, tileY));

    }
}

void Game::processInput()
{
    // EXTEND: new keybinding? Add an `else if` below matching the pattern
    // (SDL_KEYUP + scancode for a one-shot press; see update() for the
    // held-key/continuous style instead, e.g. WASD camera scroll).
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running_ = false;
        }
        else if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) &&
                 event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            // Deliberately both halves of the keypress: quitting shouldn't
            // depend on the window happening to hold focus for the whole of
            // it. (Alt+F4 and the window's close box arrive as SDL_QUIT.)
            running_ = false;
        }
        else if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_F11) {
            // Escape hatch: if fullscreen looks wrong on this machine's
            // screen, drop to a window (and back) without rebuilding. Start
            // the program with --windowed to boot straight into windowed.
            Renderer::Instance()->toggleFullscreen();
        }
        else if (event.type == SDL_WINDOWEVENT &&
                 (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                  event.window.event == SDL_WINDOWEVENT_RESIZED)) {
            // WINDOW_WIDTH/HEIGHT are the screen size the camera maths uses,
            // so they have to follow the window - see renderer.hpp.
            Renderer::Instance()->refreshWindowSize();
        }
        else if (event.type == SDL_KEYUP &&
                 (event.key.keysym.scancode == SDL_SCANCODE_F1 || event.key.keysym.scancode == SDL_SCANCODE_I)) {
            // I is a second binding alongside F1 - on plenty of laptops F1 is
            // a media/brightness key by default (needs Fn+F1 for the actual
            // F1 keycode) and silently does nothing otherwise, so debug mode
            // shouldn't be reachable only through it.
            debugMode_ = !debugMode_;
        }
        else if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_G) {
            showGrid_ = !showGrid_;
        }
        else if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_TAB) {
            cycleWeapon();
        }
        else if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
            triggerAttack();
        }
        else if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
            // Selects which agent Click sets the target of, and (F1) whose
            // influence map is shown - works regardless of debug mode; only
            // the on-screen "which one is selected" marker needs F1.
            cycleSelectedAgent(-1);
        }
        else if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
            cycleSelectedAgent(1);
        }
        else if (event.type == SDL_MOUSEWHEEL) {
            // Zoom around the screen centre, not whatever tile the mouse
            // happens to be over (that could be anywhere, including past the
            // map edge right after edge-scrolling, landing you in empty space).
            Vector2 centerWorld = screenCenterWorldPos();
            point2DT center;
            center.x = centerWorld.x;
            center.y = centerWorld.y;
            iso_->tilePos = center;   // isoEngineZoomIn/Out re-centre on this
            if (event.wheel.y > 0) {
                isoEngineZoomIn(iso_);
            }
            else if (event.wheel.y < 0) {
                isoEngineZoomOut(iso_);
            }
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            // Set the steering target on click, not continuously on hover.
            // (updateMouseTarget reads the mouse position itself, fresh.)
            updateMouseTarget();
        }
    }
}

void Game::update(float dt)
{
    float step = iso_->mapScrollSpeed * dt;

    // Camera scrolling: WASD. (Arrow keys are Left/Right = cycle selected
    // agent, see processInput - Up/Down are free but left unbound rather
    // than half-repurposing the arrow cluster.)
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    Vector2 scroll(0.0f, 0.0f);
    if (keys[SDL_SCANCODE_W]) scroll.y += step;
    if (keys[SDL_SCANCODE_S]) scroll.y -= step;
    if (keys[SDL_SCANCODE_A]) scroll.x -= step;
    if (keys[SDL_SCANCODE_D]) scroll.x += step;
    if (scroll.x != 0.0f || scroll.y != 0.0f) {
        iso_->mapScroll2Dpos.x += scroll.x;
        iso_->mapScroll2Dpos.y += scroll.y;
        isoEngineConvertCartesianCameraToIsometric(iso_, &iso_->mapScroll2Dpos);
    }

    // Mouse: resolve position and edge-scroll (the steering target itself is
    // only set on click; see processInput).
    isoEngineUpdateMousePos(iso_);
    if (iso_->gameMode == GAME_MODE_OVERVIEW) {
        isoEngineScrollMapWithMouse(iso_);
    }

    // Keep the camera from panning far enough that the map scrolls off-screen.
    clampCameraToMap();

    // Advance the AI, then sync each agent's animation to its motion.
    world_.update(dt);
    army_.update(dt);
    updateAnimations(dt);
}

void Game::render()
{
    SDL_Renderer *r = Renderer::Instance()->getRenderer();
    SDL_SetRenderDrawColor(r, 0x3b, 0x3b, 0x3b, 0x00);
    SDL_RenderClear(r);

    // Flat ground first: code-drawn (no texture), windowed to the visible
    // tiles, so it scales with World::tileSize automatically.
    drawGround();

    // Influence-map overlay (F1): tint each tile by the selected agent's own
    // influenceAt (Left/Right cycles which agent, when there's more than
    // one), right on top of the ground so objects/agents still draw
    // normally above it.
    if (debugMode_ && !world_.agents.empty()) {
        if (selectedAgentIndex_ >= world_.agents.size()) {
            selectedAgentIndex_ = 0;
        }
        const Agent &inspected = world_.agents[selectedAgentIndex_];
        float halfW = world_.tileSize * iso_->zoomLevel;
        float halfH = (world_.tileSize * 0.5f) * iso_->zoomLevel;
        for (int y = 0; y < world_.heightTiles; ++y) {
            for (int x = 0; x < world_.widthTiles; ++x) {
                float v = inspected.influenceAt(x, y);
                if (v == 0.0f) {
                    continue;
                }
                int sx, sy;
                worldToScreen(world_.tileCenter(x, y), sx, sy);
                float mag = std::min(std::fabs(v), 1.0f);
                SDL_Color c = (v > 0.0f)
                    ? SDL_Color{ 0xff, 0x60, 0x00, (Uint8)(mag * 160) }   // positive: warm
                    : SDL_Color{ 0x00, 0x90, 0xff, (Uint8)(mag * 160) };  // negative: cool
                DebugDraw::filledDiamond(sx, sy, (int)halfW, (int)halfH, c);
            }
        }
    }

    ensureAgentVisuals();

    // Then objects + agents, back-to-front by isometric depth (world x + y).
    // The ground is flat, so this gives the same occlusion as full row
    // integration, but every sprite is always drawn (culled only by SDL against
    // the screen), so objects near the screen borders don't flicker.
    struct Drawable { float depth; int kind; size_t idx; };   // kind: 0 agent, 1 object
    std::vector<Drawable> drawList;
    drawList.reserve(world_.agents.size() + world_.objects.size());
    for (size_t i = 0; i < world_.agents.size(); ++i) {
        const Vector2 &p = world_.agents[i].position;
        drawList.push_back({ p.x + p.y, 0, i });
    }
    for (size_t j = 0; j < world_.objects.size(); ++j) {
        const Vector2 &p = world_.objects[j].position;
        drawList.push_back({ p.x + p.y, 1, j });
    }
    std::sort(drawList.begin(), drawList.end(),
              [](const Drawable &a, const Drawable &b) {
                  if (a.depth != b.depth) return a.depth < b.depth;
                  return a.kind > b.kind;   // at a tie, object first -> agent on top
              });
    for (const Drawable &d : drawList) {
        if (d.kind == 0) {
            drawAgent(d.idx);
        } else {
            drawObject(d.idx);
        }
    }

    // Debug overlay (F1 to toggle): each agent's own steering target + a
    // line to it, and a marker on every object's logical (collision) tile
    // centre — useful for checking a sprite is actually drawn where its
    // blocked tile is.
    if (debugMode_) {
        const SDL_Color targetColor = { 0xff, 0xd2, 0x00, 0xff };
        const SDL_Color lineColor   = { 0x30, 0xd0, 0x30, 0xff };
        for (const Agent &agent : world_.agents) {
            if (!agent.hasTarget) {
                continue;
            }
            int sx, sy, tx, ty;
            worldToScreen(agent.position, sx, sy);
            worldToScreen(agent.target, tx, ty);
            DebugDraw::line(sx, sy, tx, ty, lineColor);
            DebugDraw::marker(tx, ty, (int)(6 * iso_->zoomLevel), targetColor);
        }

        const SDL_Color blockColor = { 0xff, 0x20, 0x20, 0xff };
        for (const WorldObject &obj : world_.objects) {
            int sx, sy;
            worldToScreen(obj.position, sx, sy);
            DebugDraw::marker(sx, sy, (int)(5 * iso_->zoomLevel), blockColor);
        }

        // Which agent's influence map is currently shown above (only
        // meaningful, and only drawn, once there's more than one to
        // distinguish between). Left/Right arrows cycle it.
        if (world_.agents.size() > 1 && selectedAgentIndex_ < world_.agents.size()) {
            const SDL_Color selectColor = { 0x00, 0xff, 0xff, 0xff };
            int sx, sy;
            worldToScreen(world_.agents[selectedAgentIndex_].position, sx, sy);
            DebugDraw::marker(sx, sy, (int)(10 * iso_->zoomLevel), selectColor);
        }
    }

    SDL_RenderPresent(r);
}

void Game::drawGround()
{
    // Which tiles are visible: inverse-project the four screen corners into
    // world space, then bound them in tile space. This over-shoots a little
    // (the axis-aligned box around a rotated diamond), which is fine - it's
    // still a small fraction of the full map, and never under-shoots.
    Vector2 corners[4] = {
        screenToWorld(0, 0),
        screenToWorld(WINDOW_WIDTH, 0),
        screenToWorld(0, WINDOW_HEIGHT),
        screenToWorld(WINDOW_WIDTH, WINDOW_HEIGHT),
    };
    int minX, minY, maxX, maxY;
    world_.worldToTile(corners[0], minX, minY);
    maxX = minX;
    maxY = minY;
    for (int i = 1; i < 4; ++i) {
        int tx, ty;
        world_.worldToTile(corners[i], tx, ty);
        minX = std::min(minX, tx); maxX = std::max(maxX, tx);
        minY = std::min(minY, ty); maxY = std::max(maxY, ty);
    }
    const int margin = 2;   // tiles of slack so edge diamonds aren't clipped
    minX = std::max(0, minX - margin);
    minY = std::max(0, minY - margin);
    maxX = std::min(world_.widthTiles  - 1, maxX + margin);
    maxY = std::min(world_.heightTiles - 1, maxY + margin);
    if (minX > maxX || minY > maxY) {
        return;
    }

    float halfW = world_.tileSize * iso_->zoomLevel;
    float halfH = world_.tileSize * 0.5f * iso_->zoomLevel;

    int totalTiles    = world_.widthTiles * world_.heightTiles;
    int decalModulus  = std::max(1, totalTiles / 8);   // ~8 decals, whole map

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            int sx, sy;
            worldToScreen(world_.tileCenter(x, y), sx, sy);
            DebugDraw::filledDiamond(sx, sy, (int)halfW, (int)halfH, jitteredGroundColor(x, y));
            drawDecal(x, y, sx, sy, halfW, halfH, decalModulus);
        }
    }

    if (showGrid_) {
        // Dots at shared tile corners, not full grid lines - far less noise,
        // same readability - faded out with distance from the cursor.
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        for (int y = minY; y <= maxY + 1; ++y) {
            for (int x = minX; x <= maxX + 1; ++x) {
                int sx, sy;
                worldToScreen(Vector2(x * (float)world_.tileSize, y * (float)world_.tileSize), sx, sy);
                drawGridDot(sx, sy, mouseX, mouseY);
            }
        }
    }
}

void Game::drawAgent(size_t index)
{
    int sx, sy;
    worldToScreen(world_.agents[index].position, sx, sy);

    const SDL_Rect *clip  = animators_[index].clip();
    Texture        *sheet = animators_[index].sheet();
    if (clip == nullptr || sheet == nullptr) {
        return;
    }
    SDL_Rect c = *clip;
    // Anchor by the feet: kSpriteAnchorX/Y is the pack's ground-contact point
    // within its fixed 180x180 frame (see the constants' comment).
    int drawX = (int)(sx - kSpriteAnchorX * iso_->zoomLevel);
    int drawY = (int)(sy - kSpriteAnchorY * iso_->zoomLevel);

    // Shadow first, body on top - both share the same frame grid/clip rect.
    Texture *shadow = animators_[index].shadowSheet();
    if (shadow != nullptr) {
        shadow->textureRenderXYClipScale(drawX, drawY, &c, iso_->zoomLevel);
    }
    sheet->textureRenderXYClipScale(drawX, drawY, &c, iso_->zoomLevel);
}

// EXTEND: this if/else if chain is the ObjectType switch (see WorldObject.hpp)
// - add a branch here for a new ObjectType, with its own anchor/draw logic.
void Game::drawObject(size_t index)
{
    const WorldObject &obj = world_.objects[index];
    float zoom = iso_->zoomLevel;

    if (obj.type == ObjectType::TilesetTile || obj.type == ObjectType::Column) {
        // Draw tile-set tiles at the tile's isometric origin (exactly like a map
        // tile), but here in the depth-sorted pass so they interleave with agents.
        if (tilesetTex_ == nullptr) {
            return;
        }
        // Anchor at the same world-space point collision/agents/trees use (the
        // tile's centre), not the ground-layer's per-tile corner formula: the
        // tileset art's ground-contact diamond sits low in its 64x80 cell (the
        // same convention flat ground tiles use), so anchoring by the corner
        // instead of the footprint's own centre drew objects about a tile-row
        // away from the tile that's actually marked blocked.
        //
        // This offset is baked into the sprite's fixed 64x80 pixel art, not
        // World::tileSize - it must NOT scale with the tile grid's size, or
        // it drifts out of alignment the moment the two diverge (e.g. this
        // sprite sheet not yet being redrawn at the new tile resolution).
        const float kSpriteFootprintHalfHeightPx = 16.0f;
        int sx, sy;
        worldToScreen(obj.position, sx, sy);
        float anchorX = 32.0f;                                   // half of the 64px-wide clip
        float anchorY = 80.0f - kSpriteFootprintHalfHeightPx;     // vertical centre of the footprint diamond
        float drawX = sx - anchorX * zoom;
        float drawY = sy - anchorY * zoom;

        if (obj.type == ObjectType::Column) {
            // A tall wall = the cube tile (index 2) stacked upward. One tile wide
            // (no side overhang) but tall enough to hide an agent behind it.
            SDL_Rect cube;
            setupRect(&cube, 2 * 64, 0, 64, 80);
            const int   levels     = 2;      // wall height in cubes (tweak to taste)
            const float levelPixels = 34.0f;  // vertical spacing between stacked cubes
            for (int level = 0; level < levels; ++level) {
                tilesetTex_->textureRenderXYClipScale(
                    (int)drawX, (int)(drawY - level * levelPixels * zoom), &cube, zoom);
            }
        } else {
            SDL_Rect clip;
            setupRect(&clip, obj.tileIndex * 64, 0, 64, 80);
            tilesetTex_->textureRenderXYClipScale((int)drawX, (int)drawY, &clip, zoom);
        }
        return;
    }

    // Tall billboard (tree): anchor by its base.
    Texture *tex = treeTex_;
    if (tex == nullptr) {
        return;
    }
    int sx, sy;
    worldToScreen(obj.position, sx, sy);
    float anchorX = tex->width * 0.5f;
    float anchorY = tex->height - 10.0f;
    tex->textureRenderXYClipScale(
        (int)(sx - anchorX * zoom),
        (int)(sy - anchorY * zoom),
        NULL, zoom);
}

void Game::run()
{
    while (running_) {
        processInput();
        DeltaTimer::Instance()->deltaTimerUpdate();
        float dt = (float)DeltaTimer::Instance()->deltaTimerGetDeltatime();
        update(dt);
        render();
    }
}
