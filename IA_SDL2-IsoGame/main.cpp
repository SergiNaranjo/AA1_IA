/*
 * FILE: scene setup - build a World, place agents, run. This is the file
 * students edit to set up their own scene; engine machinery lives in engine/.
 * FITS IN: main() below is the whole program entry point.
 * EXTEND: add/position agents in the loop below, or give one custom
 * behaviour by editing Agent::update (engine/Agent.cpp).
 */
#include "core/initclose.hpp"
#include "core/logger.hpp"
#include "engine/Game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

// 128: pixel width of a ground tile's iso diamond on screen (halved
// internally to a 64-unit world tile - see Game::init's WATCH OUT comment).
// Purely visual; change freely, nothing AI-side depends on the exact number.
#define TILE_SIZE 128

namespace {

// DEMO ONLY — shows off the influence-map overlay (F1); it isn't a real AI
// signal: paints a radial gradient into `agent`'s own influence map, centred
// on (centerX, centerY) - max (+1) there, fading smoothly out to min (-1) at
// the map's far corners, like a source of influence radiating out and
// decaying with distance (the classic textbook picture of what this overlay
// is for). Influence maps are per-agent (Agent::setInfluence), so each agent
// can show a different picture; press F1 to see it, and (with more than one
// agent) Left/Right to cycle whose map is drawn. Replace this with values
// computed by your own AI code.
void paintDemoInfluence(World &world, Agent &agent, int centerX, int centerY)
{
    agent.initInfluence(world);
    int mapW = world.widthTiles;
    int mapH = world.heightTiles;
    float maxDist = 0.0f;
    for (int cx : { 0, mapW - 1 }) {
        for (int cy : { 0, mapH - 1 }) {
            float dx = (float)(cx - centerX);
            float dy = (float)(cy - centerY);
            maxDist = std::max(maxDist, std::sqrt(dx * dx + dy * dy));
        }
    }
    for (int y = 0; y < mapH; ++y) {
        for (int x = 0; x < mapW; ++x) {
            float dx = (float)(x - centerX);
            float dy = (float)(y - centerY);
            float t = std::min(std::sqrt(dx * dx + dy * dy) / maxDist, 1.0f);   // 0 at centre, 1 at the far corners
            agent.setInfluence(x, y, 1.0f - 2.0f * t);                          // +1 at centre -> -1 at the edges
        }
    }
}

// Nudges (tileX, tileY) to the nearest walkable tile if it's blocked (e.g. a
// demo spawn point that happens to land on a tree) - searches outward ring
// by ring so it finds the closest opening rather than a fixed direction.
void nudgeOffBlockedTile(const World &world, int &tileX, int &tileY)
{
    if (world.isWalkable(tileX, tileY)) {
        return;
    }
    int maxRadius = std::max(world.widthTiles, world.heightTiles);
    for (int radius = 1; radius <= maxRadius; ++radius) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != radius) {
                    continue;   // only the outer ring at this radius
                }
                if (world.isWalkable(tileX + dx, tileY + dy)) {
                    tileX += dx;
                    tileY += dy;
                    return;
                }
            }
        }
    }
}

// True if the program was started with --windowed (or -w): boot into a
// 1360x768 window instead of borderless fullscreen. This is the fallback for
// a machine whose graphics driver or display setup misbehaves fullscreen -
// F11 toggles the same thing while running.
bool wantsWindowed(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--windowed") == 0 || std::strcmp(argv[i], "-w") == 0) {
            return true;
        }
    }
    return false;
}

// Checks that we're being run from the project folder, where data/ and logs/
// live. Launching the .exe from inside bin\Debug\ instead gives a game with
// no art and no map, which is confusing to debug from the symptoms alone, so
// say so plainly in the log and on the console instead.
void warnIfAssetsMissing(const std::string &mapPath)
{
    FILE *f = fopen(mapPath.c_str(), "r");
    if (f != NULL) {
        fclose(f);
        return;
    }
    std::string msg = "ERROR: cannot find '" + mapPath + "' from the current working directory. "
                      "Run the game from the project folder (the one holding data/ and logs/), "
                      "not from bin/Debug. In Visual Studio this is the Debugging > Working "
                      "Directory setting, already set to the project folder in IsoGame.vcxproj.";
    Logger::Instance()->writeToLog(msg, "runlog.txt");
    fprintf(stderr, "%s\n", msg.c_str());
}

} // namespace

int main(int argc, char *argv[])
{
    Logger::Instance()->setLoggerDirectory("logs");
    Logger::Instance()->deleteLog("runlog.txt");

    const char *kMapPath = "data/maps/demo.map";
    warnIfAssetsMissing(kMapPath);

    initSDL((char *)"IsoEngine - AI Sandbox", !wantsWindowed(argc, argv));

    // Build the runtime from a data-driven map (size + trees/blocks) ...
    Game game(kMapPath, TILE_SIZE);

    // ... place a few agents near the middle of the map, close enough
    // together to demo Left/Right agent-cycling at a glance - Click sets
    // the *selected* agent's target (Left/Right picks which one, Click
    // moves it), and each gets its own personalised influence map, centred
    // on its own spawn tile, so F1 visibly shows a different overlay
    // per agent too.
    // EXTEND: add/move agents here - push_back as many as you like, give
    // each a distinct position.x/y (world units - see World::tileCenter),
    // and optionally its own maxSpeed/radius/influence map.
    int centerX = game.world().widthTiles / 2;
    int centerY = game.world().heightTiles / 2;
    const int spawnOffsets[12][2] = { { 0,  0 }, { -2, -2 }, {  2, -2 }, { -4, -4 }, {  0, -4 }, { 4, -4 }, { -6, -6 }, { -2, -6 }, {  2, -6 }, {  3, -6 }, {  2, 5 }, {  6, -6 } };
    for (const auto& offset : spawnOffsets) {
        int tileX = centerX + offset[0];
        int tileY = centerY + offset[1];
        nudgeOffBlockedTile(game.world(), tileX, tileY);
        Agent agent;
        agent.position = game.world().tileCenter(tileX, tileY);
        paintDemoInfluence(game.world(), agent, tileX, tileY);
        game.world().agents.push_back(agent);
    }
    game.army().addAllAgents();
    game.centerCameraOn(game.world().agents[0].position);

    // ... and run. Move the mouse; the agents seek the tile under it,
    // and the trees/blocks from the map file are placed as obstacles.
    game.run();

    closeDownSDL();
    return 0;
}
