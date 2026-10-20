#ifndef __ISOENGINE_H_
#define __ISOENGINE_H_
// FILE: isometric camera/projection state (scroll, zoom) + the 2D<->iso maths.
// FITS IN: provided rendering code, C-style; Game.cpp is the only
// caller. AI code (engine/) never touches this - Game converts at draw time.
// EXTEND: not meant for new AI behaviour; extend Game/Agent/World instead.
#include <SDL2/SDL.h>
#include "isoMap.hpp"

typedef enum isoEngineGameModeE
{
        GAME_MODE_OVERVIEW = 0,
        GAME_MODE_OBJECT_FOCUS,
}isoEngineGameModeE;

typedef struct point2DT
{
    float x;
    float y;
}point2DT;

typedef struct isoEngineT
{
    int scrollX;
    int scrollY;
    int mapScrollSpeed;
    point2DT mapScroll2Dpos;
    float zoomLevel;
    SDL_Rect mouseRect;
    point2DT tilePos;
    isoMapT *isoMap;
    int gameMode;
}isoEngineT;

isoEngineT *isoEngineNewIsoEngine();
void isoEngineFreeIsoEngine(isoEngineT *isoEngine);
// WATCH OUT: these two must stay exact inverses of each other - Game.cpp's
// worldToScreen/screenToWorld are built directly on top of this pair.
void isoEngineConvert2dToIso(point2DT *point);
void isoEngineConvertIsoTo2D(point2DT *point);

void isoEngineConvertCartesianCameraToIsometric(isoEngineT *isoEngine,point2DT *cartesianCamPos);

// Mouse position (for edge-scroll detection) - NOT tile picking. Game::updateMouseTarget
// picks the tile under the mouse itself, via the exact worldToScreen/screenToWorld
// inverse transform (see engine/Game.cpp), not through this engine.
void isoEngineUpdateMousePos(isoEngineT *isoEngine);
void isoEngineScrollMapWithMouse(isoEngineT *isoEngine);
void isoEngineCenterMap(isoEngineT *isoEngine,point2DT *objectPoint,point2DT *objSize);
void isoEngineZoomIn(isoEngineT *isoEngine);
void isoEngineZoomOut(isoEngineT *isoEngine);
void isoEngineSetGameMode(isoEngineT *isoEngine,isoEngineGameModeE gameMode);

#endif // __ISOENGINE_H_
