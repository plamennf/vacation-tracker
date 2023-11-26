#pragma once

#include "bitmap.h"

enum Key_Code {
    KEY_UNKNOWN = 0,

    KEY_BACKSPACE = 8,
    KEY_TAB = 9,
    KEY_ESCAPE = 27,
    KEY_SPACE = 32,

    KEY_F1 = 129,
    KEY_F2 = 130,
    KEY_F3 = 131,
    KEY_F4 = 132,
    KEY_F5 = 133,
    KEY_F6 = 134,
    KEY_F7 = 135,
    KEY_F8 = 136,
    KEY_F9 = 137,
    KEY_F10 = 138,
    KEY_F11 = 139,
    KEY_F12 = 140,

    KEY_ENTER = 153,

    KEY_SHIFT = 189,
    KEY_CTRL = 190,
    KEY_ALT = 191,

    MOUSE_BUTTON_LEFT = 210,
    MOUSE_BUTTON_RIGHT = 211,

    KEY_UP = 267,
    KEY_DOWN = 268,
    KEY_RIGHT = 269,
    KEY_LEFT = 270,
    
    NUM_KEY_CODES,
};

enum Event_Type {
    EVENT_TYPE_UNKNOWN,
    EVENT_TYPE_QUIT,
    EVENT_TYPE_KEYBOARD,
    EVENT_TYPE_MOUSE_WHEEL,
    EVENT_TYPE_TEXT_INPUT,
};

struct Event {
    Event_Type type   = EVENT_TYPE_UNKNOWN;

    Key_Code key_code = KEY_UNKNOWN;
    bool key_pressed  = false;
    bool is_repeat    = false;
    bool alt_pressed  = false;

    int utf32         = 0;
    
    int wheel_delta = 0;
    int typical_wheel_delta = 0;
};

struct Texture {
    char *full_path = NULL;
    char *name = NULL;
    u64 modtime = 0;
    
    virtual ~Texture() = default;
    
    int width  = 0;
    int height = 0;

    Texture_Format format = TEXTURE_FORMAT_UNKNOWN;
    int bytes_per_pixel = 0;
};

const int MAX_IMMEDIATE_VERTICES = 2400;

struct Immediate_Vertex {
    Vector3 position;
    Vector4 color;
    Vector2 uv;
};

enum Depth_Test {
    DEPTH_TEST_OFF,
    DEPTH_TEST_LEQUAL,
};

enum Cull_Mode {
    CULL_MODE_OFF,
    CULL_MODE_BACK,
    CULL_MODE_FRONT,
};

enum Texture_Filter {
    TEXTURE_FILTER_LINEAR,
    TEXTURE_FILTER_POINT,
};

enum Texture_Address {
    TEXTURE_ADDRESS_REPEAT,
    TEXTURE_ADDRESS_CLAMP,
};

struct Sampler_State {
    Texture_Filter filter;
    Texture_Address address;
};

enum Vertex_Type {
    VERTEX_TYPE_IMMEDIATE,
};

enum Blend_Type {
    BLEND_TYPE_NONE,
    BLEND_TYPE_ALPHA,
    BLEND_TYPE_DUAL,
};

struct Shader_Options {
    Depth_Test depth_test = DEPTH_TEST_LEQUAL;
    bool depth_write = true;
    Blend_Type blend_type = BLEND_TYPE_NONE;
    Cull_Mode cull_mode = CULL_MODE_BACK;
    Vertex_Type vertex_type = VERTEX_TYPE_IMMEDIATE;

    int num_sampler_states = 0;
    Sampler_State *sampler_states = NULL;
};

struct Shader {
    char *full_path = NULL;
    char *name = NULL;
    u64 modtime = 0;

    virtual ~Shader() = default;
    
    Shader_Options options;
};

struct Display_System {
    Array <Event> events_this_frame;
    bool should_vsync;

    Immediate_Vertex immediate_vertices[MAX_IMMEDIATE_VERTICES];
    int num_immediate_vertices;

    bool maintain_aspect_ratio;
    float desired_aspect_ratio;
    float aspect_ratio;    
    
    int display_width;
    int display_height;
    bool maximized;
    
    int target_width;
    int target_height;
    
    Matrix4 object_to_proj_matrix;
    Matrix4 view_to_proj_matrix;
    Matrix4 world_to_view_matrix;
    Matrix4 object_to_world_matrix;
    
    Texture *back_buffer;
    Texture *offscreen_buffer;

    int offset_offscreen_to_back_buffer_x;
    int offset_offscreen_to_back_buffer_y;

    void (*resize_callback)();
    
    Display_System();
    virtual ~Display_System();

    virtual void update_window_events() = 0;

    virtual void set_render_targets(Texture *ct, Texture *dt) = 0;
    virtual void clear_render_target(float r, float g, float b, float a) = 0;

    virtual void set_scissor(int x, int y, int width, int height) = 0;
    virtual void clear_scissor() = 0;
    
    virtual void immediate_begin() = 0;
    virtual void immediate_flush() = 0;
    virtual void immediate_quad(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, Vector4 color) = 0;
    virtual void immediate_quad(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, Vector2 uv0, Vector2 uv1, Vector2 uv2, Vector2 uv3, Vector4 color) = 0;

    virtual bool load_shader(Shader *shader, char *filepath) = 0;
    virtual void set_shader(Shader *shader) = 0;
    
    virtual void refresh_transform() = 0;

    bool load_texture(Texture *texture, char *filepath);
    
    virtual void load_texture_from_bitmap(Texture *texture, Bitmap bitmap) = 0;
    virtual void update_texture(Texture *texture, int x, int y, int width, int height, u8 *data) = 0;
    virtual void set_texture(int index, Texture *texture) = 0;
    
    virtual void swap_buffers() = 0;

    virtual Texture *create_rendertarget(Texture_Format format, int width, int height) = 0;

    virtual void get_mouse_pointer_position(int *x, int *y) = 0;
    
    void resize_render_targets(); // Based on display_width and display_height
};

Display_System *make_display_system(int width, int height, char *title, bool vsync);
Shader *make_shader();
Texture *make_texture();
