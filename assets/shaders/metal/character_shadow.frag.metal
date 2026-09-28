#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _22
{
    int _m0;
    int _m1;
};

struct character_shadow_frag_in
{
    float2 m_17 [[user(locn0)]];
};

fragment void character_shadow_frag(character_shadow_frag_in in [[stage_in]], constant _22& _24 [[buffer(0)]], texture2d<float> _13 [[texture(0)]], sampler _13Smplr [[sampler(0)]])
{
    float4 _19 = _13.sample(_13Smplr, in.m_17);
    bool _29 = _24._m0 != 0;
    bool _39;
    if (_29)
    {
        _39 = _19.w < 0.5;
    }
    else
    {
        _39 = _29;
    }
    if (_39)
    {
        discard_fragment();
    }
    if (_24._m1 != 0)
    {
        if (dot(_19.xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625)) < 0.119999997317790985107421875)
        {
            discard_fragment();
        }
    }
}

