#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Implementation of the GLSL mod() function, which is slightly different than Metal fmod()
template<typename Tx, typename Ty>
inline Tx mod(Tx x, Ty y)
{
    return x - y * floor(x / y);
}

struct _22
{
    float2 _m0;
    int _m1;
};

struct m2_particle_frag_out
{
    float4 m_115 [[color(0)]];
};

struct m2_particle_frag_in
{
    float4 m_96 [[user(locn0)]];
    float m_16 [[user(locn1)]];
    float m_102 [[user(locn2)]];
};

fragment m2_particle_frag_out m2_particle_frag(m2_particle_frag_in in [[stage_in]], constant _22& _24 [[buffer(0)]], texture2d<float> _54 [[texture(0)]], sampler _54Smplr [[sampler(0)]], float2 gl_PointCoord [[point_coord]])
{
    m2_particle_frag_out out = {};
    float _18 = floor(in.m_16);
    float4 _57 = _54.sample(_54Smplr, ((float2(mod(_18, _24._m0.x), floor(_18 / _24._m0.x)) + gl_PointCoord) / _24._m0));
    if (_24._m1 != 0)
    {
        if (dot(_57.xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625)) < 0.0500000007450580596923828125)
        {
            discard_fragment();
        }
    }
    float _104 = ((_57.w * in.m_96.w) * (1.0 - smoothstep(0.4000000059604644775390625, 0.5, length(gl_PointCoord - float2(0.5))))) * in.m_102;
    out.m_115 = float4((_57.xyz * in.m_96.xyz) * _104, _104);
    return out;
}

