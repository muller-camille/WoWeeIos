#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wmissing-braces"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

template<typename T, size_t Num>
struct spvUnsafeArray
{
    T elements[Num ? Num : 1];
    
    thread T& operator [] (size_t pos) thread
    {
        return elements[pos];
    }
    constexpr const thread T& operator [] (size_t pos) const thread
    {
        return elements[pos];
    }
    
    device T& operator [] (size_t pos) device
    {
        return elements[pos];
    }
    constexpr const device T& operator [] (size_t pos) const device
    {
        return elements[pos];
    }
    
    constexpr const constant T& operator [] (size_t pos) const constant
    {
        return elements[pos];
    }
    
    threadgroup T& operator [] (size_t pos) threadgroup
    {
        return elements[pos];
    }
    constexpr const threadgroup T& operator [] (size_t pos) const threadgroup
    {
        return elements[pos];
    }
};

struct _24
{
    float2 _m0;
    float _m1;
    float _m2;
};

constant spvUnsafeArray<float, 12> _644 = spvUnsafeArray<float, 12>({ 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 });

struct fxaa_frag_out
{
    float4 m_271 [[color(0)]];
};

struct fxaa_frag_in
{
    float2 m_38 [[user(locn0)]];
};

fragment fxaa_frag_out fxaa_frag(fxaa_frag_in in [[stage_in]], constant _24& _26 [[buffer(0)]], texture2d<float> _75 [[texture(0)]], sampler _75Smplr [[sampler(0)]])
{
    fxaa_frag_out out = {};
    do
    {
        float _34 = fast::clamp(_26._m2, 0.0, 1.0);
        float2 _66 = in.m_38 + (float2(sin(in.m_38.y * 34.0) * _26._m0.x, cos(in.m_38.x * 29.0) * _26._m0.y) * (3.0 * _34));
        float4 _79 = _75.sample(_75Smplr, _66);
        float3 _80 = _79.xyz;
        float3 _936;
        if (_34 > 0.0)
        {
            float2 _91 = _26._m0 * mix(1.0, 5.0, _34);
            float _97 = _91.x;
            float _108 = -_97;
            float _118 = _91.y;
            float _129 = -_118;
            _936 = mix(_80, ((((((((_80 + _75.sample(_75Smplr, (_66 + float2(_97, 0.0))).xyz) + _75.sample(_75Smplr, (_66 + float2(_108, 0.0))).xyz) + _75.sample(_75Smplr, (_66 + float2(0.0, _118))).xyz) + _75.sample(_75Smplr, (_66 + float2(0.0, _129))).xyz) + _75.sample(_75Smplr, (_66 + _91)).xyz) + _75.sample(_75Smplr, (_66 - _91)).xyz) + _75.sample(_75Smplr, (_66 + float2(_97, _129))).xyz) + _75.sample(_75Smplr, (_66 + float2(_108, _118))).xyz) * float3(0.111111111938953399658203125), float3(0.75 * _34));
        }
        else
        {
            _936 = _80;
        }
        float _851 = dot(_936, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
        float4 _200 = _75.sample(_75Smplr, (_66 + (float2(0.0, -1.0) * _26._m0)));
        float _855 = dot(_200.xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
        float4 _211 = _75.sample(_75Smplr, (_66 + (float2(0.0, 1.0) * _26._m0)));
        float _859 = dot(_211.xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
        float4 _222 = _75.sample(_75Smplr, (_66 + (float2(1.0, 0.0) * _26._m0)));
        float _863 = dot(_222.xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
        float4 _233 = _75.sample(_75Smplr, (_66 + (float2(-1.0, 0.0) * _26._m0)));
        float _867 = dot(_233.xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
        float _246 = fast::max(_851, fast::max(fast::max(_855, _859), fast::max(_863, _867)));
        float _260 = _246 - fast::min(_851, fast::min(fast::min(_855, _859), fast::min(_863, _867)));
        if (_260 < fast::max(0.083300001919269561767578125, _246 * 0.16599999368190765380859375))
        {
            out.m_271 = float4(_936, 1.0);
            break;
        }
        float _871 = dot(_75.sample(_75Smplr, (_66 + (float2(-1.0) * _26._m0))).xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
        float _875 = dot(_75.sample(_75Smplr, (_66 + (float2(1.0, -1.0) * _26._m0))).xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
        float _879 = dot(_75.sample(_75Smplr, (_66 + (float2(-1.0, 1.0) * _26._m0))).xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
        float _883 = dot(_75.sample(_75Smplr, (_66 + _26._m0)).xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
        float _325 = _855 + _859;
        float _329 = _867 + _863;
        float _333 = _871 + _879;
        float _337 = _875 + _883;
        float _367 = fast::clamp(abs(((((_325 + _329) * 2.0) + (_333 + _337)) * 0.083333335816860198974609375) - _851) / _260, 0.0, 1.0);
        float _376 = ((((-2.0) * _367) + 3.0) * _367) * _367;
        float _389 = (-2.0) * _851;
        bool _424 = ((abs(((-2.0) * _867) + _333) + (abs(_389 + _325) * 2.0)) + abs(((-2.0) * _863) + _337)) >= ((abs(((-2.0) * _859) + (_879 + _883)) + (abs(_389 + _329) * 2.0)) + abs(((-2.0) * _855) + (_871 + _875)));
        float _429 = _424 ? _855 : _867;
        float _434 = _424 ? _859 : _863;
        float _445 = abs(_429 - _851);
        float _447 = abs(_434 - _851);
        bool _448 = _445 >= _447;
        float _456 = fast::max(_445, _447) * 0.25;
        float _937;
        if (_424)
        {
            _937 = _26._m0.y;
        }
        else
        {
            _937 = _26._m0.x;
        }
        float _940;
        if (_448)
        {
            _940 = -_937;
        }
        else
        {
            _940 = _937;
        }
        float _481 = ((_448 ? _429 : _434) + _851) * 0.5;
        bool _486 = (_851 - _481) < 0.0;
        float2 _938;
        if (_424)
        {
            _938 = float2(_26._m0.x, 0.0);
        }
        else
        {
            _938 = float2(0.0, _26._m0.y);
        }
        float2 _943;
        if (_424)
        {
            float2 _914 = _66;
            _914.y = _66.y + (_940 * 0.5);
            _943 = _914;
        }
        else
        {
            _943 = _66;
        }
        bool _512 = !_424;
        float2 _944;
        if (_512)
        {
            float2 _917 = _943;
            _917.x = _943.x + (_940 * 0.5);
            _944 = _917;
        }
        else
        {
            _944 = _943;
        }
        float2 _524 = _938 * 1.0;
        float2 _525 = _944 - _524;
        float2 _530 = _944 + _524;
        float4 _534 = _75.sample(_75Smplr, _525);
        float _539 = dot(_534.xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625)) - _481;
        float4 _543 = _75.sample(_75Smplr, _530);
        float _548 = dot(_543.xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625)) - _481;
        bool _553 = abs(_539) >= _456;
        bool _558 = abs(_548) >= _456;
        float2 _956;
        if (!_553)
        {
            _956 = _525 - (_938 * 1.5);
        }
        else
        {
            _956 = _525;
        }
        float2 _961;
        if (!_558)
        {
            _961 = _530 + (_938 * 1.5);
        }
        else
        {
            _961 = _530;
        }
        float2 _954;
        float2 _960;
        float _966;
        float _972;
        _972 = _548;
        _966 = _539;
        _960 = _961;
        _954 = _956;
        bool _625;
        bool _634;
        float _1006;
        float _1010;
        float2 _1033;
        float2 _1034;
        int _947 = 2;
        bool _948 = _553;
        bool _951 = _558;
        for (;;)
        {
            bool _585 = _947 < 12;
            bool _592;
            if (_585)
            {
                _592 = !(_948 && _951);
            }
            else
            {
                _592 = _585;
            }
            if (_592)
            {
                bool _594 = !_948;
                if (_594)
                {
                    _1006 = dot(_75.sample(_75Smplr, _954).xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625)) - _481;
                }
                else
                {
                    _1006 = _966;
                }
                bool _606 = !_951;
                if (_606)
                {
                    _1010 = dot(_75.sample(_75Smplr, _960).xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625)) - _481;
                }
                else
                {
                    _1010 = _972;
                }
                if (_594)
                {
                    _625 = abs(_1006) >= _456;
                }
                else
                {
                    _625 = _948;
                }
                if (_606)
                {
                    _634 = abs(_1010) >= _456;
                }
                else
                {
                    _634 = _951;
                }
                if (!_625)
                {
                    _1033 = _954 - (_938 * _644[_947]);
                }
                else
                {
                    _1033 = _954;
                }
                if (!_634)
                {
                    _1034 = _960 + (_938 * _644[_947]);
                }
                else
                {
                    _1034 = _960;
                }
                _972 = _1010;
                _966 = _1006;
                _960 = _1034;
                _954 = _1033;
                _951 = _634;
                _948 = _625;
                _947++;
                continue;
            }
            else
            {
                break;
            }
        }
        float _957;
        if (_424)
        {
            _957 = _66.x - _954.x;
        }
        else
        {
            _957 = _66.y - _954.y;
        }
        float _962;
        if (_424)
        {
            _962 = _960.x - _66.x;
        }
        else
        {
            _962 = _960.y - _66.y;
        }
        float _741 = fast::max(((_957 < _962) ? ((_966 < 0.0) != _486) : ((_972 < 0.0) != _486)) ? (0.5 - (fast::min(_957, _962) / (_957 + _962))) : 0.0, (_376 * _376) * 0.75);
        float2 _983;
        if (_424)
        {
            float2 _928 = _66;
            _928.y = _66.y + (_741 * _940);
            _983 = _928;
        }
        else
        {
            _983 = _66;
        }
        float2 _984;
        if (_512)
        {
            float2 _931 = _983;
            _931.x = _983.x + (_741 * _940);
            _984 = _931;
        }
        else
        {
            _984 = _983;
        }
        float4 _768 = _75.sample(_75Smplr, _984);
        float3 _775 = mix(_768.xyz, _936, float3(0.75 * _34));
        float3 _998;
        if (_26._m1 > 0.0)
        {
            _998 = fast::clamp(_775 + ((_775 - ((((_75.sample(_75Smplr, (_66 + float2(-_26._m0.x, 0.0))).xyz + _75.sample(_75Smplr, (_66 + float2(_26._m0.x, 0.0))).xyz) + _75.sample(_75Smplr, (_66 + float2(0.0, -_26._m0.y))).xyz) + _75.sample(_75Smplr, (_66 + float2(0.0, _26._m0.y))).xyz) * 0.25)) * (_26._m1 * 0.1500000059604644775390625)), float3(0.0), float3(1.0));
        }
        else
        {
            _998 = _775;
        }
        out.m_271 = float4(_998, 1.0);
        break;
    } while(false);
    return out;
}

