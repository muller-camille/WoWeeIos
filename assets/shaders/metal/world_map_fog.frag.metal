#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _23
{
    char _m0_pad[32];
    float4 _m0;
};

struct world_map_fog_frag_out
{
    float4 m_21 [[color(0)]];
};

struct world_map_fog_frag_in
{
    float2 m_17 [[user(locn0)]];
};

fragment world_map_fog_frag_out world_map_fog_frag(world_map_fog_frag_in in [[stage_in]], constant _23& _25 [[buffer(0)]], texture2d<float> _13 [[texture(0)]], sampler _13Smplr [[sampler(0)]])
{
    world_map_fog_frag_out out = {};
    out.m_21 = _13.sample(_13Smplr, in.m_17) * _25._m0;
    return out;
}

