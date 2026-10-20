#ifndef __INIT_CLOSE_H_
#define __INIT_CLOSE_H_

// FILE: SDL/SDL_image startup + shutdown, called once each from main().
// FITS IN: initSDL must run before any Renderer/Texture use; closeDownSDL
// tears down the same singletons. Errors: initSDL logs and exit(1)s on
// failure - it does not return an error code to the caller.
//
// `fullscreen` chooses borderless fullscreen at the display's own resolution
// (the default) or a plain 1360x768 window. main.cpp passes false when the
// program is started with `--windowed`, which is the fallback for a machine
// whose graphics driver misbehaves fullscreen.
void initSDL(char *windowName, bool fullscreen = true);

void closeDownSDL();

#endif // __INIT_CLOSE_H_
