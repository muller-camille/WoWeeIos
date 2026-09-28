#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _16
{
    float4 _m0;
};

struct minimap_display_vert_out
{
    float2 m_9 [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct minimap_display_vert_in
{
    float2 m_25 [[attribute(0)]];
    float2 m_11 [[attribute(1)]];
};

vertex minimap_display_vert_out minimap_display_vert(minimap_display_vert_in in [[stage_in]], constant _16& _18 [[buffer(0)]])
{
    minimap_display_vert_out out = {};
    out.m_9 = in.m_11;
    out.gl_Position = float4((fma(in.m_25, _18._m0.zw, _18._m0.xy) * 2.0) - float2(1.0), 0.0, 1.0);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

