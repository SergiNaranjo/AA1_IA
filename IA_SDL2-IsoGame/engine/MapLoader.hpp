#ifndef ENGINE_MAPLOADER_HPP
#define ENGINE_MAPLOADER_HPP
#include <string>
#include <vector>
#include "WorldObject.hpp"

// FILE: parses data/maps/*.map text files into a MapData (size + objects).
// FITS IN: Game(mapPath, tileSize) calls loadMapFile() once at startup.
// EXTEND: add a new directive keyword in MapLoader.cpp's parseObjectDirective
// (and usually a new ObjectType - see WorldObject.hpp) - see FILE FORMAT below.

// Data-driven map definition, loaded from a simple text file.
//
// File format (one directive per line; '#' starts a comment):
//     size <width> <height>       map size in tiles
//     tree <tileX> <tileY>          tree (tall billboard)
//     wall <tileX> <tileY>          tall one-tile-wide wall (stacked cube) — occludes
//     block <tileX> <tileY>         cube block (tile-set tile 2)
//     tile <index> <tileX> <tileY>  any tile-set tile as an object (e.g. 5 = fence)
//     repeat <count> <dx> <dy> <object-directive...>
//                                    <count> copies of the directive, each shifted
//                                    (dx, dy) tiles further than the last — a row or
//                                    column instead of writing N lines by hand.
//
// Example:
//     size 64 64
//     tree 20 20
//     wall 30 30
//     block 36 30
//     tile 5 40 40
//     repeat 5 1 0 tile 5 40 41       # 5 fence tiles in a row along X
//     repeat 5 0 1 wall 30 30         # 5-tile-long wall along Y
struct MapObjectSpec
{
    ObjectType type;
    int tileX;
    int tileY;
    int tileIndex = -1;   // tile-set tile when type == TilesetTile
};

struct MapData
{
    int width  = 64;
    int height = 64;
    std::vector<MapObjectSpec> objects;
};

// Summary: parse the map file at `path` into `out`.
// Inputs: path to a text file in the format documented above.
// Output/side effects: fills out.width/height/objects; on failure `out`
// keeps its MapData defaults (64x64, no objects) rather than being left
// partially written.
// Errors: returns false if the file can't be opened; unrecognised or
// malformed directives are skipped line-by-line (not fatal) and logged to
// runlog.txt, not thrown/returned - check the log if objects seem missing.
bool loadMapFile(const std::string &path, MapData &out);

#endif // ENGINE_MAPLOADER_HPP
