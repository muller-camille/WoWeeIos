#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct world_map_frag_out
{
    float4 m_9 [[color(0)]];
};

struct world_map_frag_in
{
    float2 m_17 [[user(locn0)]];
};

fragment world_map_frag_out world_map_frag(world_map_frag_in in [[stage_in]], texture2d<float> _13 [[texture(0)]], sampler _13Smplr [[sampler(0)]])
{
    world_map_frag_out out = {};
    out.m_9 = _13.sample(_13Smplr, in.m_17);
    return out;
}

