#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _26
{
    float4x4 _m0;
    float4 _m1;
};

struct footprint_frag_out
{
    float4 m_43 [[color(0)]];
};

struct footprint_frag_in
{
    float2 m_16 [[user(locn0)]];
};

fragment footprint_frag_out footprint_frag(footprint_frag_in in [[stage_in]], constant _26& _28 [[buffer(0)]], texture2d<float> _12 [[texture(0)]], sampler _12Smplr [[sampler(0)]])
{
    footprint_frag_out out = {};
    float4 _19 = _12.sample(_12Smplr, in.m_16);
    float _34 = _19.w * _28._m1.w;
    if (_34 < 0.014999999664723873138427734375)
    {
        discard_fragment();
    }
    out.m_43 = float4(_28._m1.xyz, _34);
    return out;
}

