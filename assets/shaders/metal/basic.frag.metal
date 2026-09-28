#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _20
{
    float4 _m0;
    packed_float3 _m1;
    int _m2;
};

struct _45
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

struct basic_frag_out
{
    float4 m_89 [[color(0)]];
};

struct basic_frag_in
{
    float3 m_27 [[user(locn0)]];
    float3 m_14 [[user(locn1)]];
    float2 m_97 [[user(locn2)]];
};

fragment basic_frag_out basic_frag(basic_frag_in in [[stage_in]], constant _20& _22 [[buffer(0)]], constant _45& _47 [[buffer(1)]], texture2d<float> _93 [[texture(0)]], sampler _93Smplr [[sampler(0)]])
{
    basic_frag_out out = {};
    float3 _16 = fast::normalize(in.m_14);
    float3 _30 = fast::normalize(float3(_22._m1) - in.m_27);
    float3 _78 = (float3(0.300000011920928955078125) + (float3(1.0) * fast::max(dot(_16, _30), 0.0))) + (float3(1.0) * (0.5 * pow(fast::max(dot(fast::normalize(_47._m6.xyz - in.m_27), reflect(-_30, _16)), 0.0), 32.0)));
    if (_22._m2 != 0)
    {
        out.m_89 = _93.sample(_93Smplr, in.m_97) * float4(_78, 1.0);
    }
    else
    {
        out.m_89 = _22._m0 * float4(_78, 1.0);
    }
    return out;
}

