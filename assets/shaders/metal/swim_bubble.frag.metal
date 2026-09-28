#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct swim_bubble_frag_out
{
    float4 m_56 [[color(0)]];
};

struct swim_bubble_frag_in
{
    float m_51 [[user(locn0)]];
};

fragment swim_bubble_frag_out swim_bubble_frag(swim_bubble_frag_in in [[stage_in]], float2 gl_PointCoord [[point_coord]])
{
    swim_bubble_frag_out out = {};
    float _19 = length(gl_PointCoord - float2(0.5));
    if (_19 > 0.5)
    {
        discard_fragment();
    }
    out.m_56 = float4(0.800000011920928955078125, 0.89999997615814208984375, 1.0, fma(smoothstep(0.300000011920928955078125, 0.100000001490116119384765625, length(gl_PointCoord - float2(0.3499999940395355224609375, 0.64999997615814208984375))), 0.5, smoothstep(0.5, 0.4000000059604644775390625, _19) - smoothstep(0.37999999523162841796875, 0.2800000011920928955078125, _19)) * in.m_51);
    return out;
}

