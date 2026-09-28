#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _25
{
    float4x4 _m0;
};

struct selection_circle_vert_out
{
    float2 m_9 [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct selection_circle_vert_in
{
    float3 m_12 [[attribute(0)]];
};

vertex selection_circle_vert_out selection_circle_vert(selection_circle_vert_in in [[stage_in]], constant _25& _27 [[buffer(0)]])
{
    selection_circle_vert_out out = {};
    out.m_9 = in.m_12.xy;
    out.gl_Position = _27._m0 * float4(in.m_12, 1.0);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

