Prebuilt SDL2 libraries for Windows / Visual Studio (64-bit), bundled so the
project builds out of the box with no extra installs.

  SDL2        2.32.10  https://github.com/libsdl-org/SDL/releases/tag/release-2.32.10
  SDL2_image  2.8.12   https://github.com/libsdl-org/SDL_image/releases/tag/release-2.8.12

Taken unmodified from the official "-devel-...-VC.zip" downloads. The headers
were placed under include/SDL2/ so that the code's #include <SDL2/SDL.h> works
the same on Windows and Linux. Both libraries are zlib-licensed (see LICENSE-*.txt).

Linux/macOS users don't need this folder: install the system SDL2 and
SDL2_image development packages instead and build with CMake.
