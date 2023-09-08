#pragma once

#define TEXTURE_DIRECTORY "data/textures"

struct Texture;

struct Texture_Catalog {
    String_Hash_Table <Texture *> texture_lookup;
    Array <Texture *> loaded_textures;

    ~Texture_Catalog();
    
    Texture *get_by_name(char *name);
    void do_hotloading();
};
