#pragma once

#include "display_system.h"
#include "font.h"

void init_shaders();
void rendering_2d_right_handed();

void handle_resizes();

void resolve_to_back_buffer();

void draw_quad(Vector2 position, Vector2 size, Vector4 color);
void draw_text(Dynamic_Font *font, char *text, int x, int y, Vector4 color);

void draw_game_view();
