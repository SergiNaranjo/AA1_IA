#ifndef __ISO_MAP_H_
#define __ISO_MAP_H_

// FILE: the isometric tile-set (source art for one-tile objects: cube/fences).
// FITS IN: provided rendering code, C-style - only Game.cpp calls this. Not the AI-facing tile grid; that's
// World (engine/World.hpp). WATCH OUT: isoMapCreateEmptyMap halves tileSize
// internally - isoMap->tileSize is not what you passed in (see Game::init).

#include <SDL2/SDL.h>
#include "../core/texture.hpp"

typedef struct isoTileSetT
{
    int numTileClipRects;
    Texture *tilesTex;
    SDL_Rect *tileClipRects;
}isoTileSetT;

typedef struct isoMapT
{
    int tileSize;
    isoTileSetT *tileSet;
}isoMapT;

isoMapT* isoMapCreateEmptyMap(int tileSize);
void isoMapFreeMap(isoMapT *isoMap);
// Summary: slice `texture` into a grid of tileWidth x tileHeight clip rects
// (left-to-right, top-to-bottom) for isoMapT::tileSet.
// Errors: returns -1 (logged) if isoMap/texture is NULL or the texture is
// smaller than one tile; 1 on success. Not exceptions - always check the
// return value before using tileSet->tileClipRects.
int isoMapLoadTileSet(isoMapT *isoMap,Texture *texture,int tileWidth,int tileHeight);

#endif // __ISO_MAP_H_
