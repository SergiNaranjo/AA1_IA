cmake_minimum_required(VERSION 3.16)
project(isogame CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# --- Dependencies: SDL2 and SDL2_image ---------------------------------------
# Windows / Visual Studio (MSVC): use the prebuilt 64-bit SDL2 + SDL2_image
# bundled in external/SDL2, so nothing has to be installed. (Most people on
# Windows should just open IsoGame.sln; this branch is for "Open Folder".)
if(MSVC)
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "The bundled SDL2 in external/SDL2 is 64-bit only - configure for x64.")
    endif()
    set(SDL2_BUNDLED_DIR "${CMAKE_SOURCE_DIR}/external/SDL2")
    set(SDL2_BUNDLED_LIBDIR "${SDL2_BUNDLED_DIR}/lib/x64")

    add_library(SDL2::SDL2 SHARED IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        IMPORTED_IMPLIB "${SDL2_BUNDLED_LIBDIR}/SDL2.lib"
        IMPORTED_LOCATION "${SDL2_BUNDLED_LIBDIR}/SDL2.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_BUNDLED_DIR}/include"
    )
    add_library(SDL2::SDL2main STATIC IMPORTED)
    set_target_properties(SDL2::SDL2main PROPERTIES
        IMPORTED_LOCATION "${SDL2_BUNDLED_LIBDIR}/SDL2main.lib"
    )
    add_library(SDL2_image::SDL2_image SHARED IMPORTED)
    set_target_properties(SDL2_image::SDL2_image PROPERTIES
        IMPORTED_IMPLIB "${SDL2_BUNDLED_LIBDIR}/SDL2_image.lib"
        IMPORTED_LOCATION "${SDL2_BUNDLED_LIBDIR}/SDL2_image.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_BUNDLED_DIR}/include"
    )
else()

    # SDL2's CMake interface differs by distribution:
    #  - vcpkg and recent SDL2
    #    releases ship a CONFIG package with imported targets (SDL2::SDL2,
    #    SDL2::SDL2main).
    #  - Debian/Ubuntu's libsdl2-dev ships an older autotools-generated config
    #    that only sets SDL2_INCLUDE_DIRS/SDL2_LIBRARIES - no targets.
    # Support both by synthesizing the SDL2::SDL2 target when it isn't provided,
    # so the rest of this file only ever deals with targets.
    find_package(SDL2 REQUIRED)

    if(NOT TARGET SDL2::SDL2)
        add_library(SDL2::SDL2 INTERFACE IMPORTED)
        set_target_properties(SDL2::SDL2 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${SDL2_LIBRARIES}"
        )
    endif()

    # SDL2_image: vcpkg ships a CONFIG package (SDL2_image::SDL2_image); Debian/
    # Ubuntu's libsdl2-image-dev doesn't, so fall back to locating the library
    # directly and reusing SDL2's include dir (both SDL.h and SDL_image.h live
    # under the same <prefix>/include/SDL2 there).
    find_package(SDL2_image CONFIG QUIET)
    if(NOT TARGET SDL2_image::SDL2_image)
        find_library(SDL2_IMAGE_LIBRARY NAMES SDL2_image)
        if(NOT SDL2_IMAGE_LIBRARY)
            message(FATAL_ERROR "SDL2_image not found. Install libsdl2-image-dev (or equivalent).")
        endif()
        add_library(SDL2_image::SDL2_image INTERFACE IMPORTED)
        set_target_properties(SDL2_image::SDL2_image PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${SDL2_IMAGE_LIBRARY}"
        )
    endif()

endif()

# --- Sources -----------------------------------------------------------------
# core/ : SDL utility modules (kept from the original project).
set(CORE_SOURCES
    core/deltatimer.cpp
    core/initclose.cpp
    core/logger.cpp
    core/renderer.cpp
    core/texture.cpp
    core/texture_pool.cpp
)

# iso/ : isometric rendering engine.
set(ISO_SOURCES
    iso/isoEngine.cpp
    iso/isoMap.cpp
)

# engine/ : object-oriented, AI-facing framework.
set(ENGINE_SOURCES
    engine/World.cpp
    engine/Agent.cpp
    engine/Game.cpp
    engine/DebugDraw.cpp
    engine/Animation.cpp
    engine/MapLoader.cpp
)

add_executable(isogame
    main.cpp
    ${CORE_SOURCES}
    ${ISO_SOURCES}
    ${ENGINE_SOURCES}
)

target_include_directories(isogame PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# SDL2main (the WinMain shim) only exists as a target on platforms that need
# it (Windows); harmless to skip elsewhere. Must be linked before SDL2.
set(ISOGAME_SDL_LIBS "")
if(TARGET SDL2::SDL2main)
    list(APPEND ISOGAME_SDL_LIBS SDL2::SDL2main)
endif()
list(APPEND ISOGAME_SDL_LIBS SDL2::SDL2 SDL2_image::SDL2_image)

target_link_libraries(isogame PRIVATE ${ISOGAME_SDL_LIBS})

if(MSVC)
    target_compile_definitions(isogame PRIVATE _CRT_SECURE_NO_WARNINGS)
    # Copy SDL2.dll / SDL2_image.dll next to isogame.exe, and debug from the
    # project root so the game finds data/ and logs/.
    add_custom_command(TARGET isogame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_BUNDLED_LIBDIR}/SDL2.dll"
            "${SDL2_BUNDLED_LIBDIR}/SDL2_image.dll"
            "$<TARGET_FILE_DIR:isogame>"
    )
    set_target_properties(isogame PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()
