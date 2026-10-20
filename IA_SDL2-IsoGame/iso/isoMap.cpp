// FILE: isoMap implementation - see isoMap.hpp for the contracts/EXTEND note.
#include <stdlib.h>
#include <math.h>
#include "isoMap.hpp"
#include "../core/texture.hpp"
#include "../core/logger.hpp"

isoMapT* isoMapCreateEmptyMap(int tileSize)
{
    //allocate memory for the map
    isoMapT *isoMap = (isoMapT*)malloc(sizeof(struct isoMapT));
    if(isoMap == NULL){
        Logger::Instance()->writeToLog("Error in function: isoMapCreateEmptyMap() - Could not allocate memory for isometric map!","runlog.txt");
        return NULL;
    }

    //allocate memory for the tile set
    isoMap->tileSet = (isoTileSetT*)malloc(sizeof(struct isoTileSetT));
    if(isoMap->tileSet == NULL)
    {
        Logger::Instance()->writeToLog("Error in function: isoMapCreateEmptyMap() - Could not allocate memory for tile set!","runlog.txt");
        return NULL;
    }

    isoMap->tileSet->numTileClipRects = 0;
    isoMap->tileSet->tilesTex = NULL;
    isoMap->tileSet->tileClipRects = NULL;

    // WATCH OUT: halved here - see the WATCH OUT in isoMap.hpp for why.
    isoMap->tileSize = tileSize/2;
    return isoMap;
}

void isoMapFreeMap(isoMapT *isoMap)
{
    if(isoMap != NULL)
    {
        if(isoMap->tileSet!=NULL)
        {
            if(isoMap->tileSet->tileClipRects!=NULL){
                free(isoMap->tileSet->tileClipRects);
            }
            //Freeing textures is handled by the texture pool, so we don't
            //free the memory the texture pointer is pointing too.
            free(isoMap->tileSet);
        }
        free(isoMap);
    }
}

int isoMapLoadTileSet(isoMapT *isoMap,Texture *texture,int tileWidth,int tileHeight)
{
    int x=0,y=0;
    int w,h;
    int numTilesX;
    int numTilesY;
    int i = 0;
    SDL_Rect tmpRect;

    if(isoMap == NULL)
    {
        Logger::Instance()->writeToLog("Error in function: isoMapLoadTileSet() - Parameter: 'isoMapT *isoMap' is NULL!","runlog.txt");
        return -1;
    }
    //if the texture is NULL
    if(texture == NULL){
        Logger::Instance()->writeToLog("Error in function: isoMapLoadTileSet() - Parameter: 'textureT *texture' is NULL!","runlog.txt");
        return -1;
    }
    //if a tile set already has been loaded
    if(isoMap->tileSet->tileClipRects!=NULL){
        free(isoMap->tileSet->tileClipRects);
    }

    isoMap->tileSet->tilesTex = texture;

    //get width and height
    w = isoMap->tileSet->tilesTex->width;
    h = isoMap->tileSet->tilesTex->height;

    if(w<tileWidth){
        Logger::Instance()->writeToLog("Error in function: isoMapLoadTileSet() - Texture width is smaller than the tile width! Aborting!","runlog.txt");
        return -1;
    }
    if(h<tileHeight){
        Logger::Instance()->writeToLog("Error in function: isoMapLoadTileSet() - Texture height is smaller than the tile height! Aborting!","runlog.txt");
        return -1;
    }
    // WATCH OUT: assumes w/h are exact multiples of tileWidth/tileHeight -
    // floor() below silently drops a leftover partial row/column instead of
    // erroring, so a mis-sized sprite sheet just loses tiles quietly.
    numTilesX = floor(w/tileWidth);
    numTilesY = floor(h/tileHeight);

    //set the number of clip rectangles
    isoMap->tileSet->numTileClipRects = numTilesX * numTilesY;

    //allocate memory for the tile clip rectangles
    isoMap->tileSet->tileClipRects = (SDL_Rect*)malloc(sizeof(SDL_Rect)*isoMap->tileSet->numTileClipRects);

    //loop through the texture
    while(1)
    {
        //setup the clip rectangle
        setupRect(&tmpRect,x,y,tileWidth,tileHeight);

        //copy the rectangle
        isoMap->tileSet->tileClipRects[i] = tmpRect;

        //go to the next tile tile the image
        x+=tileWidth;

        //if the x position passed the width of the texture
        if(x>=w){
            //reset the x position
            x = 0;
            //go to the next row in the image
            y += tileHeight;
            if(y>=h){
                //break out of the while loop. No more clip rectangles to create.
                break;
            }
        }
        i++;
    }
    return 1;
}
