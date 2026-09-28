#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct lightning_bolt_frag_out
{
    float4 m_24 [[color(0)]];
};

struct lightning_bolt_frag_in
{
    float m_16 [[user(locn0)]];
};

fragment lightning_bolt_frag_out lightning_bolt_frag(lightning_bolt_frag_in in [[stage_in]])
{
    lightning_bolt_frag_out out = {};
    out.m_24 = float4(mix(float3(0.60000002384185791015625, 0.800000011920928955078125, 1.0), float3(1.0), float3(in.m_16 * 0.5)), in.m_16);
    return out;
}

