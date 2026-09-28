#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct m2_smoke_frag_out
{
    float4 m_54 [[color(0)]];
};

struct m2_smoke_frag_in
{
    float m_38 [[user(locn0)]];
    float m_27 [[user(locn1)]];
};

fragment m2_smoke_frag_out m2_smoke_frag(m2_smoke_frag_in in [[stage_in]], float2 gl_PointCoord [[point_coord]])
{
    m2_smoke_frag_out out = {};
    float _19 = length(gl_PointCoord - float2(0.5));
    if (_19 > 0.5)
    {
        discard_fragment();
    }
    if (in.m_27 > 0.5)
    {
        float _35 = smoothstep(0.5, 0.0, _19);
        out.m_54 = float4(mix(float3(1.0, 0.60000002384185791015625, 0.100000001490116119384765625), float3(1.0, 0.20000000298023223876953125, 0.0), float3(in.m_38)) * _35, _35 * (1.0 - in.m_38));
    }
    else
    {
        out.m_54 = float4(0.5, 0.5, 0.5, ((smoothstep(0.5, 0.300000011920928955078125, _19) * smoothstep(0.0, 0.20000000298023223876953125, in.m_38)) * (1.0 - smoothstep(0.60000002384185791015625, 1.0, in.m_38))) * 0.4000000059604644775390625);
    }
    return out;
}

