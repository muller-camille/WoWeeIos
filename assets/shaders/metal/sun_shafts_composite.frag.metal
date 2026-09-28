#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct sun_shafts_composite_frag_out
{
    float4 m_9 [[color(0)]];
};

struct sun_shafts_composite_frag_in
{
    float2 m_17 [[user(locn0)]];
};

fragment sun_shafts_composite_frag_out sun_shafts_composite_frag(sun_shafts_composite_frag_in in [[stage_in]], texture2d<float> _13 [[texture(0)]], sampler _13Smplr [[sampler(0)]])
{
    sun_shafts_composite_frag_out out = {};
    out.m_9 = float4(_13.sample(_13Smplr, in.m_17).xyz, 0.0);
    return out;
}

