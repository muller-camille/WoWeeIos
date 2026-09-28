#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _22
{
    float4x4 _m0;
    float4 _m1;
    int4 _m2;
    float4 _m3;
};

struct shadow_vert_out
{
    float2 m_178 [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct shadow_vert_in
{
    float3 m_12 [[attribute(0)]];
    float2 m_180 [[attribute(1)]];
};

vertex shadow_vert_out shadow_vert(shadow_vert_in in [[stage_in]], constant _22& _24 [[buffer(0)]])
{
    shadow_vert_out out = {};
    float4 _18 = float4(in.m_12, 1.0);
    float4 _205;
    if (_24._m2.z != 0)
    {
        float _55 = fast::clamp(in.m_12.z / fast::max(_24._m1.z, 0.00999999977648258209228515625), 0.0, 1.0);
        float _64 = _24._m1.w * (_55 * _55);
        float _77 = (_24._m3.x * 0.800000011920928955078125) + dot(_24._m1.xy, float2(0.100000001490116119384765625, 0.12999999523162841796875));
        float _104 = (_24._m3.x * 1.7000000476837158203125) + dot(_24._m1.xy, float2(0.37000000476837158203125, 0.709999978542327880859375));
        float _142 = (_24._m3.x * 4.5) + dot(in.m_12, float3(1.7000000476837158203125, 2.2999999523162841796875, 0.89999997615814208984375));
        float4 _201 = _18;
        _201.x = in.m_12.x + (_64 * (((sin(_77) * 0.3499999940395355224609375) + (sin(_104 + (in.m_12.y * 0.4000000059604644775390625)) * 0.1500000059604644775390625)) + (sin(_142) * 0.0599999986588954925537109375)));
        _201.y = in.m_12.y + (_64 * (((cos(_77 * 0.699999988079071044921875) * 0.25) + (cos((_104 * 1.10000002384185791015625) + (in.m_12.x * 0.300000011920928955078125)) * 0.119999997317790985107421875)) + (cos(_142 * 1.2999999523162841796875) * 0.0500000007450580596923828125)));
        _205 = _201;
    }
    else
    {
        _205 = _18;
    }
    out.m_178 = in.m_180;
    out.gl_Position = _24._m0 * _205;
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

