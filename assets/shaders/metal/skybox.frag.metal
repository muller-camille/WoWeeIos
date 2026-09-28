#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _57
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

struct _93
{
    float4 _m0;
    float4 _m1;
    float4 _m2;
    float4 _m3;
    float4 _m4;
};

struct skybox_frag_out
{
    float4 m_309 [[color(0)]];
};

struct skybox_frag_in
{
    float2 m_32 [[user(locn0)]];
};

fragment skybox_frag_out skybox_frag(skybox_frag_in in [[stage_in]], constant _57& _59 [[buffer(0)]], constant _93& _95 [[buffer(1)]], texture3d<float> _17 [[texture(0)]], sampler _17Smplr [[sampler(0)]])
{
    skybox_frag_out out = {};
    float3 _91 = fast::normalize(transpose(float3x3(_59._m0[0].xyz, _59._m0[1].xyz, _59._m0[2].xyz)) * float3(((in.m_32.x * 2.0) - 1.0) / _59._m1[0].x, (1.0 - (in.m_32.y * 2.0)) / abs(_59._m1[1].y), -1.0));
    float _109 = _91.z;
    float _112 = fast::clamp(_109, 0.0, 1.0);
    float3 _329;
    if (_112 > 0.4000000059604644775390625)
    {
        _329 = mix(_95._m1.xyz, _95._m0.xyz, float3((_112 - 0.4000000059604644775390625) * 1.66666662693023681640625));
    }
    else
    {
        float3 _330;
        if (_112 > 0.0500000007450580596923828125)
        {
            _330 = mix(_95._m2.xyz, _95._m1.xyz, float3((_112 - 0.0500000007450580596923828125) * 2.857142925262451171875));
        }
        else
        {
            _330 = mix(_95._m3.xyz, _95._m2.xyz, float3(_112 * 20.0));
        }
        _329 = _330;
    }
    float3 _328;
    if (_109 < 0.0)
    {
        _328 = mix(_95._m3.xyz, _95._m3.xyz * 0.300000011920928955078125, float3(fast::clamp(_109 * (-3.0), 0.0, 1.0)));
    }
    else
    {
        _328 = _329;
    }
    float _196 = fast::max(dot(_91, _95._m4.xyz), 0.0);
    float _200 = fast::clamp(_95._m4.z, 0.0, 1.0);
    float3 _214 = float3(_112);
    float3 _270 = ((_328 + (((mix(float3(0.800000011920928955078125, 0.449999988079071044921875, 0.1500000059604644775390625), float3(0.300000011920928955078125, 0.5, 1.0), _214) * (pow(1.0 - _112, 3.0) * 0.1500000059604644775390625)) * _196) * _200)) + ((mix(float3(1.0, 0.85000002384185791015625, 0.550000011920928955078125), float3(1.0, 1.0, 0.949999988079071044921875), _214) * ((pow(_196, 64.0) * 0.4000000059604644775390625) + (pow(_196, 8.0) * 0.100000001490116119384765625))) * _200)) + ((_95._m2.xyz * (exp(_112 * (-12.0)) * 0.0599999986588954925537109375)) * _200);
    float3 _331;
    if (_95._m4.z < 0.0)
    {
        _331 = _270 + (float3(0.0199999995529651641845703125, 0.02999999932944774627685546875, 0.07999999821186065673828125) * fast::clamp(_95._m4.z * (-0.5), 0.0, 0.1500000059604644775390625));
    }
    else
    {
        _331 = _270;
    }
    float3 _332;
    if (_59._m15.x > 0.5)
    {
        float4 _322 = _17.sample(_17Smplr, float3(in.m_32, 1.0), level(0.0));
        _332 = (_331 * _322.w) + _322.xyz;
    }
    else
    {
        _332 = _331;
    }
    out.m_309 = float4(_332, 1.0);
    return out;
}

