#pragma once

#define SHADER_DIRECTORY "data/shaders"

struct Shader;

struct Shader_Catalog {
    String_Hash_Table <Shader *> shader_lookup;
    Array <Shader *> loaded_shaders;

    ~Shader_Catalog();
    
    Shader *get_by_name(char *name);
    void do_hotloading();
};
