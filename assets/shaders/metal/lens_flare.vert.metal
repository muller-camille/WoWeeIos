#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _17
{
    float2 _m0;
    float _m1;
    float _m2;
};

struct lens_flare_vert_out
{
    float2 m_9 [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct lens_flare_vert_in
{
    float2 m_15 [[attribute(0)]];
    float2 m_11 [[attribute(1)]];
};

vertex lens_flare_vert_out lens_flare_vert(lens_flare_vert_in in [[stage_in]], constant _17& _19 [[buffer(0)]])
{
    lens_flare_vert_out out = {};
    out.m_9 = in.m_11;
    float2 _25 = in.m_15 * _19._m1;
    _25.x = _25.x / _19._m2;
    out.gl_Position = float4(_25 + _19._m0, 0.0, 1.0);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

