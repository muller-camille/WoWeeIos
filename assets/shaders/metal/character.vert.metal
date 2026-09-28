#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _75
{
    float4x4 _m0[1];
};

struct _193
{
    float4x4 _m0;
};

struct _280
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
};

struct character_vert_out
{
    float3 m_213 [[user(locn0)]];
    float3 m_216 [[user(locn1)]];
    float2 m_222 [[user(locn2)]];
    float3 m_272 [[user(locn3)]];
    float3 m_274 [[user(locn4)]];
    float4 gl_Position [[position]];
};

struct character_vert_in
{
    float3 m_159 [[attribute(0)]];
    float4 m_88 [[attribute(1)]];
    uint4 m_65 [[attribute(2)]];
    float3 m_176 [[attribute(3)]];
    float2 m_224 [[attribute(4)]];
    float4 m_188 [[attribute(5)]];
};

vertex character_vert_out character_vert(character_vert_in in [[stage_in]], const device _75& _77 [[buffer(0)]], constant _193& _195 [[buffer(1)]], constant _280& _282 [[buffer(2)]])
{
    character_vert_out out = {};
    uint4 _69 = min(in.m_65, uint4(239u));
    float4x4 _92 = _77._m0[_69.x] * in.m_88.x;
    float4x4 _100 = _77._m0[_69.y] * in.m_88.y;
    float4x4 _120 = _77._m0[_69.z] * in.m_88.z;
    float4x4 _141 = _77._m0[_69.w] * in.m_88.w;
    float4 _144 = ((_92[0] + _100[0]) + _120[0]) + _141[0];
    float4 _147 = ((_92[1] + _100[1]) + _120[1]) + _141[1];
    float4 _150 = ((_92[2] + _100[2]) + _120[2]) + _141[2];
    float3x3 _175 = float3x3(_144.xyz, _147.xyz, _150.xyz);
    float4 _200 = _195._m0 * (float4x4(_144, _147, _150, ((_92[3] + _100[3]) + _120[3]) + _141[3]) * float4(in.m_159, 1.0));
    float3x3 _211 = float3x3(_195._m0[0].xyz, _195._m0[1].xyz, _195._m0[2].xyz);
    out.m_213 = _200.xyz;
    out.m_216 = _211 * (_175 * in.m_176);
    out.m_222 = in.m_224;
    float3 _513;
    do
    {
        float _309 = dot(out.m_216, out.m_216);
        if (_309 > 9.9999999392252902907785028219223e-09)
        {
            _513 = out.m_216 * rsqrt(_309);
            break;
        }
        _513 = float3(0.0, 0.0, 1.0);
        break;
    } while(false);
    float _344;
    bool _346;
    float3 _234 = _211 * (_175 * in.m_188.xyz);
    float3 _334 = cross(select(float3(0.0, 1.0, 0.0), float3(0.0, 0.0, 1.0), bool3(abs(_513.z) < 0.999000012874603271484375)), _513);
    float3 _514;
    do
    {
        _344 = dot(_334, _334);
        _346 = _344 > 9.9999999392252902907785028219223e-09;
        if (_346)
        {
            _514 = _334 * rsqrt(_344);
            break;
        }
        _514 = float3(1.0, 0.0, 0.0);
        break;
    } while(false);
    float3 _515;
    do
    {
        float _364 = dot(_234, _234);
        if (_364 > 9.9999999392252902907785028219223e-09)
        {
            _515 = _234 * rsqrt(_364);
            break;
        }
        _515 = _514;
        break;
    } while(false);
    float3 _247 = _515 - (_513 * dot(_515, _513));
    float3 _516;
    do
    {
        if (_346)
        {
            _516 = _334 * rsqrt(_344);
            break;
        }
        _516 = float3(1.0, 0.0, 0.0);
        break;
    } while(false);
    float3 _517;
    do
    {
        float _419 = dot(_247, _247);
        if (_419 > 9.9999999392252902907785028219223e-09)
        {
            _517 = _247 * rsqrt(_419);
            break;
        }
        _517 = _516;
        break;
    } while(false);
    float3 _260 = cross(_513, _517) * in.m_188.w;
    float3 _518;
    do
    {
        if (_346)
        {
            _518 = _334 * rsqrt(_344);
            break;
        }
        _518 = float3(1.0, 0.0, 0.0);
        break;
    } while(false);
    float3 _265 = cross(_513, _518);
    float3 _519;
    do
    {
        float _474 = dot(_265, _265);
        if (_474 > 9.9999999392252902907785028219223e-09)
        {
            _519 = _265 * rsqrt(_474);
            break;
        }
        _519 = float3(0.0, 1.0, 0.0);
        break;
    } while(false);
    float3 _520;
    do
    {
        float _494 = dot(_260, _260);
        if (_494 > 9.9999999392252902907785028219223e-09)
        {
            _520 = _260 * rsqrt(_494);
            break;
        }
        _520 = _519;
        break;
    } while(false);
    out.m_272 = _517;
    out.m_274 = _520;
    out.gl_Position = (_282._m1 * _282._m0) * _200;
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

