#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct swim_ripple_frag_out
{
    float4 m_36 [[color(0)]];
};

struct swim_ripple_frag_in
{
    float m_31 [[user(locn0)]];
};

fragment swim_ripple_frag_out swim_ripple_frag(swim_ripple_frag_in in [[stage_in]], float2 gl_PointCoord [[point_coord]])
{
    swim_ripple_frag_out out = {};
    float _19 = length(gl_PointCoord - float2(0.5));
    if (_19 > 0.5)
    {
        discard_fragment();
    }
    out.m_36 = float4(0.85000002384185791015625, 0.920000016689300537109375, 1.0, smoothstep(0.5, 0.100000001490116119384765625, _19) * in.m_31);
    return out;
}

