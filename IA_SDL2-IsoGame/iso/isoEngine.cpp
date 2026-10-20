// FILE: isoEngine implementation - see isoEngine.hpp for the contracts.
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "isoEngine.hpp"
#include "../core/logger.hpp"
#include "../core/deltatimer.hpp"
#include "../core/renderer.hpp"

isoEngineT *isoEngineNewIsoEngine()
{
    isoEngineT *isoEngine = (isoEngineT*)malloc(sizeof(struct isoEngineT));

    if(isoEngine == NULL)
    {
        Logger::Instance()->writeToLog("Error in isoEngineNewIsoEngine(): Could not allocate memory for new isoEngine!","runlog.txt");
        return NULL;
    }
    isoEngine->scrollX = 0;
    isoEngine->scrollY = 0;
    isoEngine->mapScrollSpeed = 300;
    isoEngine->mapScroll2Dpos.x = 0;
    isoEngine->mapScroll2Dpos.y = 0;
    isoEngine->zoomLevel = 1.0;
    isoEngine->isoMap = NULL;
    isoEngine->gameMode = GAME_MODE_OVERVIEW;

    setupRect(&isoEngine->mouseRect,0,0,1,1);
    isoEngine->tilePos.x = 0;
    isoEngine->tilePos.y = 0;

    return isoEngine;
}

void isoEngineFreeIsoEngine(isoEngineT *isoEngine)
{
    if(isoEngine != NULL)
    {
        if(isoEngine->isoMap!=NULL)
        {
            isoMapFreeMap(isoEngine->isoMap);
        }
        free(isoEngine);
    }
}

void isoEngineConvert2dToIso(point2DT *point)
{
    int tmpX = point->x - point->y;
    int tmpY = (point->x + point->y)*0.5;
    point->x = tmpX;
    point->y = tmpY;
}

void isoEngineConvertIsoTo2D(point2DT *point)
{
    int tmpX = (2 * point->y + point->x)*0.5;
    int tmpY = (2 * point->y - point->x)*0.5;
    point->x = tmpX;
    point->y = tmpY;
}

void isoEngineConvertCartesianCameraToIsometric(isoEngineT *isoEngine,point2DT *cartesianCamPos)
{
    point2DT tmpPoint;

    if(isoEngine == NULL || cartesianCamPos == NULL)
    {
        return;
    }
    tmpPoint = *cartesianCamPos;

    tmpPoint.x = (int)tmpPoint.x*2;

    if(tmpPoint.x<0){
        tmpPoint.x = abs((int)tmpPoint.x);
    }
    else if(tmpPoint.x>0)
    {
        tmpPoint.x = -abs((int)tmpPoint.x);
    }

    isoEngineConvertIsoTo2D(&tmpPoint);

    isoEngine->scrollX = (int)tmpPoint.x;
    isoEngine->scrollY = (int)tmpPoint.y;
}

void isoEngineUpdateMousePos(isoEngineT *isoEngine)
{
    SDL_GetMouseState(&isoEngine->mouseRect.x,&isoEngine->mouseRect.y);
    isoEngine->mouseRect.x = isoEngine->mouseRect.x/isoEngine->zoomLevel;
    isoEngine->mouseRect.y = isoEngine->mouseRect.y/isoEngine->zoomLevel;
}

void isoEngineScrollMapWithMouse(isoEngineT *isoEngine)
{
    int zoomEdgeX = (WINDOW_WIDTH*isoEngine->zoomLevel)-(WINDOW_WIDTH);
    int zoomEdgeY = (WINDOW_HEIGHT*isoEngine->zoomLevel)-(WINDOW_HEIGHT);

    if(isoEngine->mouseRect.x<2){
        isoEngine->mapScroll2Dpos.x-=isoEngine->mapScrollSpeed*DeltaTimer::Instance()->deltaTimerGetDeltatime();
        isoEngineConvertCartesianCameraToIsometric(isoEngine,&isoEngine->mapScroll2Dpos);
    }
    if(isoEngine->mouseRect.x>WINDOW_WIDTH-(zoomEdgeX/isoEngine->zoomLevel)-2){
        isoEngine->mapScroll2Dpos.x+=isoEngine->mapScrollSpeed*DeltaTimer::Instance()->deltaTimerGetDeltatime();
        isoEngineConvertCartesianCameraToIsometric(isoEngine,&isoEngine->mapScroll2Dpos);

    }
    if(isoEngine->mouseRect.y<2){
        isoEngine->mapScroll2Dpos.y+=isoEngine->mapScrollSpeed*DeltaTimer::Instance()->deltaTimerGetDeltatime();
        isoEngineConvertCartesianCameraToIsometric(isoEngine,&isoEngine->mapScroll2Dpos);

    }
    if(isoEngine->mouseRect.y>WINDOW_HEIGHT-(zoomEdgeY/isoEngine->zoomLevel)-2){
        isoEngine->mapScroll2Dpos.y-=isoEngine->mapScrollSpeed*DeltaTimer::Instance()->deltaTimerGetDeltatime();
        isoEngineConvertCartesianCameraToIsometric(isoEngine,&isoEngine->mapScroll2Dpos);
    }
}

void isoEngineCenterMap(isoEngineT *isoEngine,point2DT *objectPoint,point2DT *objectSize)
{
    point2DT pointPos = *objectPoint;
    point2DT objSize;
    int moveAmount = 0;
    //calculate the offset of the center of the screen
    int offsetX = WINDOW_WIDTH/isoEngine->zoomLevel*0.5;
    int offsetY = WINDOW_HEIGHT/isoEngine->zoomLevel*0.5;

    objSize.x = 0;
    objSize.y = 0;

    //if an object size is passed in
    if(objectSize!=NULL){
        objSize = *objectSize;
    }
    // WATCH OUT: the GAME_MODE_OBJECT_FOCUS branches below are unreachable in
    // practice - nothing in this codebase ever sets gameMode to it (only
    // GAME_MODE_OVERVIEW is used). Designed-but-not-yet-used, not dead code
    // to delete; wire up isoEngineSetGameMode(..., GAME_MODE_OBJECT_FOCUS)
    // somewhere if you want this path to actually run.

    isoEngine->tilePos.x = objectPoint->x;
    isoEngine->tilePos.y = objectPoint->y;

    isoEngineConvert2dToIso(&pointPos);

    isoEngine->mapScroll2Dpos.x = floor((pointPos.x)*isoEngine->zoomLevel)*0.5;
    isoEngine->mapScroll2Dpos.x -= offsetX*isoEngine->zoomLevel*0.5;

    if(isoEngine->gameMode == GAME_MODE_OBJECT_FOCUS){
        moveAmount = (int)(objSize.x*isoEngine->zoomLevel*0.5);
        isoEngine->mapScroll2Dpos.x += moveAmount*0.5;
    }

    isoEngine->mapScroll2Dpos.y = -floor((pointPos.y)*isoEngine->zoomLevel);
    isoEngine->mapScroll2Dpos.y += offsetY*isoEngine->zoomLevel;

    if(isoEngine->gameMode == GAME_MODE_OBJECT_FOCUS){

        moveAmount = (int)(objSize.y*isoEngine->zoomLevel*0.5);
        isoEngine->mapScroll2Dpos.y -= moveAmount;
    }

    isoEngineConvertCartesianCameraToIsometric(isoEngine,&isoEngine->mapScroll2Dpos);
}


// 3.0 / 0.5 / 0.25: zoom range and step, purely a feel choice - README
// documents the mouse-wheel zoom range as [0.5, 3.0]; change both together.
void isoEngineZoomIn(isoEngineT *isoEngine)
{
    if(isoEngine->zoomLevel<3.0){
        isoEngine->zoomLevel+=0.25;

        if(isoEngine->gameMode==GAME_MODE_OVERVIEW)
        {
            isoEngineCenterMap(isoEngine,&isoEngine->tilePos,NULL);
        }
    }
}
void isoEngineZoomOut(isoEngineT *isoEngine)
{
    if(isoEngine->zoomLevel>0.5){
        isoEngine->zoomLevel-=0.25;

        if(isoEngine->gameMode==GAME_MODE_OVERVIEW)
        {
            isoEngineCenterMap(isoEngine,&isoEngine->tilePos,NULL);
        }
    }
}

void isoEngineSetGameMode(isoEngineT *isoEngine,isoEngineGameModeE gameMode)
{
    if(isoEngine == NULL){
        return;
    }
    isoEngine->gameMode = gameMode;
}
