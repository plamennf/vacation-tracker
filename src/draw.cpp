#include "pch.h"
#include "main.h"
#include "draw.h"
#include "hud.h"
#include "os_specific.h"
#include "vacation.h"
#include "text_input.h"

#include "shader_catalog.h"
#include "texture_catalog.h"

#include <stdio.h>

static bool should_draw_right_click_options = false;
static Employee *currently_right_clicked_employee = NULL;
static int right_click_x, right_click_y, right_click_width, right_click_height;
static int right_click_prev_mx, right_click_prev_my;
static int right_click_prev_target_width, right_click_prev_target_height;

float draw_y_offset_due_to_scrolling = 0.0f;
static float bottom_y_after_drawing = 0.0f;

static float scroll_delta_speed = 20.0f;

enum Employee_Name_State {
    EMPLOYEE_NAME_FOR_ADDING,
    EMPLOYEE_NAME_FOR_RENAMING,
};

static Text_Input employee_name_text_input;
static bool should_draw_employee_name_text_input;
static Employee_Name_State employee_name_state;

static bool should_draw_employee_info_text_input;
static Text_Input vacation_info_from_text_input;
static Text_Input vacation_info_to_text_input;
static int currently_selected_text_input = 0; // 0 - from, 1 - to

static char *right_click_options[] = {
    "Премахни",
    "Преименувай",
    "Добави отпуска",
};

static int string_length_unicode(char *utf8) {
    if (!utf8) return 0;

    int length = 0;
    for (char *at = utf8; *at;) {
        int codepoint_byte_count = 0;
        int codepoint = get_codepoint(at, &codepoint_byte_count);

        length += 1;

        at += codepoint_byte_count;
    }
    return length;
}

static bool strings_match_unicode(char *a, char *b) {
    if (a == b) return true;
    if (!a || !b) return false;

    while (*a && *b) {
        int a_codepoint_byte_count = 0;
        int a_codepoint = get_codepoint(a, &a_codepoint_byte_count);
        
        int b_codepoint_byte_count = 0;
        int b_codepoint = get_codepoint(b, &b_codepoint_byte_count);

        if (a_codepoint != b_codepoint) return false;

        a += a_codepoint_byte_count;
        b += b_codepoint_byte_count;
    }

    return *a == 0 && *b == 0;
}

static char *select_longest_right_click_options_text() {
    char *longest = NULL;
    int longest_length = 0;
    
    for (auto option : right_click_options) {
        int option_length = string_length_unicode(option);
        if (option_length > longest_length) {
            longest_length = option_length;
            longest = option;
        }
    }

    return longest;
}

static void calculate_right_click_bounds(int mx, int my) {
    auto sys = globals.display_system;

    right_click_prev_mx = mx;
    right_click_prev_my = my;
    
    right_click_x = mx;
    right_click_y = my;
    
    int offset_x = sys->offset_offscreen_to_back_buffer_x;
    int offset_y = sys->offset_offscreen_to_back_buffer_y;

    offset_y += (int)draw_y_offset_due_to_scrolling;
    
    right_click_x -= offset_x;
    right_click_y -= offset_y;
    
    int font_size = (int)(0.025f * sys->offscreen_buffer->height);
    auto font = get_font_at_size("OpenSans-Regular", font_size);

    char *largest_text = select_longest_right_click_options_text();    
    
    int text_width = font->get_text_width(largest_text);
    
    int width  = text_width * 2;
    int height = (font->character_height * 2) * ArrayCount(right_click_options);

    int x0 = right_click_x;
    int y0 = right_click_y - height;
    
    right_click_x = x0;
    right_click_y = y0;
    
    right_click_width  = width;
    right_click_height = height;

    right_click_prev_target_width  = sys->display_width;
    right_click_prev_target_height = sys->display_height;

    hud_declare_occlusion(right_click_x, right_click_y, right_click_width, right_click_height);
}

static bool mouse_is_within_right_click_options() {
    int mx, my;
    auto sys = globals.display_system;
    sys->get_mouse_pointer_position(&mx, &my);

    int offset_x = sys->offset_offscreen_to_back_buffer_x;
    int offset_y = sys->offset_offscreen_to_back_buffer_y;

    offset_y += (int)draw_y_offset_due_to_scrolling;
    
    if ((mx >= right_click_x + offset_x) && (mx <= right_click_x + right_click_width  + offset_x) &&
        (my >= right_click_y + offset_y) && (my <= right_click_y + right_click_height + offset_y)) {
        return true;
    }

    return false;
}

static void enable_right_click_options(Employee *employee) {
    should_draw_right_click_options = true;
    currently_right_clicked_employee = employee;

    int mx, my;
    auto sys = globals.display_system;
    sys->get_mouse_pointer_position(&mx, &my);
    
    calculate_right_click_bounds(mx, my);
}

static void disable_right_click_options() {
    should_draw_right_click_options = false;
    hud_remove_occlusion();
}

static void enable_employee_name_text_input(Employee_Name_State state) {
    should_draw_employee_name_text_input = true;
    employee_name_text_input.reset();
    employee_name_text_input.activate();

    employee_name_state = state;
}

static void disable_employee_name_text_input() {
    should_draw_employee_name_text_input = false;
    employee_name_text_input.deactivate();
    hud_remove_occlusion();
}

static void enable_employee_info_text_input() {
    should_draw_employee_info_text_input = true;

    vacation_info_from_text_input.reset();
    vacation_info_from_text_input.activate();

    vacation_info_to_text_input.reset();
    //vacation_info_to_text_input.activate();
}

static void disable_employee_info_text_input() {
    should_draw_employee_info_text_input = false;

    vacation_info_from_text_input.deactivate();
    vacation_info_to_text_input.deactivate();
    
    hud_remove_occlusion();
}

static void resize_right_click_options() {
    if (!right_click_prev_target_width || !right_click_prev_target_height) return;
    
    auto sys = globals.display_system;
    
    float nmx = (float)right_click_prev_mx / (float)right_click_prev_target_width;;
    float nmy = (float)right_click_prev_my / (float)right_click_prev_target_height;

    int mx = (int)(nmx * sys->display_width);
    int my = (int)(nmy * sys->display_height);

    calculate_right_click_bounds(mx, my);
}

void handle_resizes() {
    if (should_draw_right_click_options && currently_right_clicked_employee) {
        resize_right_click_options();
    }
}

void handle_mouse_wheel_event(int num_ticks) {
    draw_y_offset_due_to_scrolling -= num_ticks * scroll_delta_speed;
    if (draw_y_offset_due_to_scrolling < 0.0f) draw_y_offset_due_to_scrolling = 0.0f;

    float bottom_border = -bottom_y_after_drawing;
    if (bottom_border < 0.0f) bottom_border = 0.0f;
    if (draw_y_offset_due_to_scrolling > bottom_border) draw_y_offset_due_to_scrolling = bottom_border;
}

void handle_event(Event event) {
    if (should_draw_employee_name_text_input) {
        employee_name_text_input.handle_event(event);
    }

    if (should_draw_employee_info_text_input) {
        if (currently_selected_text_input == 0) {
            vacation_info_from_text_input.handle_event(event);
        } else if (currently_selected_text_input == 1) {
            vacation_info_to_text_input.handle_event(event);
        }
    }
}

void init_shaders() {
    auto sys = globals.display_system;
    
    globals.shader_color = globals.shader_catalog->get_by_name("color");
    globals.shader_texture = globals.shader_catalog->get_by_name("texture");
    globals.shader_text = globals.shader_catalog->get_by_name("text");

    init_hud_themes();

    // Calculate right_click_height before calculate_right_click_bounds is called,
    // so that we can properly clamp draw_y_offset_due_to_scrolling.
    int font_size = (int)(0.025f * sys->offscreen_buffer->height);
    auto font = get_font_at_size("OpenSans-Regular", font_size);
    right_click_height = (font->character_height * 2) * ArrayCount(right_click_options);
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

void rendering_2d_right_handed_with_y_offset(float y_offset) {
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

    m.identity();

    m._24 = y_offset;
    
    sys->world_to_view_matrix = m;
    
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

void draw_quad_with_border(Vector2 position, Vector2 size, Vector4 color, float border_size, Vector4 border_color) {
    int border_size_in_pixels = (int)(border_size * size.y);
    Vector2 border_size_in_pixels_v((float)border_size_in_pixels, (float)border_size_in_pixels);
    
    Vector2 border_size_v   = size     + (border_size_in_pixels_v*2.0f);
    Vector2 border_position = position - border_size_in_pixels_v;
    
    draw_quad(border_position, border_size_v, border_color);
    draw_quad(position, size, color);
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

static char *get_longest_employee_name() {
    char *longest = NULL;
    int longest_length = 0;
    
    for (auto employee : all_employees) {
        int name_length = string_length_unicode(employee->name);
        if (name_length > longest_length) {
            longest_length = name_length;
            longest = employee->name;
        }
    }
    
    return longest;
}

static void draw_hud() {
    auto sys = globals.display_system;
    
    int start_y = sys->target_height;

    rendering_2d_right_handed_with_y_offset(draw_y_offset_due_to_scrolling);
    
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
            draw_text(font, text, x+offset, y-offset, Vector4(1, 1, 1, 1));
        }
        draw_text(font, text, x, y, Vector4(0, 0, 0, 1));

        start_y -= font->character_height * 2;
        start_y -= font->character_height / 2;
    }

    //
    // Add employee button
    //
    {
        int font_size = (int)(0.025f * sys->target_height);
        auto font = get_font_at_size("OpenSans-Regular", font_size);
        
        char *text = "Добави служител";
        //int offset = (int)(0.025f * sys->target_height);
        int offset = 0;

        int width  = font->get_text_width(text) * 2;
        int height = font->character_height * 2;
        
        int x = sys->target_width  - offset - width;
        int y = sys->target_height - offset - height;
        
        if (do_button(font, text, x, y, width, height, default_button_theme) == Button_State::LEFT_PRESSED) {
            log("Adding employee.\n");
            enable_employee_name_text_input(EMPLOYEE_NAME_FOR_ADDING);
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
        
        char *longest_name = get_longest_employee_name();
        
        int offset = font->character_height / 40;
        for (auto employee : all_employees) {
            char *text = employee->name;
            
            int text_width = font->get_text_width(longest_name);

            int width  = text_width * 2;
            int height = font->character_height * 2;

            int x0 = x;
            int y0 = y - height;

            auto theme = default_button_theme;
            theme.bg_color         = Vector4(56.0f/255.0f,  176.0f/255.0f, 0, 1);
            theme.hovered_bg_color = Vector4(0,             128.0f/255.0f, 0, 1);
            theme.pressed_bg_color = Vector4(0,             114.0f/255.0f, 0, 1);
            theme.allow_right_clicks = true;
            
            auto state = do_button(font, text, x0, y0, width, height, theme);
            if (state == Button_State::LEFT_PRESSED) {
                employee->draw_all_vacations_on_hud = !employee->draw_all_vacations_on_hud;
                disable_right_click_options();
            } else if (state == Button_State::RIGHT_PRESSED) {
                enable_right_click_options(employee);
            }
            
            y -= height;

            if (!employee->draw_all_vacations_on_hud) continue;

            for (auto info : employee->vacations) {
                char *text = mprintf("От %d.%d.%dг. до %d.%d.%dг.", info.from_day, info.from_month, info.from_year, info.to_day, info.to_month, info.to_year);
                defer { delete [] text; };

                y0 = y - font->character_height;
                
                draw_text(font, text, x0, y0, Vector4(0, 0, 0, 1));

                y -= font->character_height - font->typical_descender;
            }
        }

        bottom_y_after_drawing = (float)(y - right_click_height);

        /*
        if (y > 0) {
            bottom_y_after_drawing = (float)(y - right_click_height);
        } else {
            bottom_y_after_drawing = 0.0f;
        }
        */
    }

    if (should_draw_right_click_options && currently_right_clicked_employee) {
        auto employee = currently_right_clicked_employee;
        
        int font_size = (int)(0.025f * sys->target_height);
        auto font = get_font_at_size("OpenSans-Regular", font_size);

        int x0 = right_click_x;
        int y0 = right_click_y;

        int height = right_click_height / ArrayCount(right_click_options);
        
        for (int i = ArrayCount(right_click_options) - 1; i >= 0; i--) {
            char *option = right_click_options[i];

            auto theme = default_button_theme;
            bool bypass_occlusion = !should_draw_employee_name_text_input;
            auto state = do_button(font, option, x0, y0, right_click_width, height, theme, bypass_occlusion);
            if (state == Button_State::LEFT_PRESSED) {
                disable_right_click_options();
                
                if (strings_match_unicode(option, "Премахни")) {
                    int employee_index = all_employees.find(currently_right_clicked_employee);
                    if (employee_index != -1) {
                        all_employees.ordered_remove_by_index(employee_index);
                    }
                } else if (strings_match_unicode(option, "Преименувай")) {
                    enable_employee_name_text_input(EMPLOYEE_NAME_FOR_RENAMING);

                    auto employee = currently_right_clicked_employee;
                    if (employee) {
                        employee_name_text_input.add_text(employee->name);
                    }
                } else if (strings_match_unicode(option, "Добави отпуска")) {
                    //auto info = employee->add_vacation_info(5, 5, 6, 6, 2023);
                    enable_employee_info_text_input();
                }
            }

            y0 += font->character_height * 2;
        }
    }
    
    if (is_key_pressed(MOUSE_BUTTON_LEFT) && should_draw_right_click_options && !mouse_is_within_right_click_options()) {
        disable_right_click_options();
    }

    if (should_draw_employee_name_text_input) {
        assert(!should_draw_employee_info_text_input);
        
        int font_size = (int)(0.05f * sys->target_height);
        auto font = get_font_at_size("OpenSans-Regular", font_size);
        
        int width  = (int)(0.5f * sys->target_width);
        int height = font->character_height;

        int x = (sys->target_width  - width)  / 2;
        int y = (sys->target_height - height) / 2;

        hud_declare_occlusion(x, y, width, height);
        
        rendering_2d_right_handed();
        sys->set_shader(globals.shader_color);
        
        Vector4 color(0.1f, 0.1f, 0.9f, 1);
        Vector4 border_color(0.011f, 0.01f, 0.96, 1);
        
        sys->immediate_begin();
        draw_quad_with_border(Vector2((float)x, (float)y), Vector2((float)width, (float)height), color, 0.1f, border_color);
        sys->immediate_flush();
        
        employee_name_text_input.draw(font, x, y, width, Vector4(1, 1, 1, 1));
        
        if (was_key_just_released(KEY_ENTER)) {
            auto state = employee_name_state;
            disable_employee_name_text_input();

            if (state == EMPLOYEE_NAME_FOR_ADDING) {
                Employee *employee = add_employee(employee_name_text_input.get_result()); // @Leak
                //auto info = employee->add_vacation_info(5, 5, 6, 6, 2023);
            } else if (state == EMPLOYEE_NAME_FOR_RENAMING) {
                auto employee = currently_right_clicked_employee;
                if (employee) {
                    if (employee->name) {
                        delete [] employee->name;
                        employee->name = NULL;
                    }

                    employee->name = employee_name_text_input.get_result();
                }
            }
        }
    }

    if (should_draw_employee_info_text_input) {
        assert(!should_draw_employee_name_text_input);

        if (is_key_pressed(KEY_TAB)) {
            if (currently_selected_text_input == 0) {
                currently_selected_text_input = 1;
                vacation_info_from_text_input.deactivate();
                vacation_info_to_text_input.activate();
            } else if (currently_selected_text_input == 1) {
                currently_selected_text_input = 0;
                vacation_info_to_text_input.deactivate();
                vacation_info_from_text_input.activate();
            }
        }
        
        int font_size = (int)(0.05f * sys->target_height);
        auto font = get_font_at_size("OpenSans-Regular", font_size);

        int from_length = font->get_text_width("от");
        int to_length = font->get_text_width("до");

        int text_length = Max(from_length, to_length);
        
        int width  = (int)(0.5f * sys->target_width);
        int height = font->character_height;

        int pad = (int)(0.1f * height) * 2;
        int line_width = width + text_length + pad;
        Vector4 bg_color(0, 0, 0, 1);
        
        int x = (sys->target_width  - width)  / 2;
        int y = (sys->target_height - height) / 2;

        hud_declare_occlusion(x - text_length - pad, y, width + text_length + pad/2, 3*height + pad);
        
        rendering_2d_right_handed();
        sys->set_shader(globals.shader_color);
        
        Vector4 color(0.1f, 0.1f, 0.9f, 1);
        Vector4 border_color(0.011f, 0.01f, 0.96, 1);
        
        sys->immediate_begin();
        draw_quad(Vector2((float)(x - text_length - pad), (float)(y - pad/2)), Vector2((float)(line_width + pad/2), (float)(3*height + pad)), bg_color);
        draw_quad_with_border(Vector2((float)x, (float)y), Vector2((float)width, (float)height), color, 0.1f, border_color);
        draw_quad_with_border(Vector2((float)x, (float)(y + height)), Vector2((float)width, (float)height), color, 0.1f, border_color);
        //draw_quad_with_border(Vector2((float)x, (float)(y + 2*height)), Vector2((float)width, (float)height), color, 0.1f, border_color);
        sys->immediate_flush();

        vacation_info_from_text_input.draw(font, x, y+height, width, Vector4(1, 1, 1, 1));
        vacation_info_to_text_input.draw(font, x, y, width, Vector4(1, 1, 1, 1));

        x -= text_length;
        x -= pad;

        int yy = y + font->y_offset_for_centering / 2;
        
        draw_text(font, "От", x, yy+height, Vector4(1, 1, 1, 1));
        draw_text(font, "До", x, yy, Vector4(1, 1, 1, 1));

        char *text = "Добави";
        
        //x += text_length + pad;
        //x += width / 2;

        width  = font->get_text_width(text);
        height = font->character_height;
        
        auto state = do_button(font, text, x, y + height*2 + pad/2, width, height, default_button_theme, true);
        if (state == Button_State::LEFT_PRESSED) {
            char *from_text = vacation_info_from_text_input.get_result();
            char *to_text   = vacation_info_to_text_input.get_result();

            // TODO: Check if they are valid

            Employee *employee = NULL; // It is here so that goto works
            
            bool success = true;
            
            int from_day, from_month, from_year;
            int res = sscanf(from_text, "%d.%d.%d", &from_day, &from_month, &from_year);
            if (res != 3) {
                success = false;
                goto error_from;
            }

            int to_day, to_month, to_year;
            res = sscanf(to_text, "%d.%d.%d", &to_day, &to_month, &to_year);
            if (res != 3) {
                success = false;
                goto error_to;
            }

            employee = currently_right_clicked_employee;
            if (employee) {
                employee->add_vacation_info(from_day, from_month, from_year, to_day, to_month, to_year);
            }

            disable_employee_info_text_input();
            
    error_to:
            if (!success) {
                os_show_message_box("Грешка", "Невалидна до дата\nФормат: ДД.ММ.ГГГГ", true);
                success = true; // Already handled the error, no need to show it again.
            }
            
    error_from:
            if (!success) {
                os_show_message_box("Грешка", "Невалидна от дата\nФормат: ДД.ММ.ГГГГ", true);
            }
        }
    }
}

void draw_game_view() {
    auto sys = globals.display_system;

    auto dummy_texture = globals.texture_catalog->get_by_name("white");
    sys->set_texture(0, dummy_texture); // To avoid a d3d11 warning.
    
    sys->set_render_targets(sys->offscreen_buffer, NULL);
    sys->clear_render_target(1, 224.0f/255.0f, 228.0f/255.0f, 1);

    draw_hud();
    
    resolve_to_back_buffer();
}
