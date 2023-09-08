#pragma once

struct Button_Theme {
    Vector4 bg_color;
    Vector4 text_color;
    Vector4 hovered_bg_color;
    Vector4 pressed_bg_color;
};

extern Button_Theme default_button_theme;

void init_hud_themes();

bool do_button(Dynamic_Font *font, char *text, int x, int y, int width, int height, Button_Theme theme);
