#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _16
{
    float _m0;
};

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

struct weather_vert_out
{
    float4 gl_Position [[position]];
    float gl_PointSize [[point_size]];
};

struct weather_vert_in
{
    float3 m_37 [[attribute(0)]];
};

vertex weather_vert_out weather_vert(weather_vert_in in [[stage_in]], constant _16& _18 [[buffer(0)]], constant _26& _28 [[buffer(1)]])
{
    weather_vert_out out = {};
    out.gl_PointSize = _18._m0;
    out.gl_Position = (_28._m1 * _28._m0) * float4(in.m_37, 1.0);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

