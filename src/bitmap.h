#pragma once

enum Texture_Format {
    TEXTURE_FORMAT_UNKNOWN,
    TEXTURE_FORMAT_RGBA8,
    TEXTURE_FORMAT_RGBA8_NO_SRGB,
    TEXTURE_FORMAT_R8,
};

struct Bitmap {
    int width  = 0;
    int height = 0;

    Texture_Format format = TEXTURE_FORMAT_UNKNOWN;
    int bytes_per_pixel = 0;

    u8 *data = NULL;
};

bool load_bitmap(Bitmap *bitmap, char *filepath);
void free_bitmap(Bitmap *bitmap);

void bitmap_alloc(Bitmap *bitmap, int w, int h, Texture_Format format);
