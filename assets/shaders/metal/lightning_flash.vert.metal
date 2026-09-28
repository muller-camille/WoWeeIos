#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct lightning_flash_vert_out
{
    float4 gl_Position [[position]];
};

struct lightning_flash_vert_in
{
    float2 m_18 [[attribute(0)]];
};

vertex lightning_flash_vert_out lightning_flash_vert(lightning_flash_vert_in in [[stage_in]])
{
    lightning_flash_vert_out out = {};
    out.gl_Position = float4(in.m_18, 0.0, 1.0);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

