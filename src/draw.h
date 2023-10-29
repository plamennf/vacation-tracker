#pragma once

#include "display_system.h"
#include "font.h"

extern float draw_y_offset_due_to_scrolling;

void init_shaders();
void rendering_2d_right_handed();
void rendering_2d_right_handed_with_y_offset(float y_offset);

void handle_resizes();
void handle_mouse_wheel_event(int num_ticks);
void handle_event(Event event);

void resolve_to_back_buffer();

void draw_quad(Vector2 position, Vector2 size, Vector4 color);
void draw_text(Dynamic_Font *font, char *text, int x, int y, Vector4 color);

void draw_game_view();
