#ifndef ENGINE_ANIMATION_HPP
#define ENGINE_ANIMATION_HPP
#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <map>
#include "../core/texture.hpp"

// FILE: sprite-sheet animation data (Animation) + playback (Animator).
// FITS IN: rendering-only - Game owns one Animator per agent and picks which
// name to play() based on agent state; Agent/World know nothing about this.
// EXTEND: register a new Animation via add(), then have Game::updateAnimations
// call play() with its name when the matching agent state is reached.

// A single named animation: a strip of frames on one sprite sheet.
struct Animation
{
    Texture              *sheet = nullptr;  // the sprite sheet the frames live on
    // Optional separate shadow sheet, sharing the same frame grid/clip rects
    // as `sheet` (e.g. the x180p pack's Body/Shadow layer pair) - drawn under
    // the body at the same position when present.
    Texture              *shadowSheet = nullptr;
    std::vector<SDL_Rect> frames;           // clip rectangle per frame
    float                 frameSeconds = 0.06f;  // how long each frame shows
    bool                  loop = true;
};

// Plays named animations and advances frames over time.
//
// This is a rendering-side helper (it holds textures), used by the Game layer.
// Agents stay free of it — the Game decides which animation an agent shows
// based on the agent's velocity/heading.
class Animator
{
public:
    // IMPORTANT INVARIANT: add() every name before calling play(name) with
    // it - play() on an unregistered name is a silent no-op (see below), not
    // an error, so a typo'd animation name just keeps showing the old one.
    void add(const std::string &name, const Animation &anim);

    // Switch to a named animation. No-op if already playing it (so the frame
    // counter keeps running); resets to frame 0 when the animation changes.
    // WATCH OUT: also a silent no-op if `name` was never add()-ed.
    void play(const std::string &name);

    void update(float dt);

    bool             has(const std::string &name) const;
    Texture         *sheet() const;         // current sheet, or nullptr
    Texture         *shadowSheet() const;   // current shadow sheet, or nullptr
    const SDL_Rect  *clip() const;          // current frame's clip rect, or nullptr

    // True once a non-looping animation has reached (and is holding on) its
    // last frame. Always false for a looping animation, or one still mid-way.
    bool finished() const;

private:
    std::map<std::string, Animation> anims_;
    std::string current_;
    int         frame_   = 0;
    float       elapsed_ = 0.0f;
};

#endif // ENGINE_ANIMATION_HPP
