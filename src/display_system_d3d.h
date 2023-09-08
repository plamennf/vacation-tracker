#pragma once

#include "display_system.h"

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#define SafeRelease(ptr) if (ptr) { ptr->Release(); ptr = NULL; }

struct Texture_D3D : public Texture {
    ID3D11Texture2D          *texture = NULL;
    ID3D11ShaderResourceView *srv = NULL;
    ID3D11RenderTargetView   *rtv = NULL;
    ID3D11DepthStencilView   *dsv = NULL;

    ~Texture_D3D() {
        SafeRelease(dsv);
        SafeRelease(rtv);
        SafeRelease(srv);
        SafeRelease(texture);
    }
};

struct Shader_D3D : public Shader {
    ID3D11VertexShader      *vertex_shader            = NULL;
    ID3D11PixelShader       *pixel_shader             = NULL;
    ID3D11InputLayout       *input_layout             = NULL;
    ID3D11BlendState        *blend_state              = NULL;
    ID3D11RasterizerState1  *rasterizer_state         = NULL;
    ID3D11RasterizerState1  *rasterizer_state_scissor = NULL;
    ID3D11DepthStencilState *depth_stencil_state      = NULL;
    
    int num_sampler_states = 0;
    ID3D11SamplerState **sampler_states = NULL;
    
    ~Shader_D3D() {
        if (sampler_states) {
            for (int i = 0; i < num_sampler_states; i++) {
                SafeRelease(sampler_states[i]);
            }
        }
        
        SafeRelease(depth_stencil_state);
        SafeRelease(rasterizer_state_scissor);
        SafeRelease(rasterizer_state);
        SafeRelease(blend_state);
        SafeRelease(input_layout);
        SafeRelease(pixel_shader);
        SafeRelease(vertex_shader);
    }
};

struct Display_System_D3D : public Display_System {
    Display_System_D3D(int width, int height, char *title, bool vsync);
    ~Display_System_D3D();

    void handle_resizes(int width, int height);
    
    void update_window_events() override;

    void set_render_targets(Texture *ct, Texture *dt) override;
    void clear_render_target(float r, float g, float b, float a) override;

    void set_scissor(int x, int y, int width, int height) override;
    void clear_scissor() override;
    
    void immediate_begin() override;
    void immediate_flush() override;
    void immediate_quad(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, Vector4 color) override;
    void immediate_quad(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, Vector2 uv0, Vector2 uv1, Vector2 uv2, Vector2 uv3, Vector4 color) override;

    bool load_shader(Shader *shader, char *filepath) override;
    void set_shader(Shader *shader) override;

    void refresh_transform() override;

    void load_texture_from_bitmap(Texture *texture, Bitmap bitmap) override;
    void update_texture(Texture *texture, int x, int y, int width, int height, u8 *data) override;
    void set_texture(int index, Texture *texture) override;
    
    void swap_buffers() override;

    void get_mouse_pointer_position(int *x, int *y) override;
    
    Texture *create_rendertarget(Texture_Format format, int width, int height);
    
    HWND hwnd;

    ID3D11Device1        *device = NULL;
    ID3D11DeviceContext1 *device_context = NULL;
    IDXGISwapChain1      *swap_chain = NULL;
    u32                   swap_chain_flags = 0;

    ID3D11RenderTargetView *current_rtv = NULL;
    ID3D11DepthStencilView *current_dsv = NULL;

    ID3D11Buffer *immediate_vbo;

    Shader_D3D *current_shader = NULL;
    ID3D11Buffer *transform_cbo = NULL;

    bool scissor_test_enabled = false;
    
private:
    void init_back_buffer();
    void release_back_buffer();
};
