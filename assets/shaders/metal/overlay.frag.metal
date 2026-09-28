#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _31
{
    float4x4 _m0;
    float4 _m1;
    float4 _m2;
    float4 _m3;
};

struct overlay_frag_out
{
    float4 m_156 [[color(0)]];
};

struct overlay_frag_in
{
    float2 m_12 [[user(locn0)]];
};

fragment overlay_frag_out overlay_frag(overlay_frag_in in [[stage_in]], constant _31& _33 [[buffer(0)]])
{
    overlay_frag_out out = {};
    float4 _40 = _33._m0 * float4(fma(in.m_12.x, 2.0, -1.0), fma(in.m_12.y, 2.0, -1.0), 0.0, 1.0);
    float _44 = _40.w;
    float4 _58 = _40 / float4((abs(_44) > 9.9999999747524270787835121154785e-07) ? _44 : 9.9999999747524270787835121154785e-07);
    float _95 = fma(sin(fma(_58.x, 1.7000000476837158203125, _33._m3.y * 1.89999997615814208984375)) + sin(fma(_58.y, 2.2999999523162841796875, _33._m3.y * (-1.39999997615814208984375))), _33._m3.z, _33._m2.w) - _58.z;
    float _100 = fast::max(_33._m3.x, 9.9999997473787516355514526367188e-05);
    bool _105 = _33._m3.w < 0.5;
    float _170;
    if (_105)
    {
        _170 = 1.0;
    }
    else
    {
        _170 = smoothstep(-_100, _100, _95);
    }
    float _171;
    if (_105)
    {
        _171 = 0.0;
    }
    else
    {
        _171 = 1.0 - smoothstep(0.0, _100 * 1.7999999523162841796875, abs(_95));
    }
    out.m_156 = float4(_33._m1.xyz * fma(-0.449999988079071044921875, _171, 1.0), fast::clamp(fma(_33._m1.w, _170, _171 * 0.550000011920928955078125), 0.0, 1.0));
    return out;
}

