#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct m2_ribbon_frag_out
{
    float4 m_51 [[color(0)]];
};

struct m2_ribbon_frag_in
{
    float3 m_43 [[user(locn0)]];
    float m_27 [[user(locn1)]];
    float2 m_17 [[user(locn2)]];
    float m_46 [[user(locn3)]];
};

fragment m2_ribbon_frag_out m2_ribbon_frag(m2_ribbon_frag_in in [[stage_in]], texture2d<float> _13 [[texture(0)]], sampler _13Smplr [[sampler(0)]])
{
    m2_ribbon_frag_out out = {};
    float4 _19 = _13.sample(_13Smplr, in.m_17);
    float _29 = _19.w * in.m_27;
    if (_29 < 0.00999999977648258209228515625)
    {
        discard_fragment();
    }
    out.m_51 = float4((_19.xyz * in.m_43) * in.m_46, _29);
    return out;
}

