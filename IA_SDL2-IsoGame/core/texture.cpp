// FILE: Texture implementation - see texture.hpp for the contract.
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "renderer.hpp"
#include "texture.hpp"
#include "logger.hpp"


// utility function
void setupRect(SDL_Rect *rect,int x,int y,int w,int h)
{
    rect->x = x;
    rect->y = y;
    rect->w = w;
    rect->h = h;
}


Texture::Texture()
{
}

Texture::~Texture()
{
        //if the texture is allocated
        if(texture != NULL){
            SDL_DestroyTexture(texture);
        }
}


int Texture::loadTexture(string filename)
{
    // WATCH OUT: these messages are built with std::string, not sprintf into
    // a fixed char buffer. A sprite-sheet path plus an SDL error message runs
    // to ~270 characters, which overflowed the 200-byte buffer this function
    // used to use - so the one code path that reports a failed texture load
    // corrupted the stack instead, turning a clear log line into a crash
    // exactly when something was already wrong.
    SDL_Surface *tmpSurface = IMG_Load(filename.c_str());

    if(tmpSurface == NULL){
        string msg = "Error in loadTexture(): Could not load image:"+filename+"! SDL_image Error:"+string(IMG_GetError());
        Logger::Instance()->writeToLog(msg,"runlog.txt");
        return 0;
    }
    else{
        texture = SDL_CreateTextureFromSurface(Renderer::Instance()->getRenderer(),tmpSurface);

        if(texture == NULL){
            // SDL_GetError, not IMG_GetError: it's SDL_CreateTextureFromSurface
            // that failed here, which is what you hit when the graphics card
            // runs out of memory part-way through loading.
            string msg = "Error in loadTexture(): Could not create texture for:"+filename+"! SDL Error:"+string(SDL_GetError());
            Logger::Instance()->writeToLog(msg,"runlog.txt");
            SDL_FreeSurface(tmpSurface);
            return 0;
        }
        else{
            width = tmpSurface->w;
            height = tmpSurface->h;
        }
        SDL_FreeSurface(tmpSurface);
        return 1;
    }
    return 0;
}

int Texture::textureInit(double _angle, SDL_Rect *_cliprect, SDL_RendererFlip _fliptype)
{
    angle = _angle;
    fliptype = _fliptype;

    if(_cliprect!=NULL){
        cliprect = *_cliprect;
    }
    else{
        setupRect(&cliprect,0,0,100,100);
    }

    return 1;
}

// WATCH OUT: despite the name, _scale only scales width/height below - the
// diffx/diffy maths cancels out so the drawn quad's x/y end up exactly _x/_y
// unscaled. Callers (e.g. Game::worldToScreen) must already pass final,
// post-zoom screen pixel coordinates; don't expect this to scale a position
// around the origin the way _scale scales the sprite's size.
void Texture::textureRenderXYClipScale(int _x, int _y, SDL_Rect *_cliprect, float _scale)
{
    float w,h;
    float diffx,diffy;
    SDL_Rect quad;
    w=(float)width*_scale;
    h=(float)height*_scale;

    diffx = (_x*_scale) - _x;
    diffy = (_y*_scale) - _y;

    setupRect(&quad,(_x*_scale)-diffx,(_y*_scale)-diffy,w,h);

    if(_cliprect!=NULL)
    {
        cliprect = *_cliprect;

        quad.w = (int)cliprect.w*_scale;
        quad.h = (int)cliprect.h*_scale;

        //if the current scale is larger or smaller than 1.0
        if(_scale <1.0 || _scale >1.0){
            //increase the width and height of the quad to not get
            //rendering artifacts
            quad.h +=1;
            quad.w +=1;
        }
        //Center point is passed in as NULL. If/When a function is needed to change center point on a sprite, we'll add one.
        SDL_RenderCopyEx(Renderer::Instance()->getRenderer(),texture,&cliprect,&quad,angle,NULL,fliptype);
    }
    //if the cliprect is NULL
    else
    {
        //Center point is passed in as NULL. If/When a function is needed to change center point on a sprite, we'll add one.
        //Draw without clip rectangle
        SDL_RenderCopyEx(Renderer::Instance()->getRenderer(),texture,NULL,&quad,angle,NULL,fliptype);
    }
}
