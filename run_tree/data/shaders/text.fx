/*
depth_test = lequal
depth_write = false
blend = dual
cull_mode = off
vertex_type = immediate

sampler = point/clamp
*/

struct VS_Output {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

cbuffer Transform : register(b0) {
    float4x4 object_to_proj;
    float4x4 view_to_proj;
    float4x4 world_to_view;
    float4x4 object_to_world;
};

VS_Output vertex_main(float3 position : POSITION, float4 color : COLOR, float2 uv : TEXCOORD) {
    VS_Output output;

    output.position = mul(object_to_proj, float4(position, 1));
    output.color    = color;
    output.uv       = uv;
    
    return output;
}

struct PS_Output {
    float4 color     : SV_TARGET0;
    float4 colorMask : SV_TARGET1;
};

Texture2D dif_tex : register(t0);
SamplerState samp_state : register(s0);

PS_Output pixel_main(VS_Output input) {
    PS_Output output;

    float sample_left   = dif_tex.Sample(samp_state, input.uv, int2(-1, 0)).r;
    float sample_center = dif_tex.Sample(samp_state, input.uv).r;
    float sample_right   = dif_tex.Sample(samp_state, input.uv, int2(+1, 0)).r;
    
    output.color     = input.color;
    output.colorMask = float4(sample_left * input.color.a, sample_center * input.color.a, sample_right * input.color.a, 1.0);
    
    return output;
}
