// FILE: Animator implementation - see Animation.hpp for the full contracts.
#include "Animation.hpp"

void Animator::add(const std::string &name, const Animation &anim)
{
    anims_[name] = anim;
    if (current_.empty()) {
        current_ = name;
        frame_ = 0;
        elapsed_ = 0.0f;
    }
}

void Animator::play(const std::string &name)
{
    if (name == current_) {
        return;                 // keep the frame counter running
    }
    if (anims_.find(name) == anims_.end()) {
        return;                 // unknown animation: keep showing the current one
    }
    current_ = name;
    frame_ = 0;
    elapsed_ = 0.0f;
}

void Animator::update(float dt)
{
    auto it = anims_.find(current_);
    if (it == anims_.end()) {
        return;
    }
    const Animation &anim = it->second;
    int numFrames = (int)anim.frames.size();
    if (numFrames <= 1 || anim.frameSeconds <= 0.0f) {
        return;                 // static frame: nothing to advance
    }

    elapsed_ += dt;
    while (elapsed_ >= anim.frameSeconds) {
        elapsed_ -= anim.frameSeconds;
        frame_++;
        if (frame_ >= numFrames) {
            frame_ = anim.loop ? 0 : numFrames - 1;
        }
    }
}

bool Animator::has(const std::string &name) const
{
    return anims_.find(name) != anims_.end();
}

Texture *Animator::sheet() const
{
    auto it = anims_.find(current_);
    return (it == anims_.end()) ? nullptr : it->second.sheet;
}

Texture *Animator::shadowSheet() const
{
    auto it = anims_.find(current_);
    return (it == anims_.end()) ? nullptr : it->second.shadowSheet;
}

bool Animator::finished() const
{
    auto it = anims_.find(current_);
    if (it == anims_.end()) {
        return true;
    }
    const Animation &anim = it->second;
    if (anim.frames.empty()) {
        return true;    // nothing to play - don't block on it
    }
    if (anim.loop) {
        return false;
    }
    return frame_ >= (int)anim.frames.size() - 1;
}

const SDL_Rect *Animator::clip() const
{
    auto it = anims_.find(current_);
    if (it == anims_.end() || it->second.frames.empty()) {
        return nullptr;
    }
    int f = frame_;
    if (f < 0 || f >= (int)it->second.frames.size()) {
        f = 0;
    }
    return &it->second.frames[f];
}
