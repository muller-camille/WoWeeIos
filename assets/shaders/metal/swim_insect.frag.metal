#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct swim_insect_frag_out
{
    float4 m_36 [[color(0)]];
};

struct swim_insect_frag_in
{
    float m_31 [[user(locn0)]];
};

fragment swim_insect_frag_out swim_insect_frag(swim_insect_frag_in in [[stage_in]], float2 gl_PointCoord [[point_coord]])
{
    swim_insect_frag_out out = {};
    float _19 = length(gl_PointCoord - float2(0.5));
    if (_19 > 0.5)
    {
        discard_fragment();
    }
    out.m_36 = float4(0.119999997317790985107421875, 0.07999999821186065673828125, 0.0500000007450580596923828125, smoothstep(0.5, 0.0500000007450580596923828125, _19) * in.m_31);
    return out;
}

