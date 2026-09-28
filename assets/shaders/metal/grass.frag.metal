#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _30
{
    float4x4 _m0;
    float4x4 _m1;
    char _m2_pad[64];
    float4 _m2;
    float4 _m3;
    float4 _m4;
    float4 _m5;
    float4 _m6;
    float4 _m7;
    char _m8_pad[2112];
    float4 _m8;
};

struct grass_frag_out
{
    float4 m_244 [[color(0)]];
};

struct grass_frag_in
{
    float m_150 [[user(locn0)]];
    float3 m_186 [[user(locn1)]];
    float3 m_145 [[user(locn2)]];
    float3 m_147 [[user(locn3)]];
    float4 m_158 [[user(locn4)]];
    float4 m_174 [[user(locn5)]];
    float3 m_229 [[user(locn6)]];
};

fragment grass_frag_out grass_frag(grass_frag_in in [[stage_in]], constant _30& _32 [[buffer(0)]], texture3d<float> _85 [[texture(0)]], sampler _85Smplr [[sampler(0)]], bool gl_FrontFacing [[front_facing]])
{
    grass_frag_out out = {};
    float3 _272 = mix(_32._m6.xyz, (mix(mix(mix(in.m_145, in.m_147, float3(in.m_150 * in.m_150)), in.m_158.xyz, float3(in.m_158.w * mix(0.3499999940395355224609375, 0.0, smoothstep(0.0, 0.449999988079071044921875, in.m_150)))), in.m_174.xyz, float3(in.m_174.w * smoothstep(0.5, 0.7200000286102294921875, in.m_150))) * (_32._m4.xyz + (_32._m3.xyz * fast::clamp((dot(fast::normalize(in.m_186) * (gl_FrontFacing ? 1.0 : (-1.0)), fast::normalize(-_32._m2.xyz)) * 0.5) + 0.5, 0.0, 1.0)))) * mix(0.550000011920928955078125, 1.0, in.m_150), float3(fast::clamp((_32._m7.y - length(_32._m5.xyz - in.m_229)) / (_32._m7.y - _32._m7.x), 0.0, 1.0)));
    float3 _338;
    if (_32._m8.x > 0.5)
    {
        float4 _304 = (_32._m1 * _32._m0) * float4(in.m_229, 1.0);
        float _307 = fast::max(_304.w, 9.9999997473787516355514526367188e-05);
        float4 _337 = _85.sample(_85Smplr, float3(((_304.xy / float2(_307)) * 0.5) + float2(0.5), (log(fast::max(_307, _32._m8.y) / _32._m8.y) * _32._m8.z) - (0.5 / _32._m8.w)), level(0.0));
        _338 = (_272 * _337.w) + _337.xyz;
    }
    else
    {
        _338 = _272;
    }
    out.m_244 = float4(_338, 1.0);
    return out;
}

