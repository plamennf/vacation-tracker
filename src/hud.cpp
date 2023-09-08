#include "pch.h"
#include "main.h"
#include "draw.h"
#include "hud.h"

Button_Theme default_button_theme;

void init_hud_themes() {
    default_button_theme.bg_color         = Vector4(1, 10.0f/255.0f, 84.0f/255.0f, 1);
    default_button_theme.text_color       = Vector4(1, 1, 1, 1);
    default_button_theme.hovered_bg_color = Vector4(1, 71.0f/255.0f, 126.0f/255.0f, 1);
    default_button_theme.pressed_bg_color = Vector4(1, 92.0f/255.0f, 138.0f/255.0f, 1);
}

bool do_button(Dynamic_Font *font, char *text, int x, int y, int width, int height, Button_Theme theme) {    
    auto sys = globals.display_system;
    
    int mx, my;
    sys->get_mouse_pointer_position(&mx, &my);

    int offset_x = sys->offset_offscreen_to_back_buffer_x;
    int offset_y = sys->offset_offscreen_to_back_buffer_y;

    Vector4 color = theme.bg_color;
    bool was_pressed = false;
    if ((mx >= x + offset_x) && (mx <= x + offset_x + width) &&
        (my >= y + offset_y) && (my <= y + offset_y + height)) {
        if (was_key_just_released(MOUSE_BUTTON_LEFT)) was_pressed = true;
        
        if (is_key_down(MOUSE_BUTTON_LEFT) && !was_pressed) {
            color = theme.hovered_bg_color;
        } else {
            color = theme.pressed_bg_color;
        }
    }
    
    rendering_2d_right_handed();
    sys->set_shader(globals.shader_color);
    
    sys->immediate_begin();
    draw_quad(Vector2((float)x, (float)y), Vector2((float)(width), (float)(height)), color);
    sys->immediate_flush();

    int tx = x + ((width  - font->get_text_width(text)) / 2);
    int ty = y + ((height - font->character_height) / 2) + (font->y_offset_for_centering / 2);

    int offset = font->character_height / 10;
    if (offset > 0) {
        draw_text(font, text, tx+offset, ty-offset, Vector4(0, 0, 0, 1));
    }
    
    draw_text(font, text, tx, ty, theme.text_color);
    
    return was_pressed;
}
