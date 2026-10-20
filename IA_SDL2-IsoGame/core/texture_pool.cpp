// FILE: TexturePool implementation - see texture_pool.hpp for the contract.
#include "texture_pool.hpp"
#include "logger.hpp"

// Strips the directory, keeping just the filename - see the WATCH OUT in
// texture_pool.hpp about basename collisions this causes.
static string GetOnlyFilename(string filenameAndPath)
{
    //get length of string
    int len = filenameAndPath.length();

    int index = 0;
    //loop the end to the string and go towards the beginning
    for(int i=len-1;i>=0;--i)
    {
        //if slash is found
        if((filenameAndPath[i] == '/') || (filenameAndPath[i] == '\\'))
        {
            //set the index to the position in the string one character before
            //the forward slash
            index = i+1;
            //break out of the for loop
            break;
        }
    }

    return filenameAndPath.substr (index,len-index);
}


TexturePool::TexturePool()
{
    //initialize number of textures to 0
    numTextures = 0;
}

TexturePool::~TexturePool()
{
    //loop through all the texture containers
    for(int i=0;i<numTextures;++i)
    {
        //if the texture in the texture container is allocated
        if(textures[i].texture != NULL){
            delete textures[i].texture;
        }
    }
    //free the allocated texture containers
    textures.clear();
}

void TexturePool::AddTexture(string filename)
{
    string msg;

    //allocate memory for the texture name
    textureContainerT newcontainer;
    newcontainer.name = GetOnlyFilename(filename);

    //allocate memory for the new texture
    newcontainer.texture = new Texture();

    // (`new` throws on failure in standard C++ rather than returning NULL, so
    // this check is only a safety net.)
    if(newcontainer.texture == NULL){
        //log it as an error
        msg = "Error in texturePoolAddTextureToTexturePool(): Could not allocate memory for the texture. Texture:"+filename+" not added to the pool";
        Logger::Instance()->writeToLog(msg,"runlog.txt");
        //return
        return;
    }
    //if the texture init failed
    if(newcontainer.texture->textureInit(0,NULL,SDL_FLIP_NONE) == 0)
    {
        //Log what went wrong
        msg = "WARNING: Texture:"+filename+" was NOT loaded to texture pool.";
        Logger::Instance()->writeToLog(msg,"runlog.txt");
        delete newcontainer.texture;
        //return out of the function
        return;
    }

    //if the texture loading failed
    if(newcontainer.texture->loadTexture(const_cast<char*>(filename.c_str()))==0)
    {
        //the texture load function will log the error so just return
        delete newcontainer.texture;
        return;
    }
    msg = "Texture:"+filename+" loaded to texture pool.";
    Logger::Instance()->writeToLog(msg,"runlog.txt");
    //add texture and increase number of textures in the pool
    textures.push_back(newcontainer);
    numTextures++;
}

Texture *TexturePool::GetTexture(string filename)
{
    //loop through the textures
    for(int i=0;i<numTextures;++i)
    {
        //if the texture was found
        if(textures[i].name == filename)
        {
            //return the texture pointer
            return textures[i].texture;
        }
    }
    //if the texture could not be found, log it as an error
    // (std::string rather than sprintf into a fixed buffer - see the WATCH
    // OUT in Texture::loadTexture for why that matters with long paths.)
    string msg = "Warning in texturePoolGetTextureFromPool(): Could not find texture:"+filename;
    Logger::Instance()->writeToLog(msg,"runlog.txt");
    //return NULL
    return NULL;
}
