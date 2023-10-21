#include "pch.h"
#include "display_system_d3d.h"
#include "os_specific.h"

#include <stdio.h> // For sscanf
#include <stdlib.h> // For exit.
#include <string.h> // For strlen.

static Key_Code vk_code_to_key_code(u32 vk_code) {
    if (vk_code >= 48 && vk_code <= 90) return (Key_Code) vk_code;

    switch (vk_code) {
    case VK_BACK: return KEY_BACKSPACE;
    case VK_TAB: return KEY_TAB;
    case VK_ESCAPE: return KEY_ESCAPE;
    case VK_SPACE: return KEY_SPACE;

    case VK_F1: return KEY_F1;
    case VK_F2: return KEY_F2;
    case VK_F3: return KEY_F3;
    case VK_F4: return KEY_F4;
    case VK_F5: return KEY_F5;
    case VK_F6: return KEY_F6;
    case VK_F7: return KEY_F7;
    case VK_F8: return KEY_F8;
    case VK_F9: return KEY_F9;
    case VK_F10: return KEY_F10;
    case VK_F11: return KEY_F11;
    case VK_F12: return KEY_F12;

    case VK_RETURN: return KEY_ENTER;

    case VK_SHIFT: return KEY_SHIFT;
    case VK_CONTROL: return KEY_CTRL;
    case VK_MENU: return KEY_ALT;

    case VK_UP: return KEY_UP;
    case VK_DOWN: return KEY_DOWN;
    case VK_RIGHT: return KEY_RIGHT;
    case VK_LEFT: return KEY_LEFT;
    }

    return KEY_UNKNOWN;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Display_System_D3D *sys = (Display_System_D3D *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    case WM_CLOSE: {
        Event event;
        event.type = EVENT_TYPE_QUIT;
        sys->events_this_frame.add(event);
        break;
    }

    case WM_SIZE: {
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width  = rect.right  - rect.left;
        int height = rect.bottom - rect.top;

        sys->handle_resizes(width, height);
        
        break;
    }

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP: {
        u32 vk_code       = (u32)wParam;
        Key_Code key_code = vk_code_to_key_code(vk_code);
        bool is_down      = (lParam & (1 << 31)) == 0;
        bool was_down     = (lParam & (1 << 30)) == 1;
        
        bool alt_pressed  = GetKeyState(VK_MENU) & 0x8000;

        if (alt_pressed && is_down && !was_down) {
            if (key_code == KEY_F4) {
                Event event;
                event.type = EVENT_TYPE_QUIT;
                sys->events_this_frame.add(event);
            } else if (key_code == KEY_ENTER) {
                // sys->toggle_fullscreen();
            }
        }

        Event event;
        event.type        = EVENT_TYPE_KEYBOARD;
        event.key_code    = key_code;
        event.key_pressed = is_down;
        event.is_repeat   = is_down == was_down;
        event.alt_pressed = alt_pressed;
        sys->events_this_frame.add(event);
        
        break;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP: {
        bool alt_pressed  = GetKeyState(VK_MENU) & 0x8000;
        
        Event event;
        event.type        = EVENT_TYPE_KEYBOARD;
        event.key_code    = MOUSE_BUTTON_LEFT;
        event.key_pressed = msg == WM_LBUTTONDOWN;
        event.is_repeat   = false;
        event.alt_pressed = alt_pressed;
        sys->events_this_frame.add(event);
        
        break;
    }

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP: {
        bool alt_pressed  = GetKeyState(VK_MENU) & 0x8000;
        
        Event event;
        event.type        = EVENT_TYPE_KEYBOARD;
        event.key_code    = MOUSE_BUTTON_RIGHT;
        event.key_pressed = msg == WM_RBUTTONDOWN;
        event.is_repeat   = false;
        event.alt_pressed = alt_pressed;
        sys->events_this_frame.add(event);
        
        break;
    }
        
    default: return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    
    return 0;
}

Display_System_D3D::Display_System_D3D(int width, int height, char *title, bool vsync) {
    if (width <= 0) {
        log_error("Display_System(): width can't be <= 0.\n");
        exit(1);
    }
    if (height <= 0) {
        log_error("Display_System(): height can't be <= 0.\n");
        exit(1);
    }
    
    should_vsync = vsync;
    
    WNDCLASSEXW wc = {};

    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandleW(NULL);
    wc.hIcon         = LoadIconW(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = L"GameWin32WindowClass";
    wc.hIconSm       = LoadIconW(NULL, IDI_APPLICATION);

    if (RegisterClassExW(&wc) == 0) {
        log_error("RegisterClassExW returned 0.\n");
        exit(1);
    }

    DWORD window_style = WS_OVERLAPPEDWINDOW;

    RECT window_rect = {};
    window_rect.right  = width;
    window_rect.bottom = height;

    AdjustWindowRect(&window_rect, window_style, FALSE);

    int window_width  = window_rect.right - window_rect.left;
    int window_height = window_rect.bottom - window_rect.top;

    wchar_t wide_title[4096];
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wide_title, ArrayCount(wide_title));
    hwnd = CreateWindowExW(0, wc.lpszClassName, wide_title, window_style,
                           CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height,
                           0, 0, wc.hInstance, this);
    if (hwnd == NULL) {
        log_error("RegisterClassExW returned 0.\n");
        exit(1);
    }

    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);

    int monitor_width_without_taskbar  = mi.rcWork.right  - mi.rcWork.left;
    int monitor_height_without_taskbar = mi.rcWork.bottom - mi.rcWork.top;

    int window_x = mi.rcWork.left + ((monitor_width_without_taskbar  - window_width)  / 2);
    int window_y = mi.rcWork.top  + ((monitor_height_without_taskbar - window_height) / 2);

    SetWindowPos(hwnd, HWND_TOP, window_x, window_y, 0, 0, SWP_NOSIZE);
    
    UpdateWindow(hwnd);
    ShowWindow(hwnd, SW_SHOWDEFAULT);

    //
    // Create device and device context
    //
    {
        D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };

        ID3D11Device *base_device = NULL;
        defer { SafeRelease(base_device); };
        ID3D11DeviceContext *base_device_context = NULL;
        defer { SafeRelease(base_device_context); };

        UINT device_create_flags = D3D11_CREATE_DEVICE_SINGLETHREADED;// | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
        device_create_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        
        D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, device_create_flags,
                          feature_levels, ArrayCount(feature_levels), D3D11_SDK_VERSION,
                          &base_device, NULL, &base_device_context);

        base_device->QueryInterface(IID_PPV_ARGS(&device));
        base_device_context->QueryInterface(IID_PPV_ARGS(&device_context));
    }

    //
    // Create swap chain
    //
    {
        swap_chain_flags = 0;
        if (!should_vsync) {
            swap_chain_flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }
        
        IDXGIDevice1 *dxgi_device = NULL;
        defer { SafeRelease(dxgi_device); };
        device->QueryInterface(IID_PPV_ARGS(&dxgi_device));

        IDXGIAdapter *dxgi_adapter = NULL;
        defer { SafeRelease(dxgi_adapter); };
        dxgi_device->GetAdapter(&dxgi_adapter);

        IDXGIFactory2 *dxgi_factory = NULL;
        defer { SafeRelease(dxgi_factory); };
        dxgi_adapter->GetParent(IID_PPV_ARGS(&dxgi_factory));

        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
        swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_desc.SampleDesc.Count = 1;
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.BufferCount = 2;
        swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swap_chain_desc.Flags = swap_chain_flags;

        dxgi_factory->CreateSwapChainForHwnd(device, hwnd, &swap_chain_desc, NULL, NULL, &swap_chain);
        dxgi_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    }

    Texture_D3D *_back_buffer = new Texture_D3D();
    _back_buffer->width  = width;
    _back_buffer->height = height;
    _back_buffer->format = TEXTURE_FORMAT_RGBA8;
    _back_buffer->bytes_per_pixel = 4;
    back_buffer = (Texture *)_back_buffer;
    init_back_buffer();
    
    display_width  = width;
    display_height = height;

    target_width   = 0;
    target_height  = 0;

    resize_callback = NULL;
    
    maintain_aspect_ratio = false;
    aspect_ratio = (float)display_width / (float)display_height;

    log("vsync: %s\n", should_vsync ? "on" : "off");
    log("Display size: %dx%d\n", display_width, display_height);
    
    num_immediate_vertices = 0;

    D3D11_BUFFER_DESC immediate_vbo_bd = {};
    immediate_vbo_bd.ByteWidth         = MAX_IMMEDIATE_VERTICES * sizeof(Immediate_Vertex);
    immediate_vbo_bd.Usage             = D3D11_USAGE_DYNAMIC;
    immediate_vbo_bd.BindFlags         = D3D11_BIND_VERTEX_BUFFER;
    immediate_vbo_bd.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&immediate_vbo_bd, NULL, &immediate_vbo);

    D3D11_BUFFER_DESC transform_cbo_bd = {};
    transform_cbo_bd.ByteWidth         = 4 * sizeof(Matrix4);
    transform_cbo_bd.Usage             = D3D11_USAGE_DYNAMIC;
    transform_cbo_bd.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
    transform_cbo_bd.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&transform_cbo_bd, NULL, &transform_cbo);
}

Display_System_D3D::~Display_System_D3D() {
    SafeRelease(transform_cbo);
    SafeRelease(immediate_vbo);
    
    release_back_buffer();
    
    SafeRelease(swap_chain);
    SafeRelease(device_context);
    SafeRelease(device);
}

void Display_System_D3D::handle_resizes(int width, int height) {
    if (!swap_chain) return;
    if (!width || !height) return;

    release_back_buffer();
    swap_chain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, swap_chain_flags);
    init_back_buffer();

    back_buffer->width  = width;
    back_buffer->height = height;

    display_width  = width;
    display_height = height;

    aspect_ratio = (float)display_width / (float)display_height;

    resize_render_targets();
}

void Display_System_D3D::update_window_events() {
    events_this_frame.count = 0;
    
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void Display_System_D3D::set_render_targets(Texture *_ct, Texture *_dt) {
    auto ct = (Texture_D3D *)_ct;
    auto dt = (Texture_D3D *)_dt;
    
    target_width  = 0;
    target_height = 0;
    
    ID3D11RenderTargetView *rtv = NULL;
    if (ct) {
        rtv = ct->rtv;
        target_width = ct->width;
        target_height = ct->height;
    }

    ID3D11DepthStencilView *dsv = NULL;
    if (dt) {
        dsv = dt->dsv;

        if (!ct) {
            target_width  = dt->width;
            target_height = dt->height;
        }
    }

    device_context->OMSetRenderTargets(rtv ? 1 : 0, &rtv, dsv);
    
    current_rtv = rtv;
    current_dsv = dsv;

    D3D11_VIEWPORT viewport = {};
    viewport.Width    = (float)target_width;
    viewport.Height   = (float)target_height;
    viewport.MaxDepth = 1.0f;
    device_context->RSSetViewports(1, &viewport);
}

void Display_System_D3D::clear_render_target(float r, float g, float b, float a) {
    if (current_rtv) {
        float clear_color[4] = { r, g, b, a };
        device_context->ClearRenderTargetView(current_rtv, clear_color);
    }

    if (current_dsv) {
        UINT flags = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
        float depth = 1.0f;
        UINT stencil = 0;
        device_context->ClearDepthStencilView(current_dsv, flags, depth, stencil);
    }
}

void Display_System_D3D::set_scissor(int x, int y, int width, int height) {
    if (!current_shader) return;
    if (scissor_test_enabled) return;
    device_context->RSSetState(current_shader->rasterizer_state_scissor);

    D3D11_RECT rect;
    rect.left = x;
    rect.right = x + width;
    rect.top = y;
    rect.bottom = y + height;
    device_context->RSSetScissorRects(1, &rect);
}

void Display_System_D3D::clear_scissor() {
    if (!current_shader) return;
    if (!scissor_test_enabled) return;
    device_context->RSSetState(current_shader->rasterizer_state);
}

void Display_System_D3D::immediate_begin() {
    immediate_flush();
}

void Display_System_D3D::immediate_flush() {
    if (!current_shader) return;
    if (!num_immediate_vertices) return;

    D3D11_MAPPED_SUBRESOURCE msr;
    device_context->Map(immediate_vbo, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, immediate_vertices, num_immediate_vertices * sizeof(Immediate_Vertex));
    device_context->Unmap(immediate_vbo, 0);
    
    UINT stride = sizeof(Immediate_Vertex);
    UINT offset = 0;
    device_context->IASetVertexBuffers(0, 1, &immediate_vbo, &stride, &offset);
    
    device_context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device_context->Draw(num_immediate_vertices, 0);
    
    num_immediate_vertices = 0;
}

static void put_vertex(Immediate_Vertex *v, Vector2 position, Vector4 color, Vector2 uv) {
    v->position = Vector3(position.x, position.y, 0);
    v->color    = color;
    v->uv       = uv;
}

void Display_System_D3D::immediate_quad(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, Vector4 color) {
    Vector2 uv0(0, 0);
    Vector2 uv1(1, 0);
    Vector2 uv2(1, 1);
    Vector2 uv3(0, 1);

    immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, color);
}

void Display_System_D3D::immediate_quad(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, Vector2 uv0, Vector2 uv1, Vector2 uv2, Vector2 uv3, Vector4 color) {
    if (num_immediate_vertices + 6 > MAX_IMMEDIATE_VERTICES) immediate_flush();

    auto v = immediate_vertices + num_immediate_vertices;

    put_vertex(&v[0], p0, color, uv0);
    put_vertex(&v[1], p1, color, uv1);
    put_vertex(&v[2], p2, color, uv2);
    
    put_vertex(&v[3], p0, color, uv0);
    put_vertex(&v[4], p2, color, uv2);
    put_vertex(&v[5], p3, color, uv3);
    
    num_immediate_vertices += 6;
}

static bool parse_shader_options(Shader_Options *options, char *file_data) {
    Array <Sampler_State> sampler_states;
    
    while (1) {
        char *line = consume_next_line(&file_data);
        if (!line) break;

        line = eat_spaces(line);
        line = eat_trailing_spaces(line);

        if (starts_with(line, "depth_test")) {
            line += strlen("depth_test");
            line = eat_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after depth_test");
                return false;
            }
            line += 1;

            line = eat_spaces(line);

            if (strings_match(line, "off")) {
                options->depth_test = DEPTH_TEST_OFF;
            } else if (strings_match(line, "lequal")) {
                options->depth_test = DEPTH_TEST_LEQUAL;
            } else {
                log_error("depth_test mode '%s' not supported\n", line);
                log_error("Valid values are:\n");
                log_error("    off\n");
                log_error("    lequal\n");
                return false;
            }
        } else if (starts_with(line, "depth_write")) {
            line += strlen("depth_write");
            line = eat_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after depth_write");
                return false;
            }
            line += 1;
            
            line = eat_spaces(line);
            
            if (strings_match(line, "false")) {
                options->depth_write = false;
            } else if (strings_match(line, "true")) {
                options->depth_write = true;
            } else {
                log_error("depth_write mode '%s' not supported\n", line);
                log_error("Valid values are:\n");
                log_error("    false\n");
                log_error("    true\n");
                return false;
            }
        } else if (starts_with(line, "blend")) {
            line += strlen("blend");
            line = eat_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after blend");
                return false;
            }
            line += 1;
            
            line = eat_spaces(line);
            
            if (strings_match(line, "none")) {
                options->blend_type = BLEND_TYPE_NONE;
            } else if (strings_match(line, "alpha")) {
                options->blend_type = BLEND_TYPE_ALPHA;
            } else if (strings_match(line, "dual")) {
                options->blend_type = BLEND_TYPE_DUAL;
            } else {
                log_error("blend mode '%s' not supported\n", line);
                log_error("Valid values are:\n");
                log_error("    none\n");
                log_error("    alpha\n");
                log_error("    dual\n");
                return false;
            }
        } else if (starts_with(line, "cull_mode")) {
            line += strlen("cull_mode");
            line = eat_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after cull_mode");
                return false;
            }
            line += 1;

            line = eat_spaces(line);

            if (strings_match(line, "off")) {
                options->cull_mode = CULL_MODE_OFF;
            } else if (strings_match(line, "back")) {
                options->cull_mode = CULL_MODE_BACK;
            } else if (strings_match(line, "front")) {
                options->cull_mode = CULL_MODE_FRONT;
            } else {
                log_error("cull_mode mode '%s' not supported\n", line);
                log_error("Valid values are:\n");
                log_error("    off\n");
                log_error("    back\n");
                log_error("    front\n");
                return false;
            }
        } else if (starts_with(line, "vertex_type")) {
            line += strlen("vertex_type");
            line = eat_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after vertex_type");
                return false;
            }

            line += 1;

            line = eat_spaces(line);

            if (strings_match(line, "immediate")) {
                options->vertex_type = VERTEX_TYPE_IMMEDIATE;
            } else {
                log_error("vertex_type mode '%s' not supported\n", line);
                log_error("Valid values are:\n");
                log_error("    immediate\n");
                return false;
            }
        } else if (starts_with(line, "sampler")) {
            line += strlen("sampler");
            line = eat_spaces(line);

            if (line[0] != '=') {
                log_error("Expected '=' after sampler");
                return false;
            }

            line += 1;

            line = eat_spaces(line);

            char texture_filter_string[4096], texture_address_string[4096];
            int res = sscanf(line, "%4095[^/]/%4095[^/]", texture_filter_string, texture_address_string); // Very weird syntax.
            if (res != 2) {
                log_error("Expected filter/address after sampler =, but instead found: %s.\n", line);
                return false;
            }

            Texture_Filter texture_filter = TEXTURE_FILTER_LINEAR;
            if (strings_match(texture_filter_string, "linear")) {
                texture_filter = TEXTURE_FILTER_LINEAR;
            } else if (strings_match(texture_filter_string, "point")) {
                texture_filter = TEXTURE_FILTER_POINT;
            } else {
                log_error("texture filter '%s' not supported\n", line);
                log_error("Valid values are:\n");
                log_error("    linear\n");
                log_error("    point\n");
                return false;
            }

            Texture_Address texture_address = TEXTURE_ADDRESS_REPEAT;
            if (strings_match(texture_address_string, "repeat")) {
                texture_address = TEXTURE_ADDRESS_REPEAT;
            } else if (strings_match(texture_address_string, "clamp")) {
                texture_address = TEXTURE_ADDRESS_CLAMP;
            } else {
                log_error("texture address '%s' not supported\n", line);
                log_error("Valid values are:\n");
                log_error("    repeat\n");
                log_error("    clamp\n");
                return false;
            }

            Sampler_State sampler_state = {};
            sampler_state.filter = texture_filter;
            sampler_state.address = texture_address;
            sampler_states.add(sampler_state);
        }
    }

    options->num_sampler_states = sampler_states.count;
    options->sampler_states     = sampler_states.copy_to_array();

    return true;
}

static D3D11_CULL_MODE d3d11_cull_mode(Cull_Mode cull_mode) {
    switch (cull_mode) {
    case CULL_MODE_OFF:   return D3D11_CULL_NONE;
    case CULL_MODE_FRONT: return D3D11_CULL_FRONT;
    case CULL_MODE_BACK:  return D3D11_CULL_BACK;
    }
    return D3D11_CULL_NONE;
}

static D3D11_COMPARISON_FUNC d3d11_depth_func(Depth_Test mode) {
    switch (mode) {
    case DEPTH_TEST_LEQUAL: return D3D11_COMPARISON_LESS_EQUAL;
    }
    return D3D11_COMPARISON_LESS_EQUAL;
}

bool Display_System_D3D::load_shader(Shader *_shader, char *filepath) {
    char *orig_file_data = os_read_entire_file(filepath);
    if (!orig_file_data) {
        log_error("Failed to read file '%s'.\n", filepath);
        return false;
    }
    defer { delete [] orig_file_data; };

    ID3DBlob *vertex_code = NULL, *vertex_error = NULL;
    defer { SafeRelease(vertex_code); SafeRelease(vertex_error); };
    D3DCompile(orig_file_data, strlen(orig_file_data), filepath, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "vertex_main", "vs_5_0", 0, 0, &vertex_code, &vertex_error);
    if (vertex_error) {
        log_error("Failed to compile '%s' vertex shader:\n%s\n", filepath, (char *)vertex_error->GetBufferPointer());
        return false;
    }
    ID3D11VertexShader *vertex_shader = NULL;
    device->CreateVertexShader(vertex_code->GetBufferPointer(), vertex_code->GetBufferSize(), NULL, &vertex_shader);

    ID3DBlob *pixel_code = NULL, *pixel_error = NULL;
    defer { SafeRelease(pixel_code); SafeRelease(pixel_error); };
    D3DCompile(orig_file_data, strlen(orig_file_data), filepath, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "pixel_main", "ps_5_0", 0, 0, &pixel_code, &pixel_error);
    if (pixel_error) {
        log_error("Failed to compile '%s' pixel shader:\n%s\n", filepath, (char *)pixel_error->GetBufferPointer());
        return false;
    }
    ID3D11PixelShader *pixel_shader = NULL;
    device->CreatePixelShader(pixel_code->GetBufferPointer(), pixel_code->GetBufferSize(), NULL, &pixel_shader);

    char *file_data = orig_file_data;
    Shader_Options options = {};
    if (!parse_shader_options(&options, file_data)) return false;
    
    Array <D3D11_INPUT_ELEMENT_DESC> ieds;
    switch (options.vertex_type) {
    case VERTEX_TYPE_IMMEDIATE: {
        ieds.resize(3);
        memset(ieds.data, 0, ieds.count * sizeof(D3D11_INPUT_ELEMENT_DESC));

        ieds[0].SemanticName      = "POSITION";
        ieds[0].Format            = DXGI_FORMAT_R32G32B32_FLOAT;
        ieds[0].AlignedByteOffset = offsetof(Immediate_Vertex, position);
        ieds[0].InputSlotClass    = D3D11_INPUT_PER_VERTEX_DATA;

        ieds[1].SemanticName      = "COLOR";
        ieds[1].Format            = DXGI_FORMAT_R32G32B32A32_FLOAT;
        ieds[1].AlignedByteOffset = offsetof(Immediate_Vertex, color);
        ieds[1].InputSlotClass    = D3D11_INPUT_PER_VERTEX_DATA;

        ieds[2].SemanticName      = "TEXCOORD";
        ieds[2].Format            = DXGI_FORMAT_R32G32_FLOAT;
        ieds[2].AlignedByteOffset = offsetof(Immediate_Vertex, uv);
        ieds[2].InputSlotClass    = D3D11_INPUT_PER_VERTEX_DATA;
        
        break;
    }
    }

    ID3D11InputLayout *input_layout = NULL;
    device->CreateInputLayout(ieds.data, ieds.count, vertex_code->GetBufferPointer(), vertex_code->GetBufferSize(), &input_layout);

    if (!input_layout) {
        log_error("Failed to create '%s' input layout:\n%s\n", filepath);
        return false;
    }
    
    ID3D11RasterizerState1  *rasterizer_state = NULL;
    ID3D11RasterizerState1  *rasterizer_state_scissor = NULL;
    ID3D11DepthStencilState *depth_stencil_state = NULL;
    ID3D11BlendState        *blend_state = NULL;
        
    D3D11_RASTERIZER_DESC1 rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D11_FILL_SOLID;
    rasterizer_desc.CullMode = d3d11_cull_mode(options.cull_mode);
    rasterizer_desc.FrontCounterClockwise = true;
    rasterizer_desc.ScissorEnable = false;
    rasterizer_desc.DepthClipEnable = true;
    device->CreateRasterizerState1(&rasterizer_desc, &rasterizer_state);

    rasterizer_desc.ScissorEnable = true;
    device->CreateRasterizerState1(&rasterizer_desc, &rasterizer_state_scissor);
    
    D3D11_DEPTH_STENCIL_DESC ds_desc = {};
    ds_desc.DepthEnable = options.depth_test != DEPTH_TEST_OFF;
    ds_desc.DepthWriteMask = options.depth_write ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    ds_desc.DepthFunc = d3d11_depth_func(options.depth_test);
    device->CreateDepthStencilState(&ds_desc, &depth_stencil_state);

    D3D11_BLEND_DESC blend_desc = {};
    blend_desc.RenderTarget[0].BlendEnable           = options.blend_type != BLEND_TYPE_NONE;

    switch (options.blend_type) {
    case BLEND_TYPE_ALPHA:
        blend_desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
        blend_desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
        blend_desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA;
        blend_desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
        blend_desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        break;

    case BLEND_TYPE_DUAL:
        blend_desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC1_COLOR;
        blend_desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC1_COLOR;
        blend_desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC1_ALPHA;
        blend_desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC1_ALPHA;
        blend_desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        break;
    }
    device->CreateBlendState(&blend_desc, &blend_state);
    
    auto sampler_states = new ID3D11SamplerState * [options.num_sampler_states];
    for (int i = 0; i < options.num_sampler_states; i++) {
        auto desc = options.sampler_states[i];
        
        D3D11_SAMPLER_DESC sampler_desc = {};
        sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        
        switch (desc.filter) {
            case TEXTURE_FILTER_LINEAR:
                sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                break;

            case TEXTURE_FILTER_POINT:
                sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
                break;
        }

        switch (desc.address) {
            case TEXTURE_ADDRESS_REPEAT:
                sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
                sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
                sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
                break;

            case TEXTURE_ADDRESS_CLAMP:
                sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
                sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
                sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
                break;
        }

        device->CreateSamplerState(&sampler_desc, &sampler_states[i]);
    }
    
    auto shader = (Shader_D3D *)_shader;
    
    shader->options                  = options;
    
    shader->vertex_shader            = vertex_shader;
    shader->pixel_shader             = pixel_shader;
    shader->input_layout             = input_layout;
    shader->blend_state              = blend_state;
    shader->rasterizer_state         = rasterizer_state;
    shader->rasterizer_state_scissor = rasterizer_state_scissor;
    shader->depth_stencil_state      = depth_stencil_state;

    shader->num_sampler_states       = options.num_sampler_states;
    shader->sampler_states           = sampler_states;

    return true;
}

void Display_System_D3D::set_shader(Shader *_shader) {
    auto shader = (Shader_D3D *)_shader;
    if (current_shader == shader) return;

    current_shader = shader;

    if (!shader) return;
    
    device_context->VSSetShader(shader->vertex_shader, NULL, 0);
    device_context->PSSetShader(shader->pixel_shader,  NULL, 0);
    device_context->IASetInputLayout(shader->input_layout);
    device_context->OMSetBlendState(shader->blend_state, NULL, 0xFFFFFFFF);
    if (scissor_test_enabled) {
        device_context->RSSetState(shader->rasterizer_state_scissor);
    } else {
        device_context->RSSetState(shader->rasterizer_state);
    }
    device_context->OMSetDepthStencilState(shader->depth_stencil_state, 0);

    ID3D11Buffer *vertex_cbos[] = { transform_cbo };
    device_context->VSSetConstantBuffers(0, ArrayCount(vertex_cbos), vertex_cbos);

    device_context->PSSetSamplers(0, shader->num_sampler_states, shader->sampler_states);
}

void Display_System_D3D::refresh_transform() {
    object_to_proj_matrix = view_to_proj_matrix * (world_to_view_matrix * object_to_world_matrix);

    Matrix4 matrices[] = {
        object_to_proj_matrix.transpose(),
        view_to_proj_matrix.transpose(),
        world_to_view_matrix.transpose(),
        object_to_world_matrix.transpose(),
    };
    
    D3D11_MAPPED_SUBRESOURCE msr;
    device_context->Map(transform_cbo, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, matrices, sizeof(matrices));
    device_context->Unmap(transform_cbo, 0);
}

void Display_System_D3D::load_texture_from_bitmap(Texture *_texture, Bitmap bitmap) {
    Texture_D3D *texture = (Texture_D3D *)_texture;

    SafeRelease(texture->dsv);
    SafeRelease(texture->rtv);
    SafeRelease(texture->srv);
    SafeRelease(texture->texture);
    
    assert(bitmap.format != TEXTURE_FORMAT_UNKNOWN);

    texture->width = bitmap.width;
    texture->height = bitmap.height;
    texture->format = bitmap.format;
    texture->bytes_per_pixel = bitmap.bytes_per_pixel;
    
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    switch (bitmap.format) {
    case TEXTURE_FORMAT_RGBA8:
        format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        break;

    case TEXTURE_FORMAT_RGBA8_NO_SRGB:
        format = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;

    case TEXTURE_FORMAT_R8:
        format = DXGI_FORMAT_R8_UNORM;
        break;
    }

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = bitmap.width;
    td.Height = bitmap.height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = format;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA srd = {};
    srd.pSysMem = bitmap.data;
    srd.SysMemPitch = bitmap.width * bitmap.bytes_per_pixel;

    device->CreateTexture2D(&td, bitmap.data ? &srd : NULL, &texture->texture);

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = td.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(texture->texture, &srv_desc, &texture->srv);
}

void Display_System_D3D::update_texture(Texture *_texture, int x, int y, int width, int height, u8 *data) {
    Texture_D3D *texture = (Texture_D3D *)_texture;
    
    D3D11_BOX box;
    box.left = x;
    box.right = box.left + width;
    box.top = y;
    box.bottom = box.top + height;
    box.front = 0;
    box.back = 1;
    
    device_context->UpdateSubresource(texture->texture, 0, &box, data, width * texture->bytes_per_pixel, 0);
}

void Display_System_D3D::set_texture(int index, Texture *_texture) {
    Texture_D3D *texture = (Texture_D3D *)_texture;

    ID3D11ShaderResourceView *srv = NULL;
    if (texture) {
        srv = texture->srv;
    }
    device_context->PSSetShaderResources(index, 1, &srv);
}

void Display_System_D3D::swap_buffers() {
    if (should_vsync) {
        swap_chain->Present(1, 0);
    } else {
        swap_chain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    }
}

void Display_System_D3D::get_mouse_pointer_position(int *x, int *y) {
    POINT pt;
    GetCursorPos(&pt);

    ScreenToClient(hwnd, &pt);

    if (1/*flipped*/) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        int height = rect.bottom - rect.top;
        pt.y = height - pt.y;
    }

    if (x) *x = pt.x;
    if (y) *y = pt.y;
}

Texture *Display_System_D3D::create_rendertarget(Texture_Format texture_format, int width, int height) {
    Texture_D3D *result = new Texture_D3D();

    result->width  = width;
    result->height = height;

    result->format = texture_format;
    
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    switch (texture_format) {
    case TEXTURE_FORMAT_RGBA8_NO_SRGB:
        format = DXGI_FORMAT_R8G8B8A8_UNORM;
        result->bytes_per_pixel = 4;
        break;

    case TEXTURE_FORMAT_RGBA8:
        format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        result->bytes_per_pixel = 4;
        break;

    case TEXTURE_FORMAT_R8:
        format = DXGI_FORMAT_R8_UNORM;
        result->bytes_per_pixel = 1;
        break;
    }
    
    D3D11_TEXTURE2D_DESC texture_desc = {};
    texture_desc.Width            = width;
    texture_desc.Height           = height;
    texture_desc.MipLevels        = 1;
    texture_desc.ArraySize        = 1;
    texture_desc.Format           = format;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage            = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags        = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    device->CreateTexture2D(&texture_desc, NULL, &result->texture);

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = texture_desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(result->texture, &srv_desc, &result->srv);
    
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = texture_desc.Format;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtv_desc.Texture2D = D3D11_TEX2D_RTV{0};
    device->CreateRenderTargetView(result->texture, &rtv_desc, &result->rtv);
    
    return result;
}

void Display_System_D3D::init_back_buffer() {
    auto bb = (Texture_D3D *)back_buffer;
    
    swap_chain->GetBuffer(0, IID_PPV_ARGS(&bb->texture));
    
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    device->CreateRenderTargetView(bb->texture, &rtv_desc, &bb->rtv);
}

void Display_System_D3D::release_back_buffer() {
    auto bb = (Texture_D3D *)back_buffer;

    SafeRelease(bb->dsv);
    SafeRelease(bb->rtv);
    SafeRelease(bb->srv);
    SafeRelease(bb->texture);
}

Display_System *make_display_system(int width, int height, char *title, bool vsync) {
    return new Display_System_D3D(width, height, title, vsync);
}

Shader *make_shader() {
    return new Shader_D3D();
}

Texture *make_texture() {
    return new Texture_D3D();
}
