#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _47
{
    float4x4 _m0;
    float _m1;
    float _m2;
};

struct quest_marker_frag_out
{
    float4 m_58 [[color(0)]];
};

struct quest_marker_frag_in
{
    float2 m_17 [[user(locn0)]];
};

fragment quest_marker_frag_out quest_marker_frag(quest_marker_frag_in in [[stage_in]], constant _47& _49 [[buffer(0)]], texture2d<float> _13 [[texture(0)]], sampler _13Smplr [[sampler(0)]])
{
    quest_marker_frag_out out = {};
    float4 _19 = _13.sample(_13Smplr, in.m_17);
    float _24 = _19.w;
    if (_24 < 0.100000001490116119384765625)
    {
        discard_fragment();
    }
    float3 _34 = _19.xyz;
    out.m_58 = float4(mix(_34, float3(dot(_34, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625))), float3(_49._m2)), _24 * _49._m1);
    return out;
}

