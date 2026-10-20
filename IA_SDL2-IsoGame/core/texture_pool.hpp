#ifndef TEXTURE_POOL_H_
#define TEXTURE_POOL_H_
#include <vector>
#include <string>
#include <SDL2/SDL.h>
#include "texture.hpp"

using namespace std;

typedef struct textureContainerT
{
    Texture *texture;      //pointer to the texture object
    string name;           //name of the texture
}textureContainerT;

// FILE: loads/owns every Texture the game uses, keyed by filename (path
// stripped - see GetOnlyFilename in texture_pool.cpp).
// FITS IN: Game owns one TexturePool; AddTexture once per asset in
// Game::init, GetTexture everywhere a sheet/sprite is drawn.
// WATCH OUT: keyed by basename only, not full path - "a/foo.png" and
// "b/foo.png" collide; GetTexture silently returns whichever loaded first.
class TexturePool
{
    public:
    TexturePool();
    ~TexturePool();

    private:
    int numTextures;                //number of textures
    vector<textureContainerT> textures;    //the textures container

    public:
    void AddTexture(string filename);
    Texture *GetTexture(string filename);

};

#endif // TEXTURE_POOL_H_
