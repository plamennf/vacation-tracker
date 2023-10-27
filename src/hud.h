#pragma once

enum Button_Press_Requirement {
    BUTTON_SHOULD_BE_PRESSED,
    BUTTON_SHOULD_BE_RELEASED,
};

struct Button_Theme {
    Vector4 bg_color;
    Vector4 text_color;
    Vector4 hovered_bg_color;
    Vector4 pressed_bg_color;

    bool                     allow_right_clicks = false;
    Button_Press_Requirement press_requirement  = BUTTON_SHOULD_BE_RELEASED;
};

extern Button_Theme default_button_theme;

void init_hud_themes();
void hud_declare_occlusion(int x, int y, int width, int height);
void hud_remove_occlusion(int num_frame_to_wait = 0);

enum class Button_State {
    NONE,
    LEFT_PRESSED,
    RIGHT_PRESSED,
};

Button_State do_button(Dynamic_Font *font, char *text, int x, int y, int width, int height, Button_Theme theme, bool bypasses_occlusion = false);
