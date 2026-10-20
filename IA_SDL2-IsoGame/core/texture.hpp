#ifndef __TEXTURE_H_
#define __TEXTURE_H_
#include <SDL2/SDL.h>
#include <string>

using namespace std;

// utility function
void setupRect(SDL_Rect *rect,int x,int y,int w,int h);

// FILE: one loaded SDL_Texture + its draw state (clip rect, angle, flip).
// FITS IN: TexturePool owns these; nothing else constructs a Texture directly.
// IMPORTANT INVARIANT: call textureInit() then loadTexture() before
// textureRenderXYClipScale() - an unloaded Texture has a NULL SDL_Texture*
// and drawing with it is undefined behaviour (see TexturePool::AddTexture
// for the order this is normally done in).
class Texture
{
    public:
    Texture();
    ~Texture();

    private:
    double angle;
    SDL_RendererFlip fliptype;

    public:
    int width;  // TODO add getter and make it private
    int height; // TODO add getter and make it private
    SDL_Rect cliprect;    // TODO add getter and make it private
    SDL_Texture *texture = nullptr; // TODO add getter and make it private

    int loadTexture(string filename);
    int textureInit(double angle, SDL_Rect *cliprect, SDL_RendererFlip fliptype);
    void textureRenderXYClipScale(int x, int y, SDL_Rect *cliprect,float scale);

};

#endif // __TEXTURE_H_
