#include "pch.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "font.h"
#include "display_system.h"
#include "main.h"
#include "os_specific.h"

static FT_Library ft_library;
static bool fonts_initted;

static int page_size_x = 2048;
static int page_size_y = 1024;

static Array <Dynamic_Font *> dynamic_fonts;
static Array <Font_Page *> font_pages;

static Memory_Arena glyph_and_line_arena;

static inline int FT_ROUND(int x) {
    if (x >= 0) return (x + 0x1f) >> 6;
    return -(((-x) + 0x1f) >> 6);
}

static Font_Line *find_line_within_page(Font_Page *page, int width, int height) {
    auto bitmap = page->bitmap_data;

    for (auto it : page->lines) {
        if (it->height < height) continue; // Line too short!
        if (((it->height * 7) / 10) > height) continue; // Line too tall!

        if (bitmap->width - it->bitmap_cursor_x < width) continue; // No room at end of line!

        return it; // Found one!
    }

    // If there's not enough room to start a new line, bail.
    auto height_remaining = bitmap->height - page->line_cursor_y;
    if (height_remaining < height) return NULL;

    // Or if for some reason the page is too narrow for the character...
    // In this case, starting a new line would not help!
    if (bitmap->width < width) return NULL;

    // Start a new line... With some extra space for expansion if we have room.
    auto desired_height = (height * 11) / 10;

    if (desired_height > height_remaining) desired_height = height_remaining;

    Font_Line *line = (Font_Line *)glyph_and_line_arena.get(sizeof(Font_Line));
    if (!line) return NULL;

    line->page = page;
    line->bitmap_cursor_x = 0;
    line->bitmap_cursor_y = page->line_cursor_y;
    line->height = desired_height;

    page->lines.add(line);

    page->line_cursor_y += (s16)desired_height;

    return line;
}

static Font_Page *make_font_page() {
    auto page = new Font_Page();

    auto bitmap = new Bitmap();
    bitmap_alloc(bitmap, page_size_x, page_size_y, TEXTURE_FORMAT_R8);
    page->bitmap_data = bitmap;

    page->texture = make_texture();

    font_pages.add(page);
    return page;
}

static Font_Line *get_font_line(int width, int height) {
    for (auto page : font_pages) {
        auto line = find_line_within_page(page, width, height);
        if (line) return line;
    }

    auto page = make_font_page();
    auto line = find_line_within_page(page, width, height);
    assert(line);

    return line;
}

static void copy_glyph_to_bitmap(FT_Face face, Glyph_Data *data) {
    auto b = &face->glyph->bitmap;

    data->width    = b->width;
    data->height   = b->rows;
    data->advance  = (s16)(face->glyph->advance.x >> 6);
    data->offset_x = (s16)face->glyph->bitmap_left;
    data->offset_y = (s16)face->glyph->bitmap_top;

    auto metrics = &face->glyph->metrics;
    data->ascent = (s16)(metrics->horiBearingY >> 6);

    auto font_line = get_font_line(b->width, b->rows);

    s16 dest_x = font_line->bitmap_cursor_x;
    s16 dest_y = font_line->bitmap_cursor_y;

    data->x0   = dest_x;
    data->y0   = dest_y;
    data->page = font_line->page;

    auto bitmap = font_line->page->bitmap_data;

    s32 rows  = (s32)b->rows;
    s32 width = (s32)b->width;
    for (int j = 0; j < rows; j++) {
        for (int i = 0; i < width; i++) {
            auto dest_pixel = bitmap->data + ((dest_y + j) * bitmap->width + (dest_x + i));
            *dest_pixel = b->buffer[(rows - 1 - j) * b->pitch + i];
        }
    }

    font_line->bitmap_cursor_x += (s16)b->width;
    font_line->page->dirty = true;
}

Glyph_Data *Dynamic_Font::find_or_create_glyph(int utf32) {
    auto _data = glyph_lookup.find(utf32);
    if (_data) return *_data;

    if (utf32 == '\t') utf32 = '→';
    if (utf32 == '\n') utf32 = '¶';

    auto glyph_index = FT_Get_Char_Index(face, utf32);
    if (!glyph_index) {
        log_error("Unable to find a glyph in font '%s' for utf32 character %d.\n", name, utf32);
        glyph_index = glyph_index_for_unknown_character;
    }
    auto error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    assert(!error);
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LCD);

    auto data = (Glyph_Data *)glyph_and_line_arena.get(sizeof(Glyph_Data));
    data->utf32 = utf32;
    data->glyph_index_within_font = glyph_index;

    copy_glyph_to_bitmap(face, data);

    glyph_lookup.add(utf32, data);

    return data;
}

void init_fonts(int _page_size_x, int _page_size_y) {
    assert(!fonts_initted);
    
    if ((_page_size_x >= 0) || (_page_size_y >= 0)) {
        assert(_page_size_x >= 64);
        assert(_page_size_y >= 64);
        page_size_x = _page_size_x;
        page_size_y = _page_size_y;
    }

    fonts_initted = true;

    auto error = FT_Init_FreeType(&ft_library);
    assert(!error);

    glyph_and_line_arena.init(Megabytes(4)); // Should be enough space
}

void destroy_fonts() {
    for (auto page : font_pages) {
        if (!page) continue;
        if (!page->texture) continue;
        
        delete page->texture;
    }
}

static bool is_latin(int utf32) {
    if (utf32 > 0x24F) { // 0x24F is the end of Latin Extended-B
        if ((utf32 >= 0x2000) && (utf32 <= 0x218F)) {  // General punctuation, currency symbols, number forms, etc.
        } else {
            return false;
        }
    }

    return true;
}

static void ensure_fonts_are_initted() {
    if (!fonts_initted) init_fonts();
}

int Dynamic_Font::convert_to_temporary_glyphs(char *s) {
    glyph_conversion_failed = false;
    temporary_glyphs.count = 0;

    if (!s) return 0;

    bool use_kerning = FT_HAS_KERNING(face);
    u32 prev_glyph = 0;

    int width_in_pixels = 0;

    for (char *t = s; *t;) {
        int utf32_size;
        int utf32 = get_codepoint(t, &utf32_size);
        auto glyph = find_or_create_glyph(utf32);

        if (glyph) {
            temporary_glyphs.add(glyph);

            width_in_pixels += glyph->advance;

            if (use_kerning && prev_glyph) {
                FT_Vector delta;
                auto error = FT_Get_Kerning(face, prev_glyph, glyph->glyph_index_within_font, FT_KERNING_DEFAULT, &delta);
                if (!error) width_in_pixels += (delta.x >> 6);
            }

            if (glyph->glyph_index_within_font == 0) glyph_conversion_failed = true;
        }

        t += utf32_size;
        prev_glyph = glyph->glyph_index_within_font;
    }

    return width_in_pixels;
}

bool Dynamic_Font::set_unknown_character(int utf32) {
    auto index = FT_Get_Char_Index(face, utf32);
    if (!index) return false;

    glyph_index_for_unknown_character = index;
    return true;
}

void Dynamic_Font::load_font(int pixel_height) {
    auto success = FT_Set_Pixel_Sizes(face, 0, pixel_height);
    character_height = pixel_height;

    float y_scale_font_to_pixels = face->size->metrics.y_scale / (64.0f * 65536.0f);
    default_line_spacing = (int)floor(y_scale_font_to_pixels * face->height + 0.5f);
    max_ascender  = (int)floor(y_scale_font_to_pixels * face->bbox.yMax + 0.5f);
    max_descender = (int)floor(y_scale_font_to_pixels * face->bbox.yMin + 0.5f);

    auto glyph_index = FT_Get_Char_Index(face, 'm');
    if (glyph_index) {
        FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        y_offset_for_centering = (int)(0.5f * FT_ROUND(face->glyph->metrics.horiBearingY) + 0.5f);
    }

    glyph_index = FT_Get_Char_Index(face, 'M');
    if (glyph_index) {
        FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        em_width  = FT_ROUND(face->glyph->metrics.width);
        x_advance = FT_ROUND(face->glyph->metrics.horiAdvance);
    }

    glyph_index = FT_Get_Char_Index(face, 'T');
    if (glyph_index) {
        FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        typical_ascender = FT_ROUND(face->glyph->metrics.horiBearingY);
    }

    glyph_index = FT_Get_Char_Index(face, 'g');
    if (glyph_index) {
        FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        typical_descender = FT_ROUND(face->glyph->metrics.horiBearingY - face->glyph->metrics.height);
    }

    auto error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    assert(!error);

    {
        auto success = set_unknown_character(0xfffd); // REPLACEMENT_CHARACTER
        if (!success) success = set_unknown_character(0x2022); // BULLET
        if (!success) success = set_unknown_character('?');
        if (!success) {
            log_error("Unable to set unknown character for font '%s'.\n", name);
        }
    }

    dynamic_fonts.add(this);
}

Dynamic_Font *get_font_at_size(char *name, int pixel_height) {
    ensure_fonts_are_initted();

    for (auto it : dynamic_fonts) {
        if (it->character_height != pixel_height) continue;
        if (!strings_match(it->name, name)) continue;

        return it;
    }

    char *extensions[] = {
        "ttf",
        "otf",
    };
    
    char full_path[4096];
    bool file_exists = false;
    for (int i = 0; i < ArrayCount(extensions); i++) {
        snprintf(full_path, sizeof(full_path), "%s/%s.%s", FONT_DIRECTORY, name, extensions[i]);
        if (os_file_exists(full_path)) {
            file_exists = true;
            break;
        }
    }
    
    if (!file_exists) {
        log_error("Unable to find file '%s' in '%s'.\n", name, FONT_DIRECTORY);
        return NULL;
    }

    s64 file_size;
    char *file_data = os_read_entire_file(full_path, &file_size);
    if (!file_data) {
        log_error("Failed to read file '%s'.\n", full_path);
        return NULL;
    }
    
    // Create a new font face for Dynamic_Font rather than sharing one between fonts.
    // The reason is because we don't want to keep changing the size every time we want to
    // do anything and worry whether another Dynamic_Font has changed the size
    FT_Face face;
    auto error = FT_New_Memory_Face(ft_library, (const FT_Byte *)file_data, (int)file_size, 0, &face);
    if (error == FT_Err_Unknown_File_Format ) {
        log_error("Error: font file format not supported: '%s'\n", name);
        return NULL;
    }
    if (error) {
        log_error("Error while loading font '%s': %d", name, error);
        return NULL;
    }

    auto result = new Dynamic_Font();
    result->name = copy_string(name);
    result->face = face;
    result->load_font(pixel_height);
    return result;
}

int Dynamic_Font::prepare_text(char *text) {
    int width = convert_to_temporary_glyphs(text);
    return width;
}

int Dynamic_Font::get_text_width(char *s) {
    if (!s) return 0;

    bool use_kerning = FT_HAS_KERNING(face);
    u32 prev_glyph = 0;

    int width_in_pixels = 0;

    for (char *t = s; *t;) {
        int utf32_size;
        int utf32 = get_codepoint(t, &utf32_size);
        auto glyph = find_or_create_glyph(utf32);

        if (glyph) {
            width_in_pixels += glyph->advance;

            if (use_kerning && prev_glyph) {
                FT_Vector delta;
                auto error = FT_Get_Kerning(face, prev_glyph, glyph->glyph_index_within_font, FT_KERNING_DEFAULT, &delta);
                if (!error) width_in_pixels += (delta.x >> 6);
            }

            if (glyph->glyph_index_within_font == 0) glyph_conversion_failed = true;
        }

        t += utf32_size;
        prev_glyph = glyph->glyph_index_within_font;
    }

    return width_in_pixels;    
}

void Dynamic_Font::generate_quads_for_prepared_text(int x, int y) {
    current_quads.count = 0;
    current_quads.reserve(temporary_glyphs.count);

    bool use_kerning = FT_HAS_KERNING(face);

    float sx = (float)x;
    float sy = (float)y;

    u32 prev_glyph = 0;

    for (auto info : temporary_glyphs) {
        if (!info->page) continue;

        if (use_kerning && prev_glyph) {
            FT_Vector delta;
            auto error = FT_Get_Kerning(face, prev_glyph, info->glyph_index_within_font, FT_KERNING_DEFAULT, &delta);
            if (!error) {
                sx += (float)(delta.x >> 6);
            } else {
                log_error("Couldn't get kerning for glyphs %d, %d\n", prev_glyph, info->glyph_index_within_font);
            }
        }

        auto sx1 = sx  + (float)info->offset_x;
        auto sx2 = sx1 + (float)info->width;

        auto sy2 = sy  + (float)info->ascent;
        auto sy1 = sy2 - (float)info->height;

        Font_Quad quad;
        quad.glyph = info;
        quad.p0.x  = sx1;
        quad.p1.x  = sx2;
        quad.p2.x  = sx2;
        quad.p3.x  = sx1;
        quad.p0.y  = sy1;
        quad.p1.y  = sy1;
        quad.p2.y  = sy2;
        quad.p3.y  = sy2;

        auto width  = info->page->bitmap_data->width;
        auto height = info->page->bitmap_data->height;

        quad.u0 = info->x0 / (float)width;
        quad.u1 = ((float)info->x0 + info->width) / width;
        quad.v0 = info->y0 / (float)height;
        quad.v1 = ((float)info->y0 + info->height) / height;

        current_quads.add(quad);
        sx += (float)info->advance;

        prev_glyph = info->glyph_index_within_font;
    }
}
