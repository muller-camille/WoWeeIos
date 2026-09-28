#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _11
{
    float4x4 _m0;
};

struct _51
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

struct terrain_vert_out
{
    float3 m_30 [[user(locn0)]];
    float3 m_33 [[user(locn1)]];
    float2 m_38 [[user(locn2)]];
    float2 m_42 [[user(locn3)]];
    float4 gl_Position [[position]];
};

struct terrain_vert_in
{
    float3 m_21 [[attribute(0)]];
    float3 m_34 [[attribute(1)]];
    float2 m_40 [[attribute(2)]];
    float2 m_43 [[attribute(3)]];
};

vertex terrain_vert_out terrain_vert(terrain_vert_in in [[stage_in]], constant _11& _13 [[buffer(0)]], constant _51& _53 [[buffer(1)]])
{
    terrain_vert_out out = {};
    float4 _28 = _13._m0 * float4(in.m_21, 1.0);
    out.m_30 = _28.xyz;
    out.m_33 = in.m_34;
    out.m_38 = in.m_40;
    out.m_42 = in.m_43;
    out.gl_Position = (_53._m1 * _53._m0) * _28;
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

