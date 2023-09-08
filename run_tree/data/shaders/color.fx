/*
depth_test = lequal
depth_write = false
blend = alpha
cull_mode = off
vertex_type = immediate
*/

struct VS_Output {
    float4 position : SV_POSITION;
    float4 color : COLOR;
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
    
    return output;
}

struct PS_Output {
    float4 color : SV_TARGET;
};

PS_Output pixel_main(VS_Output input) {
    PS_Output output;

    output.color = input.color;
    
    return output;
}
