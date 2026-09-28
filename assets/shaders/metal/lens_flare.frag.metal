#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _46
{
    float2 _m0;
    float _m1;
    float _m2;
    float4 _m3;
};

struct lens_flare_frag_out
{
    float4 m_65 [[color(0)]];
};

struct lens_flare_frag_in
{
    float2 m_11 [[user(locn0)]];
};

fragment lens_flare_frag_out lens_flare_frag(lens_flare_frag_in in [[stage_in]], constant _46& _48 [[buffer(0)]])
{
    lens_flare_frag_out out = {};
    float _19 = length(in.m_11 - float2(0.5));
    float _56 = (fast::max(smoothstep(0.5, 0.0, _19), exp(((-_19) * _19) * 8.0) * 0.5) * (1.0 - smoothstep(0.300000011920928955078125, 0.4799999892711639404296875, _19))) * _48._m3.w;
    if (_56 < 0.0040000001899898052215576171875)
    {
        discard_fragment();
    }
    out.m_65 = float4(_48._m3.xyz, _56);
    return out;
}

