#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _17
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

struct _57
{
    float4x4 _m0;
    float4 _m1;
    float _m2;
    float _m3;
    float _m4;
};

struct celestial_vert_out
{
    float2 m_9 [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct celestial_vert_in
{
    float3 m_65 [[attribute(0)]];
    float2 m_11 [[attribute(1)]];
};

vertex celestial_vert_out celestial_vert(celestial_vert_in in [[stage_in]], constant _17& _19 [[buffer(0)]], constant _57& _59 [[buffer(1)]])
{
    celestial_vert_out out = {};
    out.m_9 = in.m_11;
    out.gl_Position = float4((((_19._m1 * float4x4(float4(_19._m0[0].xyz, 0.0), float4(_19._m0[1].xyz, 0.0), float4(_19._m0[2].xyz, 0.0), float4(0.0, 0.0, 0.0, 1.0))) * _59._m0) * float4(in.m_65, 1.0)).xyww);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

