#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _15
{
    float4x4 _m0;
    float4 _m1;
};

struct _58
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

struct wmo_vert_out
{
    float3 m_167 [[user(locn0)]];
    float3 m_182 [[user(locn1)]];
    float2 m_187 [[user(locn2)]];
    float4 m_192 [[user(locn3)]];
    float3 m_223 [[user(locn4)]];
    float3 m_225 [[user(locn5)]];
    float4 gl_Position [[position]];
};

struct wmo_vert_in
{
    float3 m_11 [[attribute(0)]];
    float3 m_131 [[attribute(1)]];
    float2 m_189 [[attribute(2)]];
    float4 m_194 [[attribute(3)]];
    float4 m_198 [[attribute(4)]];
};

vertex wmo_vert_out wmo_vert(wmo_vert_in in [[stage_in]], constant _15& _17 [[buffer(0)]], constant _58& _60 [[buffer(1)]])
{
    wmo_vert_out out = {};
    float3 _244;
    if (_17._m1.y > 0.0)
    {
        float _35 = fast::max(_17._m1.y, 0.00999999977648258209228515625);
        float _47 = fast::clamp((_17._m1.x - in.m_11.z) / _35, 0.0, 1.0);
        float _56 = (_35 * 0.0500000007450580596923828125) * (_47 * _47);
        float _76 = (_60._m8.z * 1.10000002384185791015625) + dot(_17._m1.zw, float2(0.20999999344348907470703125, 0.17000000178813934326171875));
        float3 _129 = float3(((sin(_76) * 0.699999988079071044921875) + (sin((_76 * 2.7000000476837158203125) + (in.m_11.y * 1.89999997615814208984375)) * 0.25)) * _56, ((cos(_76 * 0.89999997615814208984375) * 0.60000002384185791015625) + (cos((_76 * 3.099999904632568359375) + (in.m_11.x * 1.7000000476837158203125)) * 0.20000000298023223876953125)) * _56, (sin((_76 * 1.60000002384185791015625) + (in.m_11.x * 1.2999999523162841796875)) * 0.119999997317790985107421875) * _56);
        float3 _133 = fast::normalize(in.m_131);
        float _137 = dot(_129, _133);
        _244 = in.m_11 + ((_129 - (_133 * _137)) + ((_133 * fast::max(_137, 0.0)) * 0.3499999940395355224609375));
    }
    else
    {
        _244 = in.m_11;
    }
    float4 _165 = _17._m0 * float4(_244, 1.0);
    out.m_167 = _165.xyz;
    float3x3 _181 = float3x3(_17._m0[0].xyz, _17._m0[1].xyz, _17._m0[2].xyz);
    out.m_182 = _181 * in.m_131;
    out.m_187 = in.m_189;
    out.m_192 = in.m_194;
    float3 _202 = fast::normalize(_181 * in.m_198.xyz);
    float3 _205 = fast::normalize(out.m_182);
    float3 _213 = fast::normalize(_202 - (_205 * dot(_202, _205)));
    out.m_223 = _213;
    out.m_225 = cross(_205, _213) * in.m_198.w;
    out.gl_Position = (_60._m1 * _60._m0) * _165;
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

