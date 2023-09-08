#include "pch.h"
#include "main.h"
#include "draw.h"
#include "hud.h"
#include "os_specific.h"
#include "vacation.h"

#include "shader_catalog.h"
#include "texture_catalog.h"

#include <stdio.h>

void init_shaders() {
    globals.shader_color = globals.shader_catalog->get_by_name("color");
    globals.shader_texture = globals.shader_catalog->get_by_name("texture");
    globals.shader_text = globals.shader_catalog->get_by_name("text");

    init_hud_themes();
}

void rendering_2d_right_handed() {
    auto sys = globals.display_system;

    float w = (float)sys->target_width;
    if (w < 1.0f) w = 1.0f;
    float h = (float)sys->target_height;
    if (h < 1.0f) h = 1.0f;

    Matrix4 m;
    m.identity();

    m._11 = 2.0f/w;
    m._22 = 2.0f/h;
    m._14 = -1.0f;
    m._24 = -1.0f;

    sys->view_to_proj_matrix = m;
    sys->world_to_view_matrix.identity();
    sys->object_to_world_matrix.identity();
    
    sys->refresh_transform();
}

void resolve_to_back_buffer() {
    auto sys = globals.display_system;
    
    sys->set_render_targets(sys->back_buffer, NULL);
    sys->clear_render_target(0, 0, 0, 1);

    rendering_2d_right_handed();
    sys->set_shader(globals.shader_texture);

    sys->set_texture(0, sys->offscreen_buffer);

    int width  = sys->offscreen_buffer->width;
    int height = sys->offscreen_buffer->height;
    int x      = (sys->display_width  - width)  / 2;
    int y      = (sys->display_height - height) / 2;
    
    Vector2 position((float)x, (float)y);
    Vector2 size((float)width, (float)height);

    Vector2 p0 = position;
    Vector2 p1(position.x + size.x, position.y);
    Vector2 p2 = position + size;
    Vector2 p3(position.x, position.y+size.y);

    // Y-Flipped uvs because it is direct3d11.
    Vector2 uv0(0, 1);
    Vector2 uv1(1, 1);
    Vector2 uv2(1, 0);
    Vector2 uv3(0, 0);
    
    Vector4 color(1, 1, 1, 1);
    
    sys->immediate_begin();
    sys->immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, color);
    sys->immediate_flush();
}

void draw_quad(Vector2 position, Vector2 size, Vector4 color) {
    Vector2 p0 = position;
    Vector2 p1(position.x + size.x, position.y);
    Vector2 p2 = position + size;
    Vector2 p3(position.x, position.y+size.y);

    auto sys = globals.display_system;
    sys->immediate_quad(p0, p1, p2, p3, color);
}

static void draw_generated_quads(Dynamic_Font *font, Vector4 color) {
    auto sys = globals.display_system;
    
    sys->set_shader(globals.shader_text);

    Texture *last_texture = NULL;
    sys->immediate_begin();
    for (auto quad : font->current_quads) {
        auto page = quad.glyph->page;
        auto map = page->texture;

        if (page->dirty) {
            page->dirty = false;
            sys->load_texture_from_bitmap(map, *page->bitmap_data);
        }

        if (map != last_texture) {
            sys->immediate_flush();
            sys->set_texture(0, map);
            last_texture = map;
        }

        Vector2 p1 = quad.p0 + (quad.p1 - quad.p0) / 3;
        Vector2 p2 = quad.p3 + (quad.p2 - quad.p3) / 3;

        Vector2 uv0(quad.u0, quad.v0);
        Vector2 uv1(quad.u1, quad.v0);
        Vector2 uv2(quad.u1, quad.v1);
        Vector2 uv3(quad.u0, quad.v1);
        
        sys->immediate_quad(quad.p0, p1, p2, quad.p3, uv0, uv1, uv2, uv3, color);
    }
    sys->immediate_flush();
}

static void draw_prepared_text(Dynamic_Font *font, int x, int y, Vector4 color) {
    font->generate_quads_for_prepared_text(x, y);
    draw_generated_quads(font, color);
}

void draw_text(Dynamic_Font *font, char *text, int x, int y, Vector4 color) {
    font->prepare_text(text);
    draw_prepared_text(font, x, y, color);
}

static void draw_hud() {
    auto sys = globals.display_system;

    int start_y = sys->target_height;
    
    //
    // Draw time
    //
    {
        System_Time time = os_get_local_time();
        char text[4096];
        snprintf(text, sizeof(text), "%.2d:%.2d:%.2d", time.hour, time.minute, time.second);
        
        Vector4 text_color(0, 0, 0, 1);

        int font_size = (int)(0.025f * sys->target_height);
        auto font = get_font_at_size("OpenSans-Regular", font_size);
        int x = 0;
        int y = sys->target_height - font->character_height;

        int offset = font->character_height / 20;
        if (offset) {
            draw_text(font, text, x+offset, y-offset, Vector4(0, 0, 0, 1));
        }
        draw_text(font, text, x, y, Vector4(1, 1, 1, 1));

        start_y -= font->character_height * 2;
        start_y -= font->character_height / 2;
    }

    int font_size = (int)(0.025f * sys->target_height);
    auto font = get_font_at_size("OpenSans-Regular", font_size);

    //
    // Add employee button
    //
    {
        char *text = "Add employee";
        //int offset = (int)(0.025f * sys->target_height);
        int offset = 0;

        int width  = font->get_text_width(text) * 2;
        int height = font->character_height * 2;
        
        int x = sys->target_width  - offset - width;
        int y = sys->target_height - offset - height;
        
        if (do_button(font, text, x, y, width, height, default_button_theme)) {
            log("Adding employee.\n");

            Employee *employee = add_employee("Надя Любомирова Цветкова", Vector4(0, 0, 1, 1));
        }
    }
    
    //
    // Draw all employees
    //
    {
        int font_size = (int)(0.025f * sys->target_height);
        auto font = get_font_at_size("OpenSans-Regular", font_size);

        //int pad = (int)(0.0025f * sys->target_height);
        int pad = 0;
        
        int x = pad;
        int y = start_y;
        
        int offset = font->character_height / 20;
        for (auto employee : all_employees) {
            char *text = employee->name;
            
            int text_width = font->get_text_width(text);

            int width  = text_width * 2;
            int height = font->character_height * 2;

            int x0 = x;
            int y0 = y - height;
            
            sys->set_shader(globals.shader_color);
            
            sys->immediate_begin();
            draw_quad(Vector2((float)x0, (float)y0), Vector2((float)width, (float)height), employee->color);
            sys->immediate_flush();

            int tx = x0 + ((width  - font->get_text_width(text)) / 2);
            int ty = y0 + ((height - font->character_height) / 2) + (font->y_offset_for_centering / 2);
            
            if (offset) {
                draw_text(font, text, tx+offset, ty-offset, Vector4(0, 0, 0, 1));
            }
            draw_text(font, text, tx, ty, Vector4(1, 1, 1, 1));
            y -= font->character_height;
        }
    }
}

void draw_game_view() {
    auto sys = globals.display_system;

    auto dummy_texture = globals.texture_catalog->get_by_name("white");
    sys->set_texture(0, dummy_texture); // To avoid a d3d11 warning.
    
    sys->set_render_targets(sys->offscreen_buffer, NULL);
    sys->clear_render_target(1, 224.0f/255.0f, 228.0f/255.0f, 1);
    
    /*
    //
    // Draw background
    //
    rendering_2d_right_handed();
    sys->set_shader(globals.shader_texture);
    
    auto texture = globals.texture_catalog->get_by_name("background");
    sys->set_texture(0, texture);

    Vector2 pos(0, 0);
    Vector2 size((float)sys->target_width, (float)sys->target_height);
    
    sys->immediate_begin();
    draw_quad(pos, size, Vector4(1, 1, 1, 1));
    sys->immediate_flush();
    */

    draw_hud();
    
    resolve_to_back_buffer();
}
