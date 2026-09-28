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

struct clouds_vert_out
{
    float3 m_9 [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct clouds_vert_in
{
    float3 m_11 [[attribute(0)]];
};

vertex clouds_vert_out clouds_vert(clouds_vert_in in [[stage_in]], constant _17& _19 [[buffer(0)]])
{
    clouds_vert_out out = {};
    out.m_9 = in.m_11;
    out.gl_Position = ((_19._m1 * float4x4(float4(_19._m0[0].xyz, 0.0), float4(_19._m0[1].xyz, 0.0), float4(_19._m0[2].xyz, 0.0), float4(0.0, 0.0, 0.0, 1.0))) * float4(in.m_11, 1.0)).xyww;
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

