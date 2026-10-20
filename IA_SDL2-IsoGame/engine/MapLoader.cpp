// FILE: MapLoader implementation - see MapLoader.hpp for the file format
// and the full loadMapFile contract.
#include "MapLoader.hpp"
#include "../core/logger.hpp"
#include <fstream>
#include <sstream>

namespace {

// Parse a single object directive ("tree x y", "wall x y", "block x y",
// "tile index x y") from the remaining stream. Returns false if the token
// isn't an object directive, or the arguments are malformed.
// EXTEND: add a new `if (token == "...")` case here for a new directive
// keyword (a matching ObjectType usually needs adding too, WorldObject.hpp).
bool parseObjectDirective(const std::string &token, std::istringstream &ss, MapObjectSpec &spec)
{
    if (token == "tree" || token == "wall") {
        int x, y;
        if (!(ss >> x >> y)) {
            return false;
        }
        spec.type = (token == "tree") ? ObjectType::Tree
                                       : ObjectType::Column; // occluding stacked-cube wall
        spec.tileX = x;
        spec.tileY = y;
        return true;
    }
    if (token == "block") {
        // Shorthand for the cube tile (tile-set index 2).
        int x, y;
        if (!(ss >> x >> y)) {
            return false;
        }
        spec.type = ObjectType::TilesetTile;
        spec.tileX = x;
        spec.tileY = y;
        spec.tileIndex = 2;
        return true;
    }
    if (token == "tile") {
        int index, x, y;
        if (!(ss >> index >> x >> y)) {
            return false;
        }
        spec.type = ObjectType::TilesetTile;
        spec.tileX = x;
        spec.tileY = y;
        spec.tileIndex = index;
        return true;
    }
    return false;
}

} // namespace

bool loadMapFile(const std::string &path, MapData &out)
{
    std::ifstream in(path);
    if (!in.is_open()) {
        Logger::Instance()->writeToLog("MapLoader: could not open map file: " + path, "runlog.txt");
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        // Strip comments.
        std::string::size_type hash = line.find('#');
        if (hash != std::string::npos) {
            line = line.substr(0, hash);
        }

        std::istringstream ss(line);
        std::string token;
        if (!(ss >> token)) {
            continue;   // blank line
        }

        if (token == "size") {
            int w, h;
            if (ss >> w >> h) {
                out.width = w;
                out.height = h;
            }
        }
        else if (token == "repeat") {
            // repeat <count> <dx> <dy> <object-directive...>
            // Places <count> copies of the directive, stepping (dx, dy) tiles
            // each time, e.g. `repeat 5 1 0 tile 5 30 40` places 5 fence tiles
            // in a row along X instead of 5 separate lines.
            int count, dx, dy;
            std::string innerToken;
            if (ss >> count >> dx >> dy >> innerToken) {
                MapObjectSpec base;
                if (parseObjectDirective(innerToken, ss, base)) {
                    for (int i = 0; i < count; ++i) {
                        MapObjectSpec spec = base;
                        spec.tileX += dx * i;
                        spec.tileY += dy * i;
                        out.objects.push_back(spec);
                    }
                } else {
                    Logger::Instance()->writeToLog(
                        "MapLoader: 'repeat' has an unknown/malformed inner directive '"
                        + innerToken + "' in " + path, "runlog.txt");
                }
            }
        }
        else {
            MapObjectSpec spec;
            if (parseObjectDirective(token, ss, spec)) {
                out.objects.push_back(spec);
            } else {
                Logger::Instance()->writeToLog("MapLoader: ignoring unknown directive '" + token + "' in " + path, "runlog.txt");
            }
        }
    }
    return true;
}
