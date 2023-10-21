#include "pch.h"

#include "display_system.h"
#include "bitmap.h"

Display_System::Display_System() {
    offscreen_buffer = NULL;
}

Display_System::~Display_System() {
    if (offscreen_buffer) delete offscreen_buffer;
}

bool Display_System::load_texture(Texture *texture, char *filepath) {
    Bitmap bitmap;
    if (!load_bitmap(&bitmap, filepath)) {
        log_error("Failed to load bitmap '%s'.\n", filepath);
        return false;
    }
    defer { free_bitmap(&bitmap); };
    load_texture_from_bitmap(texture, bitmap);
    return true;
}

void Display_System::resize_render_targets() {
    int width = 0, height = 0;
    if (maintain_aspect_ratio) {
        float w = 0.0f, h = 0.0f;
        if (aspect_ratio > desired_aspect_ratio) {
            h = (float)display_height;
            w = h * desired_aspect_ratio;
        } else {
            w = (float)display_width;
            h = w / desired_aspect_ratio;
        }

        if (w < 1.0f) w = 1.0f;
        if (h < 1.0f) h = 1.0f;

        width  = (int)floor(w);
        height = (int)floor(h);
    } else {
        width  = display_width;
        height = display_height;
    }
    
    if (!width || !height) return;
    
    if (offscreen_buffer) {
        delete offscreen_buffer;
    }
    offscreen_buffer = create_rendertarget(TEXTURE_FORMAT_RGBA8, width, height);

    offset_offscreen_to_back_buffer_x = (display_width  - width)  / 2;
    offset_offscreen_to_back_buffer_y = (display_height - height) / 2;

    if (resize_callback) {
        resize_callback();
    }
}
