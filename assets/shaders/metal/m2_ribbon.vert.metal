#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _23
{
    float4x4 _m0;
    float4x4 _m1;
    float4x4 _m2;
    float4 _m3;
    float4 _m4;
    float4 _m5;
    float4 _m6;
    float4 _m7;
    float4 _m8;
    float4 _m9;
    float4 _m10;
    float4 _m11;
    float4 _m12[64];
    float4 _m13[64];
    int4 _m14;
    float4 _m15;
};

struct m2_ribbon_vert_out
{
    float3 m_154 [[user(locn0)]];
    float m_157 [[user(locn1)]];
    float2 m_162 [[user(locn2)]];
    float m_130 [[user(locn3)]];
    float4 gl_Position [[position]];
};

struct m2_ribbon_vert_in
{
    float3 m_96 [[attribute(0)]];
    float3 m_155 [[attribute(1)]];
    float m_159 [[attribute(2)]];
    float2 m_164 [[attribute(3)]];
};

vertex m2_ribbon_vert_out m2_ribbon_vert(m2_ribbon_vert_in in [[stage_in]], constant _23& _25 [[buffer(0)]], texture3d<float> _79 [[texture(0)]], sampler _79Smplr [[sampler(0)]])
{
    m2_ribbon_vert_out out = {};
    float4 _101 = float4(in.m_96, 1.0);
    float4 _106 = _25._m0 * _101;
    out.gl_Position = _25._m1 * _106;
    out.m_130 = fast::clamp((_25._m8.y - length(_106.xyz)) / fast::max(_25._m8.y - _25._m8.x, 0.001000000047497451305389404296875), 0.0, 1.0);
    if (_25._m15.x > 0.5)
    {
        float4 _182 = (_25._m1 * _25._m0) * _101;
        float _185 = fast::max(_182.w, 9.9999997473787516355514526367188e-05);
        out.m_130 *= _79.sample(_79Smplr, float3(((_182.xy / float2(_185)) * 0.5) + float2(0.5), (log(fast::max(_185, _25._m15.y) / _25._m15.y) * _25._m15.z) - (0.5 / _25._m15.w)), level(0.0)).w;
    }
    out.m_154 = in.m_155;
    out.m_157 = in.m_159;
    out.m_162 = in.m_164;
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

