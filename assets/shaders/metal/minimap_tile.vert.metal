#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _17
{
    float2 _m0;
};

struct minimap_tile_vert_out
{
    float2 m_9 [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct minimap_tile_vert_in
{
    float2 m_15 [[attribute(0)]];
    float2 m_11 [[attribute(1)]];
};

vertex minimap_tile_vert_out minimap_tile_vert(minimap_tile_vert_in in [[stage_in]], constant _17& _19 [[buffer(0)]])
{
    minimap_tile_vert_out out = {};
    out.m_9 = in.m_11;
    out.gl_Position = float4((((in.m_15 + _19._m0) * float2(0.3333333432674407958984375)) * 2.0) - float2(1.0), 0.0, 1.0);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

