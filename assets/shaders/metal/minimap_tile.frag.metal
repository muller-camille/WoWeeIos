#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct minimap_tile_frag_out
{
    float4 m_9 [[color(0)]];
};

struct minimap_tile_frag_in
{
    float2 m_17 [[user(locn0)]];
};

fragment minimap_tile_frag_out minimap_tile_frag(minimap_tile_frag_in in [[stage_in]], texture2d<float> _13 [[texture(0)]], sampler _13Smplr [[sampler(0)]])
{
    minimap_tile_frag_out out = {};
    out.m_9 = _13.sample(_13Smplr, float2(in.m_17.y, in.m_17.x));
    return out;
}

