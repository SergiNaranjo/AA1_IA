// FILE: Renderer implementation - see renderer.hpp for the contract.
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "renderer.hpp"
#include "logger.hpp"

Renderer * Renderer::s_pInstance = 0;
SDL_Window * Renderer::window = 0;
SDL_Renderer * Renderer::renderer = 0;
bool Renderer::fullscreen = false;

// Windowed-mode default only - initRenderer() replaces both with the size the
// window really got. See the WATCH OUT in renderer.hpp.
int WINDOW_WIDTH  = 1360;
int WINDOW_HEIGHT = 768;

Renderer * Renderer::Instance()
{
	if (s_pInstance == 0)
	{
		s_pInstance = new Renderer();
	}
	return s_pInstance;
}

namespace {

// Writes what SDL ended up giving us into logs/runlog.txt: display count and
// mode, window size, and which rendering backend was chosen. This is the
// first thing to look at when the game starts up black on one machine and
// fine on another - it tells you whether the window and renderer are sane
// before any of the game's own drawing is involved.
void logVideoSetup(SDL_Window *window, SDL_Renderer *renderer, bool fullscreen)
{
    char msg[512];

    const char *videoDriver = SDL_GetCurrentVideoDriver();
    snprintf(msg, sizeof(msg), "Video driver:%s, displays:%d",
             videoDriver ? videoDriver : "(none)", SDL_GetNumVideoDisplays());
    Logger::Instance()->writeToLog(msg, "runlog.txt");

    int displayIndex = SDL_GetWindowDisplayIndex(window);
    SDL_DisplayMode mode;
    if (displayIndex >= 0 && SDL_GetCurrentDisplayMode(displayIndex, &mode) == 0) {
        snprintf(msg, sizeof(msg), "Display %d (%s) current mode:%dx%d @%dHz",
                 displayIndex, SDL_GetDisplayName(displayIndex) ? SDL_GetDisplayName(displayIndex) : "?",
                 mode.w, mode.h, mode.refresh_rate);
        Logger::Instance()->writeToLog(msg, "runlog.txt");
    }

    int winW = 0, winH = 0, drawW = 0, drawH = 0;
    SDL_GetWindowSize(window, &winW, &winH);
    SDL_GetRendererOutputSize(renderer, &drawW, &drawH);
    snprintf(msg, sizeof(msg), "Window:%dx%d (%s), renderer output:%dx%d",
             winW, winH, fullscreen ? "borderless fullscreen" : "windowed", drawW, drawH);
    Logger::Instance()->writeToLog(msg, "runlog.txt");

    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(renderer, &info) == 0) {
        snprintf(msg, sizeof(msg), "Render backend:%s%s", info.name,
                 (info.flags & SDL_RENDERER_ACCELERATED) ? " (accelerated)" : " (SOFTWARE - expect low framerate)");
        Logger::Instance()->writeToLog(msg, "runlog.txt");
    }
}

} // namespace

void Renderer::initRenderer(string windowCaption, bool wantFullscreen)
{
    char msg[512];

    // WATCH OUT: SDL_WINDOW_FULLSCREEN_DESKTOP, *not* SDL_WINDOW_FULLSCREEN.
    //
    // SDL_WINDOW_FULLSCREEN is exclusive fullscreen: SDL asks the monitor to
    // switch to a WINDOW_WIDTH x WINDOW_HEIGHT video mode. The old 1360x768
    // is a legacy mode that plenty of laptop panels, classroom monitors and
    // projectors simply don't have, and when the driver takes the switch
    // anyway you get a black screen - with the window never getting keyboard
    // focus, so Esc can't reach the game and it can't be closed either.
    //
    // FULLSCREEN_DESKTOP is a borderless window at the display's *existing*
    // resolution: no mode switch, nothing to fail, Alt+Tab behaves, and
    // WINDOW_WIDTH/HEIGHT below adopt whatever that resolution turns out to
    // be so the rest of the engine draws at the screen's native size.
    Uint32 windowFlags = wantFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;

    window = SDL_CreateWindow(windowCaption.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH, WINDOW_HEIGHT, windowFlags);

    // A driver that refuses even a borderless fullscreen window still gets a
    // windowed one rather than killing the program.
    if (window == NULL && wantFullscreen) {
        snprintf(msg, sizeof(msg), "Warning: fullscreen window failed (%s) - falling back to windowed %dx%d.",
                 SDL_GetError(), WINDOW_WIDTH, WINDOW_HEIGHT);
        Logger::Instance()->writeToLog(msg, "runlog.txt");
        wantFullscreen = false;
        window = SDL_CreateWindow(windowCaption.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    }
    if (window == NULL) {
        string failed = "SDL_CreateWindow failed:" + string(SDL_GetError());
        Logger::Instance()->writeToLog(failed, "runlog.txt");
        exit(1);
    }
    fullscreen = wantFullscreen;

    // Try the renderer we'd like, then progressively less demanding ones.
    // Some integrated GPUs, remote-desktop sessions and older drivers have no
    // accelerated renderer (or no vsync) at all, and the previous code
    // exit(1)'d on that instead of falling back to one that works.
    const struct { Uint32 flags; const char *what; } attempts[] = {
        { SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_PRESENTVSYNC, "accelerated + vsync" },
        { SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE,                              "accelerated, no vsync" },
        { SDL_RENDERER_TARGETTEXTURE,                                                        "any backend SDL offers" },
        { 0,                                                                                 "last resort" },
    };
    for (const auto &attempt : attempts) {
        renderer = SDL_CreateRenderer(window, -1, attempt.flags);
        if (renderer != NULL) {
            break;
        }
        snprintf(msg, sizeof(msg), "Warning: no renderer with %s (%s) - trying a simpler one.",
                 attempt.what, SDL_GetError());
        Logger::Instance()->writeToLog(msg, "runlog.txt");
    }
    if (renderer == NULL)
    {
        string failed = "SDL_CreateRenderer failed:" + string(SDL_GetError());
        Logger::Instance()->writeToLog(failed, "runlog.txt");
        exit(1);
    }

    // Adopt the real window size: everything else in the codebase draws
    // against WINDOW_WIDTH/HEIGHT and would otherwise keep assuming 1360x768
    // and draw into one corner of a bigger screen.
    refreshWindowSize();

    logVideoSetup(window, renderer, fullscreen);
}

SDL_Renderer *Renderer::getRenderer()
{
    return renderer;
}

SDL_Window *Renderer::getWindow()
{
    return window;
}

void Renderer::refreshWindowSize()
{
    if (window == NULL) {
        return;
    }
    // Renderer *output* size, not window size: they differ on a high-DPI
    // display, and it's the pixels we actually draw into that the camera
    // maths needs.
    int w = 0, h = 0;
    if (renderer != NULL && SDL_GetRendererOutputSize(renderer, &w, &h) == 0 && w > 0 && h > 0) {
        WINDOW_WIDTH  = w;
        WINDOW_HEIGHT = h;
        return;
    }
    SDL_GetWindowSize(window, &w, &h);
    if (w > 0 && h > 0) {
        WINDOW_WIDTH  = w;
        WINDOW_HEIGHT = h;
    }
}

bool Renderer::toggleFullscreen()
{
    if (window == NULL) {
        return fullscreen;
    }
    bool wanted = !fullscreen;
    Uint32 flags = wanted ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    if (SDL_SetWindowFullscreen(window, flags) != 0) {
        string msg = "Warning: could not toggle fullscreen:" + string(SDL_GetError());
        Logger::Instance()->writeToLog(msg, "runlog.txt");
        return fullscreen;
    }
    fullscreen = wanted;
    refreshWindowSize();
    return fullscreen;
}

void Renderer::closeRenderer()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}
