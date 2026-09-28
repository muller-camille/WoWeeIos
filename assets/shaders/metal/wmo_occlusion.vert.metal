#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _17
{
    float4x4 _m0;
};

struct wmo_occlusion_vert_out
{
    float4 gl_Position [[position]];
};

struct wmo_occlusion_vert_in
{
    float3 m_25 [[attribute(0)]];
};

vertex wmo_occlusion_vert_out wmo_occlusion_vert(wmo_occlusion_vert_in in [[stage_in]], constant _17& _19 [[buffer(0)]])
{
    wmo_occlusion_vert_out out = {};
    out.gl_Position = _19._m0 * float4(in.m_25, 1.0);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

