#ifndef ENGINE_WORLD_HPP
#define ENGINE_WORLD_HPP
#include <vector>
#include <cstdint>
#include "Vector2.hpp"
#include "Agent.hpp"
#include "WorldObject.hpp"

// FILE: the AI-facing tile grid + agent list - what students' AI code reads.
// FITS IN: pure data + queries, no SDL/rendering. Each agent keeps its own
// steering target and influence map (see Agent); World only tracks the
// grid (walkable/blocked) and who's in it.
// EXTEND: add another shared per-tile layer the same way blocked_ is done
// (a std::vector sized widthTiles*heightTiles) for a new shared AI signal.
//
// Coordinates: tiles are indexed (tileX, tileY); world-space positions are in
// the same units the renderer expects (tile index * tileSize).
class World
{
public:
    World(int widthTiles = 0, int heightTiles = 0, int tileSize = 0);

    int widthTiles  = 0;
    int heightTiles = 0;
    int tileSize    = 0;

    std::vector<Agent>       agents;
    std::vector<WorldObject> objects;

    // --- Tile queries ---------------------------------------------------
    bool inBounds(int tileX, int tileY) const;
    // Summary: can an agent stand here? Output: false for out-of-bounds
    // tiles too (not just blocked ones) - callers don't need a separate
    // inBounds() check first. Errors: none.
    bool isWalkable(int tileX, int tileY) const;
    void setBlocked(int tileX, int tileY, bool blocked);

    // Summary: place an object on a tile - records it and marks the tile
    // blocked. tileIndex selects the tile-set tile when type == TilesetTile.
    // EXTEND: pairs with ObjectType (WorldObject.hpp) and the matching draw
    // case in Game::drawObject - extend all three together.
    void addObject(ObjectType type, int tileX, int tileY, int tileIndex = -1);

    // --- World <-> tile conversion --------------------------------------
    Vector2 tileCenter(int tileX, int tileY) const;
    // WATCH OUT: casts to int, which truncates toward zero, not floor - a
    // negative world position (e.g. just off the map's top/left edge) does
    // NOT round down to tile -1 the way you might expect; see World.cpp.
    void    worldToTile(const Vector2 &p, int &tileX, int &tileY) const;

    // --- Simulation -----------------------------------------------------
    void update(float dt);   // advances every agent

private:
    std::vector<uint8_t> blocked_;    // 1 = blocked, size widthTiles * heightTiles
    // WATCH OUT: no bounds check here - every caller must go through
    // inBounds()/isWalkable() first (see World.cpp), or this indexes outside
    // blocked_'s storage.
    int index(int tileX, int tileY) const { return tileY * widthTiles + tileX; }
};

#endif // ENGINE_WORLD_HPP
