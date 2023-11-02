#include "pch.h"
#include "main.h"
#include "text_input.h"
#include "os_specific.h"
#include "draw.h"

void Text_Input::init() {
    num_characters = 0;
    cursor = 0;
    
    initted = true;
}

void Text_Input::handle_event(Event event) {
    if (!initted) init();
    
    if (event.type == EVENT_TYPE_TEXT_INPUT) {
        int key = event.utf32;

        if (num_characters < MAX_BUFFER_SIZE) {
            //for (int i = num_characters - 1; i >= cursor; i--) {
            for (int i = num_characters; i >= cursor; i--) {
                input_buffer[i] = input_buffer[i - 1];
            }
            
            input_buffer[cursor] = key;
            num_characters += 1;
            cursor += 1;
        }
        
        double now = globals.time_info.current_real_world_time;
        last_keypress_time = now;
    } else if (event.type == EVENT_TYPE_KEYBOARD) {
        int key = event.key_code;
        bool pressed = event.key_pressed;
        
        if (pressed) {
            if (key == KEY_BACKSPACE) {
                if (num_characters > 0 && cursor > 0) {
                    for (int i = cursor-1; i < num_characters-1; i++) {
                        input_buffer[i] = input_buffer[i + 1];
                    }
                    input_buffer[num_characters-1] = 0;
                    num_characters--;
                    cursor -= 1;
                }
            } else if (key == KEY_LEFT) {
                cursor -= 1;
                if (cursor < 0) cursor = 0;
            } else if (key == KEY_RIGHT) {
                cursor += 1;
                if (cursor > num_characters) cursor = num_characters;
            }
        }
    }

    if (num_characters < 0) num_characters = 0;

    if (cursor < 0) cursor = 0;
    if (cursor > num_characters) cursor = num_characters;
}

void Text_Input::activate() {
    active = true;
}

void Text_Input::deactivate() {
    active = false;
}

bool Text_Input::is_active() {
    return active;
}

// @ToDo @CleanUp: Find a better solution
// @ToDo @CleanUp: Find a better solution
// @ToDo @CleanUp: Find a better solution
char *Text_Input::get_result(int final_character) {
    if (final_character == -1) final_character = num_characters;
    
    int num_characters_to_alloc = final_character * 4;
    if (num_characters_to_alloc <= 0) num_characters_to_alloc = 1;
    char *result = new char[num_characters_to_alloc];
    int result_count = 0;
    
    for (int i = 0; i < final_character; i++) {
        char utf8[4];
        int text_count = get_utf8(utf8, input_buffer[i]);
        
        for (int j = 0; j < text_count; j++) {
            result[result_count + j] = utf8[j];
        }

        result_count += text_count;
    }

    result[result_count] = 0;
    
    return result;
}

void Text_Input::reset() {
    deactivate();
    
    memset(input_buffer, 0, sizeof(input_buffer));
    num_characters = 0;
    cursor = 0;
}

void Text_Input::add_text(char *text) {
    if (!initted) init();
    
    for (char *at = text; *at;) {
        int codepoint_byte_count;
        int codepoint = get_codepoint(at, &codepoint_byte_count);
        
        if (num_characters != 0 && cursor != 0) {
            for (int i = num_characters; i >= cursor; i--) {
                input_buffer[i] = input_buffer[i - 1];
            }
        }
        
        input_buffer[cursor] = codepoint;
        num_characters += 1;
        cursor += 1;
        
        at += codepoint_byte_count;
    }
}

Vector4 Text_Input::get_cursor_color(Vector4 non_white) {
    double now = globals.time_info.current_real_world_time;

    Vector4 white(1, 1, 1, 1);

    double t = cos((now - last_keypress_time) * 3.0);
    t *= t;

    return lerp(non_white, white, (float)t);
}

void Text_Input::draw(Dynamic_Font *font, int text_x, int text_y, int entry_width, Vector4 text_color) {
    if (!initted) init();
    
    auto sys = globals.display_system;

    // @ToDo @CleanUp: Find a better solution
    // @ToDo @CleanUp: Find a better solution
    // @ToDo @CleanUp: Find a better solution
    char *s = get_result();
    defer { delete s; };
    
    auto b = (int)(font->character_height * 0.05f);
    if (b < 2) b = 2;

    int font_centering_offset = (font->y_offset_for_centering / 2);
    text_y += font_centering_offset;
    
    Vector4 bg_color(0.05f, 0.05f, 0.05f, 0.9f);
    draw_text(font, s, text_x+b, text_y-b, bg_color);
    draw_text(font, s, text_x,   text_y,   text_color);

    Vector4 cursor_color = get_cursor_color(Vector4(1, 0, 1, 1));

    char *text_to_left_of_cursor = get_result(cursor);
    int width = font->get_text_width(text_to_left_of_cursor);
    int cursor_x = text_x + width;

    //
    // Draw the cursor
    //
    if (active) {
        int cw = (int)(0.001f * sys->target_width);
        int ch = font->character_height;
        
        int x0 = text_x + width;
        int y0 = text_y - font_centering_offset;
        int x1 = x0 + cw;
        int y1 = y0 + ch;
        
        rendering_2d_right_handed();
        sys->set_shader(globals.shader_color);
        
        sys->immediate_begin();
        draw_quad(Vector2((float)x0, (float)y0), Vector2((float)(x1-x0), (float)(y1-y0)), cursor_color);
        sys->immediate_flush();
    }
}
