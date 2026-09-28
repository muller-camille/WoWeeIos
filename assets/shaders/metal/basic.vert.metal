#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _11
{
    float4x4 _m0;
};

struct _59
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

struct basic_vert_out
{
    float3 m_30 [[user(locn0)]];
    float3 m_33 [[user(locn1)]];
    float2 m_49 [[user(locn2)]];
    float4 gl_Position [[position]];
};

struct basic_vert_in
{
    float3 m_21 [[attribute(0)]];
    float3 m_44 [[attribute(1)]];
    float2 m_51 [[attribute(2)]];
};

vertex basic_vert_out basic_vert(basic_vert_in in [[stage_in]], constant _11& _13 [[buffer(0)]], constant _59& _61 [[buffer(1)]])
{
    basic_vert_out out = {};
    float4 _28 = _13._m0 * float4(in.m_21, 1.0);
    out.m_30 = _28.xyz;
    out.m_33 = float3x3(_13._m0[0].xyz, _13._m0[1].xyz, _13._m0[2].xyz) * in.m_44;
    out.m_49 = in.m_51;
    out.gl_Position = (_61._m1 * _61._m0) * _28;
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

