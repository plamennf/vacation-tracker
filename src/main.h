#pragma once

struct Display_System;
struct Shader;

struct Shader_Catalog;
struct Texture_Catalog;

struct Time_Info {
    double last_time = 0.0;

    double current_dt = 0.0;
};

struct Globals {
    Display_System *display_system = NULL;
    bool should_quit_game = false;

    Time_Info time_info;
    
    Shader_Catalog  *shader_catalog = NULL;
    Texture_Catalog *texture_catalog = NULL;
    
    Shader *shader_color = NULL;
    Shader *shader_texture = NULL;
    Shader *shader_text = NULL;
};

extern Globals globals;

bool is_key_down(int key_code);
bool is_key_pressed(int key_code);
bool was_key_just_released(int key_code);
