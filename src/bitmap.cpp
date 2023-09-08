#include "pch.h"
#include "bitmap.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool load_bitmap(Bitmap *bitmap, char *filepath) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    u8 *data = stbi_load(filepath, &width, &height, &channels, 4);
    if (!data) {
        log_error("Failed to load bitmap '%s'.\n", filepath);
        return false;
    }

    bitmap->width  = width;
    bitmap->height = height;

    bitmap->format = TEXTURE_FORMAT_RGBA8;
    bitmap->bytes_per_pixel = 4;

    bitmap->data = data;
    
    return true;
}

void free_bitmap(Bitmap *bitmap) {
    if (bitmap->data) {
        stbi_image_free(bitmap->data);
        bitmap->data = NULL;
    }
}

static int get_bytes_per_pixel(Texture_Format format) {
    switch (format) {
    case TEXTURE_FORMAT_RGBA8_NO_SRGB:
    case TEXTURE_FORMAT_RGBA8:
        return 4;

    case TEXTURE_FORMAT_R8:
        return 1;
    }

    assert(!"Invalid texture format");
    return 0;
}

void bitmap_alloc(Bitmap *bitmap, int w, int h, Texture_Format format) {
    bitmap->width  = w;
    bitmap->height = h;

    bitmap->format = format;
    bitmap->bytes_per_pixel = get_bytes_per_pixel(format);
    s64 size = (s64)w * (s64)h * (s64)bitmap->bytes_per_pixel;

    bitmap->data = new u8[size];
}
