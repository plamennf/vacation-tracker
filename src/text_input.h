#pragma once

struct Event;
struct Dynamic_Font;

struct Text_Input {
    static const int MAX_BUFFER_SIZE = 8000;
    int input_buffer[MAX_BUFFER_SIZE];
    int num_characters = 0;

    int cursor = 0;
    
    bool initted = false;
    bool active  = false;

    double last_keypress_time = 0.0;
    
    void init();

    void activate();
    void deactivate();

    bool is_active();
    char *get_result(int final_character = -1);

    void reset();
    
    void handle_event(Event event);

    void draw(Dynamic_Font *font, int text_x, int text_y, int entry_width, Vector4 text_color);

private:
    Vector4 get_cursor_color(Vector4 non_white);
};
