#ifndef __RENDERER_H
#define __RENDERER_H
#include <SDL2/SDL.h>
#include <string>

using namespace std;

// The render resolution, in pixels - referenced throughout Game.cpp and
// isoEngine.cpp for camera centring, zoom bounds and edge-scroll thresholds.
//
// WATCH OUT: these are runtime variables, not compile-time constants (they
// used to be #defines). initRenderer() overwrites them with the size the
// window *actually* got, which fullscreen means the display's own resolution
// - 1920x1080, 2560x1440, a projector's 1280x800... The 1360x768 below is
// only the windowed-mode default. So read them, never assume them: on a 4K
// screen the game draws a 3840x2160 view of the map, and because every bit
// of camera/zoom maths keys off these two values it follows along by itself.
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;

// FILE: SDL_Window/SDL_Renderer singleton.
// FITS IN: every draw call in the codebase goes through
// Renderer::Instance()->getRenderer(). IMPORTANT INVARIANT: call
// initRenderer() once (initSDL does this) before any getRenderer()/getWindow().
class Renderer
{
private:
	Renderer(){}; // Singleton private constructor
	static Renderer * s_pInstance;
	static SDL_Window *window;
    static SDL_Renderer *renderer;
    static bool fullscreen;

public:
	~Renderer(){ };
	static Renderer* Instance();
    // Summary: create the window and renderer, then set WINDOW_WIDTH/HEIGHT to
    // the size actually obtained.
    // Inputs: windowCaption - the window title; fullscreen - borderless
    //   fullscreen at the display's resolution (default), or a plain
    //   1360x768 window. main.cpp passes false for `--windowed`.
    // Errors: logs to logs/runlog.txt and exit(1)s if even a software
    //   renderer can't be created.
    void initRenderer(string windowCaption, bool fullscreen = true);
    SDL_Renderer *getRenderer();
    SDL_Window *getWindow();
    // Summary: swap between borderless fullscreen and windowed at runtime
    // (F11), keeping WINDOW_WIDTH/HEIGHT in step.
    // Output: true if the window is fullscreen afterwards. A driver that
    //   refuses the switch logs it and leaves the window as it was.
    bool toggleFullscreen();
    // Summary: re-read WINDOW_WIDTH/HEIGHT from the window. Call after
    // anything that can change its size - Game::processInput does this on
    // SDL_WINDOWEVENT_SIZE_CHANGED.
    void refreshWindowSize();
    void closeRenderer();
};

#endif // __RENDERER_H
