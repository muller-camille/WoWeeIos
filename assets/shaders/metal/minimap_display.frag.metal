#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _107
{
    float4 _m0;
    float2 _m1;
    float _m2;
    float _m3;
    float _m4;
    int _m5;
    float _m6;
};

struct minimap_display_frag_out
{
    float4 m_293 [[color(0)]];
};

struct minimap_display_frag_in
{
    float2 m_97 [[user(locn0)]];
};

fragment minimap_display_frag_out minimap_display_frag(minimap_display_frag_in in [[stage_in]], constant _107& _109 [[buffer(0)]], texture2d<float> _183 [[texture(0)]], sampler _183Smplr [[sampler(0)]])
{
    minimap_display_frag_out out = {};
    float2 _101 = in.m_97 - float2(0.5);
    float _104 = length(_101);
    bool _115 = _109._m5 == 0;
    if (_115)
    {
        if (_104 > 0.5)
        {
            discard_fragment();
        }
    }
    float _128 = cos(_109._m2);
    float _132 = sin(_109._m2);
    float _136 = -_101.x;
    float _138 = _101.y;
    float4 _186 = _183.sample(_183Smplr, (_109._m1 + ((float2(fma(_136, _132, _138 * _128), -fma(_136, _128, -(_138 * _132))) * _109._m4) * 2.0)));
    float _193 = cos(_109._m3);
    float _197 = sin(_109._m3);
    float2x2 _219 = float2x2(float2(_193, -_197), float2(_197, _193));
    float2 _222 = _219 * float2(0.0, -0.0900000035762786865234375);
    float2 _225 = _219 * float2(-0.04500000178813934326171875, 0.0199999995529651641845703125);
    float2 _228 = _219 * float2(0.04500000178813934326171875, 0.0199999995529651641845703125);
    float2 _322 = _225 - _222;
    float2 _325 = _101 - _222;
    float _373 = fma(_322.x, _325.y, -(_322.y * _325.x));
    float2 _329 = _228 - _225;
    float2 _332 = _101 - _225;
    float _386 = fma(_329.x, _332.y, -(_329.y * _332.x));
    float2 _336 = _222 - _228;
    float2 _339 = _101 - _228;
    float _399 = fma(_336.x, _339.y, -(_336.y * _339.x));
    float4 _439;
    if (!((((_373 < 0.0) || (_386 < 0.0)) || (_399 < 0.0)) && (((_373 > 0.0) || (_386 > 0.0)) || (_399 > 0.0))))
    {
        float4 _421 = _186;
        _421.x = 1.0;
        _421.y = 0.86000001430511474609375;
        _421.z = 0.0500000007450580596923828125;
        _439 = _421;
    }
    else
    {
        _439 = _186;
    }
    float3 _263 = mix(_439.xyz, float3(1.0), float3(smoothstep(0.01600000075995922088623046875, 0.0, _104) * 0.949999988079071044921875));
    float4 _427 = _439;
    _427.x = _263.x;
    _427.y = _263.y;
    _427.z = _263.z;
    float4 _440;
    if (_115)
    {
        float3 _285 = _427.xyz * fma(-smoothstep(0.4799999892711639404296875, 0.5, _104), 0.699999988079071044921875, 1.0);
        float4 _433 = _427;
        _433.x = _285.x;
        _433.y = _285.y;
        _433.z = _285.z;
        _440 = _433;
    }
    else
    {
        _440 = _427;
    }
    out.m_293 = float4(_440.xyz, _440.w * _109._m6);
    return out;
}

