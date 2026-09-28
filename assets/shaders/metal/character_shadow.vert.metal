#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _22
{
    float4x4 _m0[1];
};

struct _126
{
    float4x4 _m0;
    float4 _m1;
};

struct character_shadow_vert_out
{
    float2 m_118 [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct character_shadow_vert_in
{
    float3 m_108 [[attribute(0)]];
    float4 m_35 [[attribute(1)]];
    uint4 m_11 [[attribute(2)]];
    float2 m_120 [[attribute(3)]];
};

vertex character_shadow_vert_out character_shadow_vert(character_shadow_vert_in in [[stage_in]], const device _22& _24 [[buffer(0)]], constant _126& _128 [[buffer(1)]])
{
    character_shadow_vert_out out = {};
    uint4 _15 = min(in.m_11, uint4(239u));
    float4x4 _39 = _24._m0[_15.x] * in.m_35.x;
    float4x4 _47 = _24._m0[_15.y] * in.m_35.y;
    float4x4 _68 = _24._m0[_15.z] * in.m_35.z;
    float4x4 _89 = _24._m0[_15.w] * in.m_35.w;
    out.m_118 = in.m_120;
    out.gl_Position = _128._m0 * (float4x4(((_39[0] + _47[0]) + _68[0]) + _89[0], ((_39[1] + _47[1]) + _68[1]) + _89[1], ((_39[2] + _47[2]) + _68[2]) + _89[2], ((_39[3] + _47[3]) + _68[3]) + _89[3]) * float4(in.m_108, 1.0));
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

