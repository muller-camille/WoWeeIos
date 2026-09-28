#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct mount_dust_frag_out
{
    float4 m_38 [[color(0)]];
};

struct mount_dust_frag_in
{
    float m_31 [[user(locn0)]];
};

fragment mount_dust_frag_out mount_dust_frag(mount_dust_frag_in in [[stage_in]], float2 gl_PointCoord [[point_coord]])
{
    mount_dust_frag_out out = {};
    float _19 = length(gl_PointCoord - float2(0.5));
    if (_19 > 0.5)
    {
        discard_fragment();
    }
    out.m_38 = float4(0.699999988079071044921875, 0.64999997615814208984375, 0.550000011920928955078125, (smoothstep(0.5, 0.100000001490116119384765625, _19) * in.m_31) * 0.4000000059604644775390625);
    return out;
}

