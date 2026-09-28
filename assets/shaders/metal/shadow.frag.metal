#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _11
{
    float4x4 _m0;
    float4 _m1;
    int4 _m2;
    float4 _m3;
};

struct shadow_frag_in
{
    float2 m_34 [[user(locn0)]];
};

fragment void shadow_frag(shadow_frag_in in [[stage_in]], constant _11& _13 [[buffer(0)]], texture2d<float> _30 [[texture(0)]], sampler _30Smplr [[sampler(0)]])
{
    if (_13._m2.x != 0)
    {
        bool _41 = _13._m2.y != 0;
        bool _50;
        if (_41)
        {
            _50 = _30.sample(_30Smplr, in.m_34, level(0.0)).w < 0.5;
        }
        else
        {
            _50 = _41;
        }
        if (_50)
        {
            discard_fragment();
        }
    }
}

