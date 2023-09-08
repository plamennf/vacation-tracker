#include "pch.h"
#include "shader_catalog.h"
#include "main.h"
#include "os_specific.h"
#include "display_system.h"

#include <stdio.h>

Shader_Catalog::~Shader_Catalog() {
    for (auto shader : loaded_shaders) {
        delete shader;
    }
}

Shader *Shader_Catalog::get_by_name(char *name) {
    Shader **_shader = shader_lookup.find(name);
    if (_shader) return *_shader;

    char full_path[4096];
    snprintf(full_path, sizeof(full_path), "%s/%s.fx", SHADER_DIRECTORY, name);
    if (!os_file_exists(full_path)) {
        log_error("Unable to find file '%s.fx' in '%s'.\n", name, SHADER_DIRECTORY);
        return NULL;
    }

    auto sys = globals.display_system;
    
    Shader *shader = make_shader();
    bool success = sys->load_shader(shader, full_path);
    if (!success) {
        log_error("Unable to load shader '%s'.\n", full_path);
        delete shader;
        return NULL;
    }

    u64 modtime = 0;
    os_get_file_last_write_time(full_path, &modtime);

    shader->full_path = copy_string(full_path);
    shader->name = copy_string(name);
    shader->modtime = modtime;
    
    shader_lookup.add(name, shader);
    loaded_shaders.add(shader);
    
    return shader;
}

void Shader_Catalog::do_hotloading() {
    for (int i = 0; i < loaded_shaders.count; i++) {
        Shader *shader = loaded_shaders[i];

        u64 modtime = shader->modtime;
        bool success = os_get_file_last_write_time(shader->full_path, &modtime);
        if (!success) continue;

        if (shader->modtime != modtime) {
            shader->modtime = modtime;

            auto sys = globals.display_system;
            sys->load_shader(shader, shader->full_path);
        }
    }
}
