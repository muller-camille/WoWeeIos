#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _28
{
    float _m0;
    float _m1;
    float _m2;
    float _m3;
    float4 _m4;
};

struct weather_frag_out
{
    float4 m_43 [[color(0)]];
};

fragment weather_frag_out weather_frag(constant _28& _30 [[buffer(0)]], float2 gl_PointCoord [[point_coord]])
{
    weather_frag_out out = {};
    float _19 = length(gl_PointCoord - float2(0.5));
    if (_19 > 0.5)
    {
        discard_fragment();
    }
    out.m_43 = float4(_30._m4.xyz, _30._m4.w * smoothstep(0.5, 0.20000000298023223876953125, _19));
    return out;
}

