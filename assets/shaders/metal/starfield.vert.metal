#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _49
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

struct _83
{
    float _m0;
    float _m1;
    float _m2;
};

struct starfield_vert_out
{
    float m_111 [[user(locn0)]];
    float3 m_121 [[user(locn1)]];
    float4 gl_Position [[position]];
    float gl_PointSize [[point_size]];
};

struct starfield_vert_in
{
    float3 m_159 [[attribute(0)]];
    float m_112 [[attribute(1)]];
    float m_92 [[attribute(2)]];
    float m_122 [[attribute(3)]];
};

vertex starfield_vert_out starfield_vert(starfield_vert_in in [[stage_in]], constant _49& _51 [[buffer(0)]], constant _83& _85 [[buffer(1)]])
{
    starfield_vert_out out = {};
    out.m_111 = (in.m_112 * ((0.839999973773956298828125 + (0.100000001490116119384765625 * sin((_85._m0 * 2.2999999523162841796875) + in.m_92))) + (0.0599999986588954925537109375 * sin((_85._m0 * 5.69999980926513671875) + (in.m_92 * 1.7000000476837158203125))))) * _85._m1;
    float3 _199;
    if (in.m_122 < 0.5)
    {
        _199 = mix(float3(1.0, 0.800000011920928955078125, 0.62000000476837158203125), float3(1.0, 0.9700000286102294921875, 0.939999997615814208984375), float3(in.m_122 * 2.0));
    }
    else
    {
        _199 = mix(float3(1.0, 0.9700000286102294921875, 0.939999997615814208984375), float3(0.7799999713897705078125, 0.86000001430511474609375, 1.0), float3((in.m_122 - 0.5) * 2.0));
    }
    out.m_121 = _199;
    out.gl_PointSize = fast::clamp(((0.0019000000320374965667724609375 * _85._m2) * _51._m1[1].y) * mix(0.85000002384185791015625, 1.2999999523162841796875, in.m_112), 1.5, 5.0);
    out.gl_Position = float4(((_51._m1 * float4x4(float4(_51._m0[0].xyz, 0.0), float4(_51._m0[1].xyz, 0.0), float4(_51._m0[2].xyz, 0.0), float4(0.0, 0.0, 0.0, 1.0))) * float4(in.m_159, 1.0)).xyww);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

