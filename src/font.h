#pragma once

#define FONT_DIRECTORY "data/fonts"

struct Font_Page;
struct Font_Line;
struct Texture;
struct Bitmap;

struct Glyph_Data {
    int utf32;
    u32 glyph_index_within_font;

    s16 x0, y0;
    u32 width, height;

    s16 offset_x, offset_y;

    s16 ascent;
    s16 advance;

    Font_Page *page;
};

struct Font_Quad {
    Vector2 p0, p1, p2, p3;
    float u0, v0, u1, v1;

    Glyph_Data *glyph;
};

struct Dynamic_Font {
    char *name;
    struct FT_FaceRec_ *face;
    Hash_Table <int, Glyph_Data *> glyph_lookup;
    
    int character_height;
    int default_line_spacing;
    int max_ascender;
    int max_descender;
    int typical_ascender;
    int typical_descender;
    int em_width;
    int x_advance;

    int y_offset_for_centering;

    bool glyph_conversion_failed;
    u32 glyph_index_for_unknown_character;

    Array <Glyph_Data *> temporary_glyphs;
    Array <Font_Quad> current_quads;

    void load_font(int pixel_height);
    
    bool set_unknown_character(int utf32);
    
    int prepare_text(char *text);
    int get_text_width(char *s);
    Glyph_Data *find_or_create_glyph(int utf32);
    
    int convert_to_temporary_glyphs(char *s);
    void generate_quads_for_prepared_text(int x, int y);
};

struct Font_Page {
    Texture *texture;
    Bitmap *bitmap_data;

    s16 line_cursor_y;
    Array <Font_Line *> lines;

    bool dirty = false;
};

struct Font_Line {
    Font_Page *page;

    s16 bitmap_cursor_x;
    s16 bitmap_cursor_y;

    int height;
};

struct Code_Line {
    char *line;
    s64 tab_spaces;

    int num_colors;
    u8 *colors;
};

void init_fonts(int page_size_x = -1, int page_size_y = -1);
void destroy_fonts();
Dynamic_Font *get_font_at_size(char *name, int pixel_height);
