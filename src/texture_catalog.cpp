#include "pch.h"
#include "texture_catalog.h"
#include "main.h"
#include "os_specific.h"
#include "display_system.h"

#include <stdio.h>

Texture_Catalog::~Texture_Catalog() {
    for (auto texture : loaded_textures) {
        delete texture;
    }
}

Texture *Texture_Catalog::get_by_name(char *name) {
    Texture **_texture = texture_lookup.find(name);
    if (_texture) return *_texture;

    char *extensions[] = {
        "png",
        "jpg",
        "bmp",
    };
    
    char full_path[4096];
    bool file_exists = false;
    for (int i = 0; i < ArrayCount(extensions); i++) {
        snprintf(full_path, sizeof(full_path), "%s/%s.%s", TEXTURE_DIRECTORY, name, extensions[i]);
        if (os_file_exists(full_path)) {
            file_exists = true;
            break;
        }
    }
    
    if (!file_exists) {
        log_error("Unable to find file '%s' in '%s'.\n", name, TEXTURE_DIRECTORY);
        return NULL;
    }

    auto sys = globals.display_system;
    
    Texture *texture = make_texture();
    bool success = sys->load_texture(texture, full_path);
    if (!success) {
        log_error("Unable to load texture '%s'.\n", full_path);
        delete texture;
        return NULL;
    }

    u64 modtime = 0;
    os_get_file_last_write_time(full_path, &modtime);

    texture->full_path = copy_string(full_path);
    texture->name = copy_string(name);
    texture->modtime = modtime;
    
    texture_lookup.add(name, texture);
    loaded_textures.add(texture);
    
    return texture;
}

void Texture_Catalog::do_hotloading() {
    for (int i = 0; i < loaded_textures.count; i++) {
        Texture *texture = loaded_textures[i];

        u64 modtime = texture->modtime;
        bool success = os_get_file_last_write_time(texture->full_path, &modtime);
        if (!success) continue;

        if (texture->modtime != modtime) {
            texture->modtime = modtime;

            auto sys = globals.display_system;
            sys->load_texture(texture, texture->full_path);
        }
    }
}
