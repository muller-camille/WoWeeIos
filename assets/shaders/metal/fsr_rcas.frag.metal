#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _11
{
    float4 _m0;
    float4 _m1;
};

struct fsr_rcas_frag_out
{
    float4 m_143 [[color(0)]];
};

struct fsr_rcas_frag_in
{
    float2 m_29 [[user(locn0)]];
};

fragment fsr_rcas_frag_out fsr_rcas_frag(fsr_rcas_frag_in in [[stage_in]], constant _11& _13 [[buffer(0)]], texture2d<float> _26 [[texture(0)]], sampler _26Smplr [[sampler(0)]])
{
    fsr_rcas_frag_out out = {};
    float3 _32 = _26.sample(_26Smplr, in.m_29).xyz;
    float4 _45 = _26.sample(_26Smplr, (in.m_29 + float2(0.0, -_13._m0.y)));
    float4 _54 = _26.sample(_26Smplr, (in.m_29 + float2(0.0, _13._m0.y)));
    float4 _65 = _26.sample(_26Smplr, (in.m_29 + float2(-_13._m0.x, 0.0)));
    float4 _74 = _26.sample(_26Smplr, (in.m_29 + float2(_13._m0.x, 0.0)));
    float _81 = _45.y;
    float _84 = _54.y;
    float _87 = _65.y;
    float _90 = _74.y;
    out.m_143 = float4(fast::clamp(_32 + ((_32 - ((((_45.xyz + _54.xyz) + _65.xyz) + _74.xyz) * 0.25)) * (_13._m1.x * (1.0 - smoothstep(0.0, 0.300000011920928955078125, fast::max(fast::max(_81, _84), fast::max(_87, _90)) - fast::min(fast::min(_81, _84), fast::min(_87, _90)))))), float3(0.0), float3(1.0)), 1.0);
    return out;
}

