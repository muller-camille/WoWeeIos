#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _23
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

struct _33
{
    float4x4 _m0;
};

struct quest_marker_vert_out
{
    float2 m_9 [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct quest_marker_vert_in
{
    float3 m_42 [[attribute(0)]];
    float2 m_11 [[attribute(1)]];
};

vertex quest_marker_vert_out quest_marker_vert(quest_marker_vert_in in [[stage_in]], constant _23& _25 [[buffer(0)]], constant _33& _35 [[buffer(1)]])
{
    quest_marker_vert_out out = {};
    out.m_9 = in.m_11;
    out.gl_Position = ((_25._m1 * _25._m0) * _35._m0) * float4(in.m_42, 1.0);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

