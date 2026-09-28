#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct starfield_frag_out
{
    float4 m_50 [[color(0)]];
};

struct starfield_frag_in
{
    float m_56 [[user(locn0)]];
    float3 m_53 [[user(locn1)]];
};

fragment starfield_frag_out starfield_frag(starfield_frag_in in [[stage_in]], float2 gl_PointCoord [[point_coord]])
{
    starfield_frag_out out = {};
    float _16 = length(gl_PointCoord - float2(0.5));
    float _18 = _16 * 2.0;
    if (_18 > 1.0)
    {
        discard_fragment();
    }
    float _30 = (_16 * (-2.0)) * _18;
    out.m_50 = float4((in.m_53 * in.m_56) * fma(0.0350000001490116119384765625, exp(_30 * 3.0), exp(_30 * 26.0)), 1.0);
    return out;
}

