#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _28
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

struct charge_ribbon_vert_out
{
    float m_8 [[user(locn0)]];
    float m_12 [[user(locn1)]];
    float m_15 [[user(locn2)]];
    float4 gl_Position [[position]];
};

struct charge_ribbon_vert_in
{
    float3 m_40 [[attribute(0)]];
    float m_10 [[attribute(1)]];
    float m_13 [[attribute(2)]];
    float m_16 [[attribute(3)]];
};

vertex charge_ribbon_vert_out charge_ribbon_vert(charge_ribbon_vert_in in [[stage_in]], constant _28& _30 [[buffer(0)]])
{
    charge_ribbon_vert_out out = {};
    out.m_8 = in.m_10;
    out.m_12 = in.m_13;
    out.m_15 = in.m_16;
    out.gl_Position = (_30._m1 * _30._m0) * float4(in.m_40, 1.0);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

