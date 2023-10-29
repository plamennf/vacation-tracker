#include "pch.h"
#include "main.h"
#include "draw.h"
#include "os_specific.h"

#include "shader_catalog.h"
#include "texture_catalog.h"

struct Key_State {
    bool is_down;
    bool was_down;
    bool changed;
};

static Key_State key_states[NUM_KEY_CODES];

Globals globals;

bool is_key_down(int key_code) {
    return key_states[key_code].is_down;
}

bool is_key_pressed(int key_code) {
    return key_states[key_code].is_down && key_states[key_code].changed;
}

bool was_key_just_released(int key_code) {
    return key_states[key_code].was_down && !key_states[key_code].is_down;
}

static void update_time() {
    double now = os_get_time();
    double delta = now - globals.time_info.last_time;
    globals.time_info.last_time = now;

    globals.time_info.current_dt = delta;
    globals.time_info.current_real_world_time += delta;
}

int main(int argc, char **argv) {
    os_init_colors_and_utf8();

    {
        char *path = os_get_path_of_running_executable();
        defer { delete [] path; };
        
        char *slash = strrchr(path, '/');
        path[slash - path] = 0;

        log("setcwd: %s\n", path);
        
        os_set_current_working_directory(path);
    }
    
    int startup_window_width  = 1600;
    int startup_window_height = 900;
    globals.display_system = make_display_system(startup_window_width, startup_window_height, "A Game", true);
    defer { delete globals.display_system; };
    globals.display_system->maintain_aspect_ratio = true;
    globals.display_system->desired_aspect_ratio = 16.0f / 9.0f;
    globals.display_system->resize_render_targets();

    globals.display_system->resize_callback = handle_resizes;
    
    globals.shader_catalog = new Shader_Catalog();
    defer { delete globals.shader_catalog; };

    globals.texture_catalog = new Texture_Catalog();
    defer { delete globals.texture_catalog; };
    
    init_shaders();

    globals.time_info.last_time = os_get_time();
    
    while (!globals.should_quit_game) {
        auto sys = globals.display_system;

        update_time();
        
        for (int i = 0; i < ArrayCount(key_states); i++) {
            auto ks = &key_states[i];
            ks->was_down = ks->is_down;
            ks->changed = false;
        }
        sys->update_window_events();
        for (auto event : sys->events_this_frame) {
            switch (event.type) {
                case EVENT_TYPE_QUIT:
                    globals.should_quit_game = true;
                    break;

                case EVENT_TYPE_KEYBOARD: {
                    auto ks     = &key_states[event.key_code];
                    ks->changed = ks->is_down != event.key_pressed;
                    ks->is_down = event.key_pressed;
                
                    break;
                }

                case EVENT_TYPE_MOUSE_WHEEL: {
                    int num_ticks = event.wheel_delta / event.typical_wheel_delta;
                    if (num_ticks) {
                        handle_mouse_wheel_event(num_ticks);
                    }
                } break;
            }
            handle_event(event);
        }
        
        draw_game_view();
        sys->swap_buffers();
    }

    destroy_fonts();
    
    return 0;
}
