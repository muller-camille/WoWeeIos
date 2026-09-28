#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _27
{
    float4 _m0;
    float4 _m1;
    float4 _m2;
    float4 _m3;
};

struct fsr_easu_frag_out
{
    float4 m_411 [[color(0)]];
};

struct fsr_easu_frag_in
{
    float2 m_44 [[user(locn0)]];
};

fragment fsr_easu_frag_out fsr_easu_frag(fsr_easu_frag_in in [[stage_in]], constant _27& _29 [[buffer(0)]], texture2d<float> _18 [[texture(0)]], sampler _18Smplr [[sampler(0)]])
{
    fsr_easu_frag_out out = {};
    float2 _61 = fma(in.m_44 * _29._m2.xy, _29._m1.xy, float2(-0.5));
    float2 _64 = floor(_61);
    float2 _68 = _61 - _64;
    float4 _432 = _18.sample(_18Smplr, ((_64 + float2(0.5, -0.5)) * _29._m0.zw), level(0.0));
    float4 _446 = _18.sample(_18Smplr, ((_64 + float2(1.5, -0.5)) * _29._m0.zw), level(0.0));
    float4 _460 = _18.sample(_18Smplr, ((_64 + float2(-0.5, 0.5)) * _29._m0.zw), level(0.0));
    float4 _474 = _18.sample(_18Smplr, ((_64 + float2(0.5)) * _29._m0.zw), level(0.0));
    float4 _488 = _18.sample(_18Smplr, ((_64 + float2(1.5, 0.5)) * _29._m0.zw), level(0.0));
    float4 _516 = _18.sample(_18Smplr, ((_64 + float2(-0.5, 1.5)) * _29._m0.zw), level(0.0));
    float4 _530 = _18.sample(_18Smplr, ((_64 + float2(0.5, 1.5)) * _29._m0.zw), level(0.0));
    float4 _544 = _18.sample(_18Smplr, ((_64 + float2(1.5)) * _29._m0.zw), level(0.0));
    float _159 = _474.y;
    float _162 = _488.y;
    float _171 = _530.y;
    float _174 = _544.y;
    float _219 = ((abs(_460.y - _159) + abs(_159 - _162)) + abs(_516.y - _171)) + abs(_171 - _174);
    float _239 = ((abs(_432.y - _159) + abs(_159 - _171)) + abs(_446.y - _162)) + abs(_162 - _174);
    float _258 = _68.x;
    float _259 = 1.0 - _258;
    float _261 = _68.y;
    float _262 = 1.0 - _261;
    float _263 = _259 * _262;
    float _270 = _258 * _262;
    float _277 = _259 * _261;
    float _283 = _258 * _261;
    float _299 = fast::max(abs(_219 - _239) / ((_219 + _239) + 1.0000000116860974230803549289703e-07), 0.0);
    float _311 = fast::max(fast::max(_263, _270), fast::max(_277, _283));
    float _320 = mix(0.0, _29._m3.x, _299) * 0.25;
    float _321 = mix(_263, float(_263 == _311), _320);
    float _329 = mix(_270, float(_270 == _311), _320);
    float _337 = mix(_277, float(_277 == _311), _320);
    float _345 = mix(_283, float(_283 == _311), _320);
    float _353 = ((_321 + _329) + _337) + _345;
    float3 _415 = fast::clamp(mix((((_474.xyz * (_321 / _353)) + (_488.xyz * (_329 / _353))) + (_530.xyz * (_337 / _353))) + (_544.xyz * (_345 / _353)), (((((((_432.xyz + _446.xyz) + _460.xyz) + _18.sample(_18Smplr, ((_64 + float2(2.5, 0.5)) * _29._m0.zw), level(0.0)).xyz) + _516.xyz) + _18.sample(_18Smplr, ((_64 + float2(2.5, 1.5)) * _29._m0.zw), level(0.0)).xyz) + _18.sample(_18Smplr, ((_64 + float2(0.5, 2.5)) * _29._m0.zw), level(0.0)).xyz) + _18.sample(_18Smplr, ((_64 + float2(1.5, 2.5)) * _29._m0.zw), level(0.0)).xyz) * float3(0.125), float3(_299 * 0.0187500007450580596923828125)), float3(0.0), float3(1.0));
    out.m_411 = float4(_415, 1.0);
    return out;
}

