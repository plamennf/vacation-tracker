#include "pch.h"
#include "main.h"
#include "draw.h"
#include "os_specific.h"
#include "vacation.h"
#include "text_file_handler.h"

#include "shader_catalog.h"
#include "texture_catalog.h"

#include <stdio.h>

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

static void save_data();
static void load_data();

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
    globals.display_system = make_display_system(startup_window_width, startup_window_height, "Отпуски", true);
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

    load_data();
    
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

    save_data();
    destroy_fonts();
    
    return 0;
}

const int CURRENT_DATA_FILE_VERSION = 1;

static void save_data() {
    FILE *file = fopen("save.txt", "wb");
    if (!file) {
        log_error("Failed to open file 'save.txt' for writing.\n");
        return;
    }
    defer { fclose(file); };

    fprintf(file, "[%d] # Version number do not delete\n", CURRENT_DATA_FILE_VERSION);
    
    fprintf(file, "%d # Number of employees\n", all_employees.count);
    
    for (auto employee : all_employees) {
        fprintf(file, "%s # Employee name\n", employee->name);
        fprintf(file, "%d # Whether the employee is hidden or not\n", (int)employee->draw_all_vacations_on_hud);

        fprintf(file, "%d # Number of vacations of the current employee\n", employee->vacations.count);
        for (auto info : employee->vacations) {
            fprintf(file, "%d.%d.%d %d.%d.%d # StartDate EndDate\n",
                    info.from_day, info.from_month, info.from_year,
                    info.to_day, info.to_month, info.to_year);
        }
    }
}

static void load_data() {
    Text_File_Handler handler;
    handler.eat_spaces_before_line = false;
    handler.start_file("save", "save.txt", "save");
    if (handler.failed) return;

    char *line = handler.consume_next_line();
    int num_employees = atoi(line);
    all_employees.resize(num_employees);

    for (int i = 0; i < num_employees; i++) {
        all_employees[i] = new Employee();
        Employee *employee = all_employees[i];
        employee->has_vacation_that_overlaps = false;
        
        line = handler.consume_next_line();
        employee->name = copy_string(line);

        line = handler.consume_next_line();
        employee->draw_all_vacations_on_hud = (bool)atoi(line);

        line = handler.consume_next_line();
        int num_vacations = atoi(line);
        employee->vacations.resize(num_vacations);

        for (int j = 0; j < num_vacations; j++) {
            Vacation_Info *info = &employee->vacations[j];
            info->is_colliding = false;
            
            line = handler.consume_next_line();
            sscanf(line, "%d.%d.%d %d.%d.%d",
                   &info->from_day, &info->from_month, &info->from_year,
                   &info->to_day, &info->to_month, &info->to_year);
        }
    }
}
