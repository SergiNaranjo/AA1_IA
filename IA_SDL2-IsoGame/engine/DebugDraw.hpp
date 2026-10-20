#ifndef ENGINE_DEBUGDRAW_HPP
#define ENGINE_DEBUGDRAW_HPP
#include <SDL2/SDL.h>

// FILE: simple debug-drawing primitives (markers/lines/diamonds), screen space.
// FITS IN: Game calls these to visualise AI state (targets, influence) when
// F1/I debug mode is on; Agent/World never call this (no rendering there).
// EXTEND: add a path-drawing or state-label helper here, then call it from
// Game::render's debug block for a new "what is the AI doing" visualisation.
// WATCH OUT: all coordinates are screen pixels - run world positions through
// Game::worldToScreen first, or markers land in the wrong place.
namespace DebugDraw
{
    // A small diamond marker centred on (x, y).
    void marker(int x, int y, int size, SDL_Color color);

    // A line between two screen points.
    void line(int x1, int y1, int x2, int y2, SDL_Color color);

    // A filled diamond (isometric tile footprint), centred on (cx, cy),
    // halfW/halfH pixels from centre to each vertex. Used for the influence
    // map overlay. Colour alpha is respected (blend mode is set internally).
    void filledDiamond(int cx, int cy, int halfW, int halfH, SDL_Color color);
}

#endif // ENGINE_DEBUGDRAW_HPP
