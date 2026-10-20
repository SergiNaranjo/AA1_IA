// FILE: SDL/SDL_image lifecycle - see initclose.hpp for the contract.
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include "initclose.hpp"
#include "renderer.hpp"
#include "logger.hpp"

// SDL init and close utility functions

void initSDL(char *windowName, bool fullscreen)
{
    char msg[256];

    // IMPORTANT INVARIANT: hints go *before* SDL_Init - several of them
    // (SDL_HINT_WINDOWS_DPI_AWARENESS in particular) are only read while the
    // video subsystem starts up, so setting them afterwards does nothing.

    // Tell Windows this program handles display scaling itself. Without it a
    // machine set to 125%/150% scaling gets its window silently virtualised
    // by the OS, which on top of a fullscreen window is a known cause of
    // blurry or black output. Skipped if the student has set the matching
    // environment variable by hand (SDL_WINDOWS_DPI_AWARENESS=unaware|
    // system|permonitor|permonitorv2) to test a different setting.
    if (SDL_getenv("SDL_WINDOWS_DPI_AWARENESS") == NULL) {
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    }

    SDL_SetHint(SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING, "1");

    // "0" = nearest-neighbour texture scaling (crisp pixels, no smoothing).
    if(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0")==SDL_FALSE){
        snprintf(msg,sizeof(msg),"Warning: could not set texture scaling quality!");
        Logger::Instance()->writeToLog(msg,"runlog.txt");
    }

    if(SDL_Init(SDL_INIT_VIDEO)< 0){
        snprintf(msg,sizeof(msg),"Could not initialize SDL! SDL Error:%s",SDL_GetError());
        Logger::Instance()->writeToLog(msg,"runlog.txt");
        exit(1);
    }

    SDL_version linked;
    SDL_GetVersion(&linked);
    snprintf(msg,sizeof(msg),"SDL version:%d.%d.%d",linked.major,linked.minor,linked.patch);
    Logger::Instance()->writeToLog(msg,"runlog.txt");

    Renderer::Instance()->initRenderer(windowName, fullscreen);

    if( !(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        snprintf(msg,sizeof(msg),"Could not initialize SDL_image! SDL_image error:%s",IMG_GetError());
        Logger::Instance()->writeToLog(msg,"runlog.txt");
        exit(1);
    }

}

void closeDownSDL()
{
    Renderer::Instance()->closeRenderer();
    IMG_Quit();
    SDL_Quit();
}
