#ifndef ENGINE_WORLDOBJECT_HPP
#define ENGINE_WORLDOBJECT_HPP
#include "Vector2.hpp"

// FILE: a static object placed on the map (tree, block, wall).
// FITS IN: AI-facing - it occupies a tile World marks as blocked, so agents
// and pathfinding treat it as an obstacle. How it's drawn is Game's concern.
// EXTEND: add a case here, then a matching case in Game::drawObject
// (engine/Game.cpp) so the renderer knows how to draw the new kind.

// EXTEND: new object kinds go here. Tree = billboard sprite (isotree.png);
// TilesetTile = one-tile tile-set sprite (cube=2, fences=5-8); Column = tall
// wall built by stacking the cube tile (tall enough to hide an agent, narrow
// enough not to overhang in front). See Game::drawObject for the matching switch.
enum class ObjectType { Tree, TilesetTile, Column };

struct WorldObject
{
    ObjectType type = ObjectType::Tree;
    int     tileX = 0;
    int     tileY = 0;
    Vector2 position;      // world-space (tile centre)
    int     tileIndex = -1;  // which tile-set tile, when type == TilesetTile
};

#endif // ENGINE_WORLDOBJECT_HPP
