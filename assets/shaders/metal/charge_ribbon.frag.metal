#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct charge_ribbon_frag_out
{
    float4 m_43 [[color(0)]];
};

struct charge_ribbon_frag_in
{
    float m_36 [[user(locn0)]];
    float m_29 [[user(locn1)]];
    float m_21 [[user(locn2)]];
};

fragment charge_ribbon_frag_out charge_ribbon_frag(charge_ribbon_frag_in in [[stage_in]])
{
    charge_ribbon_frag_out out = {};
    out.m_43 = float4(mix(mix(float3(1.0, 0.5, 0.0), float3(1.0, 0.20000000298023223876953125, 0.0), float3(in.m_21)), float3(1.0, 0.800000011920928955078125, 0.300000011920928955078125), float3(in.m_29 * 0.5)), in.m_36 * smoothstep(0.0, 0.300000011920928955078125, in.m_21));
    return out;
}

