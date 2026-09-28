#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _9
{
    float _m0;
};

struct _25
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

struct lightning_bolt_vert_out
{
    float m_8 [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct lightning_bolt_vert_in
{
    float3 m_37 [[attribute(0)]];
};

vertex lightning_bolt_vert_out lightning_bolt_vert(lightning_bolt_vert_in in [[stage_in]], constant _9& _11 [[buffer(0)]], constant _25& _27 [[buffer(1)]])
{
    lightning_bolt_vert_out out = {};
    out.m_8 = _11._m0;
    out.gl_Position = (_27._m1 * _27._m0) * float4(in.m_37, 1.0);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

