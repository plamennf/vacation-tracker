#include "pch.h"
#include "main.h"
#include "draw.h"
#include "hud.h"

Button_Theme default_button_theme;

static bool occlusion_enabled;
static int occlusion_x;
static int occlusion_y;
static int occlusion_width;
static int occlusion_height;

void init_hud_themes() {
    default_button_theme.bg_color         = Vector4(1, 10.0f/255.0f, 84.0f/255.0f, 1);
    default_button_theme.text_color       = Vector4(1, 1, 1, 1);
    default_button_theme.hovered_bg_color = Vector4(1, 71.0f/255.0f, 126.0f/255.0f, 1);
    default_button_theme.pressed_bg_color = Vector4(1, 92.0f/255.0f, 138.0f/255.0f, 1);
    default_button_theme.allow_right_clicks = false;
}

void hud_declare_occlusion(int x, int y, int width, int height) {
    occlusion_enabled = true;
    occlusion_x       = x;
    occlusion_y       = y;
    occlusion_width   = width;
    occlusion_height  = height;
}

void hud_remove_occlusion(int num_frames_to_wait) {
    occlusion_enabled = false;
}

Button_State do_button(Dynamic_Font *font, char *text, int x, int y, int width, int height, Button_Theme theme, bool bypasses_occlusion) {
    auto sys = globals.display_system;
    
    int mx, my;
    sys->get_mouse_pointer_position(&mx, &my);

    int offset_x = sys->offset_offscreen_to_back_buffer_x;
    int offset_y = sys->offset_offscreen_to_back_buffer_y;

    offset_y += (int)draw_y_offset_due_to_scrolling;
    
    Vector4 color = theme.bg_color;
    Button_State state = Button_State::NONE;
    
    if (!bypasses_occlusion && occlusion_enabled &&
        (mx >= occlusion_x + offset_x) && (mx <= occlusion_x + occlusion_width  + offset_x) &&
        (my >= occlusion_y + offset_y) && (my <= occlusion_y + occlusion_height + offset_y)) {
    } else {
        if ((mx >= x + offset_x) && (mx <= x + offset_x + width) &&
            (my >= y + offset_y) && (my <= y + offset_y + height)) {
            if (was_key_just_released(MOUSE_BUTTON_LEFT))  state = Button_State::LEFT_PRESSED;
            if (theme.allow_right_clicks) {
                if (was_key_just_released(MOUSE_BUTTON_RIGHT)) state = Button_State::RIGHT_PRESSED;
            }

            if (theme.allow_right_clicks) {
                if ((is_key_down(MOUSE_BUTTON_LEFT && state != Button_State::LEFT_PRESSED)) ||
                    (is_key_down(MOUSE_BUTTON_RIGHT && state != Button_State::RIGHT_PRESSED))) {
                    color = theme.pressed_bg_color;
                } else {
                    color = theme.hovered_bg_color;
                }
            } else {
                if (is_key_down(MOUSE_BUTTON_LEFT) && state != Button_State::LEFT_PRESSED) {
                    color = theme.pressed_bg_color;
                } else {
                    color = theme.hovered_bg_color;
                }
            }
        }
    }
        
    rendering_2d_right_handed_with_y_offset(draw_y_offset_due_to_scrolling);
    sys->set_shader(globals.shader_color);
    
    sys->immediate_begin();
    draw_quad(Vector2((float)x, (float)y), Vector2((float)width, (float)height), color);
    sys->immediate_flush();
    
    int tx = x + ((width  - font->get_text_width(text)) / 2);
    int ty = y + ((height - font->character_height) / 2) + (font->y_offset_for_centering / 2);
    
    int offset = font->character_height / 20;
    if (offset > 0) {
        draw_text(font, text, tx+offset, ty-offset, Vector4(0, 0, 0, 1));
    }
    
    draw_text(font, text, tx, ty, theme.text_color);
    
    return state;
}
