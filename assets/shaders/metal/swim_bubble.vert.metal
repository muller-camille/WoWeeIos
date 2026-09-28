#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _26
{
    float4x4 _m0;
    float4x4 _m1;
    float4x4 _m2;
    float4 _m3;
    float4 _m4;
    float4 _m5;
    float4 _m6;
    float4 _m7;
    float4 _m8;
    float4 _m9;
};

struct swim_bubble_vert_out
{
    float m_21 [[user(locn0)]];
    float4 gl_Position [[position]];
    float gl_PointSize [[point_size]];
};

struct swim_bubble_vert_in
{
    float3 m_37 [[attribute(0)]];
    float m_17 [[attribute(1)]];
    float m_22 [[attribute(2)]];
};

vertex swim_bubble_vert_out swim_bubble_vert(swim_bubble_vert_in in [[stage_in]], constant _26& _28 [[buffer(0)]])
{
    swim_bubble_vert_out out = {};
    out.gl_PointSize = in.m_17;
    out.m_21 = in.m_22;
    out.gl_Position = (_28._m1 * _28._m0) * float4(in.m_37, 1.0);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

