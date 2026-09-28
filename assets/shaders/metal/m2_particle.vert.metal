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

struct m2_particle_vert_out
{
    float4 m_126 [[user(locn0)]];
    float m_130 [[user(locn1)]];
    float m_152 [[user(locn2)]];
    float4 gl_Position [[position]];
    float gl_PointSize [[point_size]];
};

struct m2_particle_vert_in
{
    float3 m_98 [[attribute(0)]];
    float4 m_128 [[attribute(1)]];
    float m_114 [[attribute(2)]];
    float m_131 [[attribute(3)]];
};

vertex m2_particle_vert_out m2_particle_vert(m2_particle_vert_in in [[stage_in]], constant _23& _25 [[buffer(0)]], texture3d<float> _79 [[texture(0)]], sampler _79Smplr [[sampler(0)]])
{
    m2_particle_vert_out out = {};
    float4 _103 = float4(in.m_98, 1.0);
    float4 _104 = _25._m0 * _103;
    out.gl_PointSize = fast::clamp((in.m_114 * 500.0) / fast::max(-_104.z, 1.0), 1.0, 128.0);
    out.m_126 = in.m_128;
    out.m_130 = in.m_131;
    out.m_152 = fast::clamp((_25._m8.y - length(_25._m6.xyz - in.m_98)) / fast::max(_25._m8.y - _25._m8.x, 0.001000000047497451305389404296875), 0.0, 1.0);
    if (_25._m15.x > 0.5)
    {
        float4 _193 = (_25._m1 * _25._m0) * _103;
        float _196 = fast::max(_193.w, 9.9999997473787516355514526367188e-05);
        out.m_152 *= _79.sample(_79Smplr, float3(((_193.xy / float2(_196)) * 0.5) + float2(0.5), (log(fast::max(_196, _25._m15.y) / _25._m15.y) * _25._m15.z) - (0.5 / _25._m15.w)), level(0.0)).w;
    }
    out.gl_Position = _25._m1 * _104;
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

