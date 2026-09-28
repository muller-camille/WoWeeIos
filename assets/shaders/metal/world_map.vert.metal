#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _13
{
    float2 _m0;
    float _m1;
    float _m2;
    float2 _m3;
    float2 _m4;
};

struct world_map_vert_out
{
    float2 m_9 [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct world_map_vert_in
{
    float2 m_24 [[attribute(0)]];
    float2 m_11 [[attribute(1)]];
};

vertex world_map_vert_out world_map_vert(world_map_vert_in in [[stage_in]], constant _13& _15 [[buffer(0)]])
{
    world_map_vert_out out = {};
    out.m_9 = in.m_11 * _15._m4;
    out.gl_Position = float4(((((in.m_24 * _15._m3) + _15._m0) / float2(_15._m1, _15._m2)) * 2.0) - float2(1.0), 0.0, 1.0);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

