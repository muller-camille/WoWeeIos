#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _78
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
    float4x4 _m16;
    float4 _m17;
    float4 _m18;
};

struct _726
{
    int _m0;
    int _m1;
    int _m2;
    int _m3;
};

struct terrain_frag_out
{
    float4 m_1109 [[color(0)]];
};

struct terrain_frag_in
{
    float3 m_708 [[user(locn0)]];
    float3 m_798 [[user(locn1)]];
    float2 m_721 [[user(locn2)]];
    float2 m_736 [[user(locn3)]];
};

fragment terrain_frag_out terrain_frag(terrain_frag_in in [[stage_in]], constant _78& _80 [[buffer(0)]], constant _726& _728 [[buffer(1)]], texture2d<float> _143 [[texture(0)]], texture2d<float> _276 [[texture(1)]], texture3d<float> _648 [[texture(2)]], texture2d<float> _718 [[texture(3)]], texture2d<float> _735 [[texture(4)]], texture2d<float> _746 [[texture(5)]], texture2d<float> _759 [[texture(6)]], texture2d<float> _768 [[texture(7)]], texture2d<float> _781 [[texture(8)]], texture2d<float> _790 [[texture(9)]], depth2d<float> _1050 [[texture(10)]], sampler _143Smplr [[sampler(0)]], sampler _276Smplr [[sampler(1)]], sampler _648Smplr [[sampler(2)]], sampler _718Smplr [[sampler(3)]], sampler _735Smplr [[sampler(4)]], sampler _746Smplr [[sampler(5)]], sampler _759Smplr [[sampler(6)]], sampler _768Smplr [[sampler(7)]], sampler _781Smplr [[sampler(8)]], sampler _790Smplr [[sampler(9)]], sampler _1050Smplr [[sampler(10)]])
{
    terrain_frag_out out = {};
    float _711 = length(_80._m6.xyz - in.m_708);
    float _716 = 1.0 - smoothstep(140.0, 260.0, _711);
    float4 _723 = _718.sample(_718Smplr, in.m_721);
    float4 _2026;
    if (_728._m1 != 0)
    {
        float _2022;
        do
        {
            float2 _1147 = fast::min(in.m_736, float2(1.0) - in.m_736);
            float _1158 = (1.0 - smoothstep(0.015625, 0.125, fast::min(_1147.x, _1147.y))) * _716;
            float4 _1161 = _735.sample(_735Smplr, in.m_736);
            float _1162 = _1161.x;
            if (_1158 < 0.001000000047497451305389404296875)
            {
                _2022 = _1162;
                break;
            }
            _2022 = mix(_1162, (((_735.sample(_735Smplr, (in.m_736 + float2(-0.0078125))).x + _735.sample(_735Smplr, (in.m_736 + float2(0.0078125, -0.0078125))).x) + _735.sample(_735Smplr, (in.m_736 + float2(-0.0078125, 0.0078125))).x) + _735.sample(_735Smplr, (in.m_736 + float2(0.0078125))).x) * 0.25, _1158);
            break;
        } while(false);
        float4 _2027;
        if (_2022 > 0.00200000009499490261077880859375)
        {
            _2027 = mix(_723, _746.sample(_746Smplr, in.m_721), float4(_2022));
        }
        else
        {
            _2027 = _723;
        }
        _2026 = _2027;
    }
    else
    {
        _2026 = _723;
    }
    float4 _2030;
    if (_728._m2 != 0)
    {
        float _2024;
        do
        {
            float2 _1238 = fast::min(in.m_736, float2(1.0) - in.m_736);
            float _1249 = (1.0 - smoothstep(0.015625, 0.125, fast::min(_1238.x, _1238.y))) * _716;
            float4 _1252 = _759.sample(_759Smplr, in.m_736);
            float _1253 = _1252.x;
            if (_1249 < 0.001000000047497451305389404296875)
            {
                _2024 = _1253;
                break;
            }
            _2024 = mix(_1253, (((_759.sample(_759Smplr, (in.m_736 + float2(-0.0078125))).x + _759.sample(_759Smplr, (in.m_736 + float2(0.0078125, -0.0078125))).x) + _759.sample(_759Smplr, (in.m_736 + float2(-0.0078125, 0.0078125))).x) + _759.sample(_759Smplr, (in.m_736 + float2(0.0078125))).x) * 0.25, _1249);
            break;
        } while(false);
        float4 _2031;
        if (_2024 > 0.00200000009499490261077880859375)
        {
            _2031 = mix(_2026, _768.sample(_768Smplr, in.m_721), float4(_2024));
        }
        else
        {
            _2031 = _2026;
        }
        _2030 = _2031;
    }
    else
    {
        _2030 = _2026;
    }
    float4 _2032;
    if (_728._m3 != 0)
    {
        float _2028;
        do
        {
            float2 _1329 = fast::min(in.m_736, float2(1.0) - in.m_736);
            float _1340 = (1.0 - smoothstep(0.015625, 0.125, fast::min(_1329.x, _1329.y))) * _716;
            float4 _1343 = _781.sample(_781Smplr, in.m_736);
            float _1344 = _1343.x;
            if (_1340 < 0.001000000047497451305389404296875)
            {
                _2028 = _1344;
                break;
            }
            _2028 = mix(_1344, (((_781.sample(_781Smplr, (in.m_736 + float2(-0.0078125))).x + _781.sample(_781Smplr, (in.m_736 + float2(0.0078125, -0.0078125))).x) + _781.sample(_781Smplr, (in.m_736 + float2(-0.0078125, 0.0078125))).x) + _781.sample(_781Smplr, (in.m_736 + float2(0.0078125))).x) * 0.25, _1340);
            break;
        } while(false);
        float4 _2033;
        if (_2028 > 0.00200000009499490261077880859375)
        {
            _2033 = mix(_2030, _790.sample(_790Smplr, in.m_721), float4(_2028));
        }
        else
        {
            _2033 = _2030;
        }
        _2032 = _2033;
    }
    else
    {
        _2032 = _2030;
    }
    float3 _800 = fast::normalize(in.m_798);
    float _826 = (1.0 - smoothstep(50.0, 125.0, _711)) * smoothstep(0.0, 0.0599999986588954925537109375, fast::min(fast::min(in.m_736.x, 1.0 - in.m_736.x), fast::min(in.m_736.y, 1.0 - in.m_736.y)));
    float3 _2041;
    if (_826 > 0.001000000047497451305389404296875)
    {
        float _838 = dot(_2032.xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
        float _841 = dfdx(_838);
        float _844 = dfdy(_838);
        float3 _847 = dfdx(in.m_708);
        float3 _850 = dfdy(in.m_708);
        float3 _854 = cross(_800, _850);
        float3 _858 = cross(_847, _800);
        float _861 = length(_854);
        float _864 = length(_858);
        float3 _2042;
        if ((_861 > 9.9999999747524270787835121154785e-07) && (_864 > 9.9999999747524270787835121154785e-07))
        {
            float3 _889 = (((_854 / float3(_861)) * _841) + ((_858 / float3(_864)) * _844)) * (9.0 * _826);
            float _892 = length(_889);
            float3 _2035;
            if (_892 > 0.699999988079071044921875)
            {
                _2035 = _889 * (0.699999988079071044921875 / _892);
            }
            else
            {
                _2035 = _889;
            }
            float3 _905 = _800 - _2035;
            float _909 = dot(_905, _905);
            float3 _2036;
            if (_909 > 9.9999999392252902907785028219223e-09)
            {
                _2036 = _905 * rsqrt(_909);
            }
            else
            {
                _2036 = _800;
            }
            _2042 = _2036;
        }
        else
        {
            _2042 = _800;
        }
        _2041 = _2042;
    }
    else
    {
        _2041 = _800;
    }
    float _940 = dot(_2041, fast::normalize(-_80._m3.xyz));
    float _2084;
    if (_80._m9.x > 0.5)
    {
        float _2191 = (_80._m9.z > 0.0) ? _80._m9.z : 0.000244140625;
        float _972 = 1.0 - abs(_940);
        float4 _988 = _80._m2 * float4(in.m_708 + (_2041 * ((_2191 * 2.0) * _972)), 1.0);
        float3 _995 = _988.xyz / float3(_988.w);
        float2 _1000 = (_995.xy * 0.5) + float2(0.5);
        float _1002 = _1000.x;
        float _1004 = _1000.y;
        bool _1007 = _1002 >= 0.0;
        bool _1013;
        if (_1007)
        {
            _1013 = _1002 <= 1.0;
        }
        else
        {
            _1013 = _1007;
        }
        bool _1019;
        if (_1013)
        {
            _1019 = _1004 >= 0.0;
        }
        else
        {
            _1019 = _1013;
        }
        bool _1025;
        if (_1019)
        {
            _1025 = _1004 <= 1.0;
        }
        else
        {
            _1025 = _1019;
        }
        bool _1031;
        if (_1025)
        {
            _1031 = _995.z >= 0.0;
        }
        else
        {
            _1031 = _1025;
        }
        bool _1037;
        if (_1031)
        {
            _1037 = _995.z <= 1.0;
        }
        else
        {
            _1037 = _1031;
        }
        float _2085;
        if (_1037)
        {
            float _1056 = _995.z - fast::max(0.0005000000237487256526947021484375 * _972, 4.9999998736893758177757263183594e-05);
            float3 _1059 = float3(_1002, _1004, _1056);
            int _2050;
            float _2051;
            _2051 = 0.0;
            _2050 = -1;
            float _2143;
            for (; _2050 <= 1; _2051 = _2143, _2050++)
            {
                _2143 = _2051;
                for (int _2139 = -1; _2139 <= 1; )
                {
                    _2143 += _1050.sample_compare(_1050Smplr, float3(_1059.xy + (float2(float(_2050), float(_2139)) * _2191), _1056).xy, _1056);
                    _2139++;
                    continue;
                }
            }
            _2085 = mix(1.0, _2051 * 0.111111111938953399658203125, _80._m9.y);
        }
        else
        {
            _2085 = 1.0;
        }
        _2084 = _2085;
    }
    else
    {
        _2084 = 1.0;
    }
    float _1502;
    float3 _2079;
    float _2080;
    float _2081;
    float _2082;
    do
    {
        _1502 = _80._m18.x;
        if (_1502 < 0.5)
        {
            _2082 = 0.0;
            _2081 = 1.0;
            _2080 = 1.0;
            _2079 = float3(0.0);
            break;
        }
        float4 _1514 = _80._m16 * float4(in.m_708, 1.0);
        float _1516 = _1514.w;
        if (_1516 <= 0.0)
        {
            _2082 = 0.0;
            _2081 = 1.0;
            _2080 = 1.0;
            _2079 = float3(0.0);
            break;
        }
        float2 _1529 = ((_1514.xy / float2(_1516)) * 0.5) + float2(0.5);
        bool _1532 = any(_1529 < float2(0.0));
        bool _1539;
        if (!_1532)
        {
            _1539 = any(_1529 > float2(1.0));
        }
        else
        {
            _1539 = _1532;
        }
        if (_1539)
        {
            _2082 = 0.0;
            _2081 = 1.0;
            _2080 = 1.0;
            _2079 = float3(0.0);
            break;
        }
        int2 _1545 = int2(_143.get_width(), _143.get_height());
        float2 _1549 = _1529 * float2(_1545);
        float2 _1551 = _1549 - float2(0.5);
        int2 _1556 = int2(floor(_1549));
        float _1562 = length(in.m_708 - _80._m17.xyz);
        float _1565 = (0.02999999932944774627685546875 * _1562) + 0.0500000007450580596923828125;
        bool _1568 = _1502 > 2.5;
        float _2053;
        float _2054;
        float2 _2055;
        float3 _2056;
        _2056 = float3(0.0);
        _2055 = float2(0.0);
        _2054 = 0.0;
        _2053 = 0.0;
        float _2069;
        float _2071;
        float2 _2073;
        float3 _2075;
        for (int _2052 = -1; _2052 <= 1; _2056 = _2075, _2055 = _2073, _2054 = _2069, _2053 = _2071, _2052++)
        {
            _2075 = _2056;
            _2073 = _2055;
            _2071 = _2053;
            _2069 = _2054;
            float _2164;
            float _2166;
            float2 _2168;
            float3 _2170;
            for (int _2065 = -1; _2065 <= 1; _2075 = _2170, _2073 = _2168, _2071 = _2166, _2069 = _2164, _2065++)
            {
                int2 _1587 = clamp(_1556 + int2(_2065, _2052), int2(0), _1545 - int2(1));
                float4 _1591 = _143.read(uint2(_1587), 0);
                float _1593 = _1591.w;
                float _1597 = (_1593 - _1562) / _1565;
                float _2067;
                if (_1593 > 0.0)
                {
                    _2067 = exp((-_1597) * _1597);
                }
                else
                {
                    _2067 = 0.0;
                }
                float2 _1613 = float2(_1587) - _1551;
                float _1620 = _2067 * exp(-dot(_1613, _1613));
                if (_1620 <= 9.9999997473787516355514526367188e-05)
                {
                    _2170 = _2075;
                    _2168 = _2073;
                    _2166 = _2071;
                    _2164 = _2069;
                    continue;
                }
                float3 _2171;
                if (_1568)
                {
                    _2171 = _2075 + (_276.read(uint2(_1587), 0).xyz * _1620);
                }
                else
                {
                    _2171 = _2075;
                }
                _2170 = _2171;
                _2168 = _2073 + (_1591.xy * _1620);
                _2166 = _2071 + _1620;
                _2164 = fast::max(_2069, _2067);
            }
        }
        if (_2053 <= 9.9999997473787516355514526367188e-05)
        {
            _2082 = 0.0;
            _2081 = 1.0;
            _2080 = 1.0;
            _2079 = float3(0.0);
            break;
        }
        _2082 = smoothstep(0.100000001490116119384765625, 0.60000002384185791015625, _2054);
        _2081 = _2055.x / _2053;
        _2080 = _2055.y / _2053;
        _2079 = _2056 / float3(_2053);
        break;
    } while(false);
    float3 _2099;
    do
    {
        if (_1502 < 1.5)
        {
            _2099 = _80._m5.xyz;
            break;
        }
        float3 _2098;
        if (_1502 > 2.5)
        {
            _2098 = _2079;
        }
        else
        {
            _2098 = _80._m5.xyz * _2080;
        }
        _2099 = mix(_80._m5.xyz, _2098, float3(_2082));
        break;
    } while(false);
    float3 _2133;
    _2133 = float3(0.0);
    float3 _2176;
    for (int _2132 = 0; _2132 < min(_80._m14.x, 64); _2133 = _2176, _2132++)
    {
        float3 _1749 = _80._m12[_2132].xyz - in.m_708;
        float _1755 = dot(_1749, _1749);
        bool _1757 = _80._m12[_2132].w <= 0.0;
        bool _1766;
        if (!_1757)
        {
            _1766 = _1755 >= (_80._m12[_2132].w * _80._m12[_2132].w);
        }
        else
        {
            _1766 = _1757;
        }
        if (_1766)
        {
            _2176 = _2133;
            continue;
        }
        float _1770 = sqrt(_1755);
        float _1774 = 1.0 - (_1770 / _80._m12[_2132].w);
        _2176 = _2133 + ((_2032.xyz * _80._m13[_2132].xyz) * ((_80._m13[_2132].w * (_1774 * _1774)) * (0.2199999988079071044921875 + (0.7799999713897705078125 * fast::max(dot(_2041, _1749 / float3(fast::max(_1770, 0.001000000047497451305389404296875))), 0.0)))));
    }
    float3 _1831 = mix(_80._m7.xyz, ((_2099 * _2032.xyz) + (((_80._m4.xyz * fast::max(_940, 0.0)) * _2032.xyz) * fast::min(_2084, mix(1.0, _2081, _2082)))) + _2133, float3(fast::clamp((_80._m8.y - _711) / (_80._m8.y - _80._m8.x), 0.0, 1.0)));
    float3 _2135;
    if (_80._m15.x > 0.5)
    {
        float4 _1863 = (_80._m1 * _80._m0) * float4(in.m_708, 1.0);
        float _1866 = fast::max(_1863.w, 9.9999997473787516355514526367188e-05);
        float4 _1896 = _648.sample(_648Smplr, float3(((_1863.xy / float2(_1866)) * 0.5) + float2(0.5), (log(fast::max(_1866, _80._m15.y) / _80._m15.y) * _80._m15.z) - (0.5 / _80._m15.w)), level(0.0));
        _2135 = (_1831 * _1896.w) + _1896.xyz;
    }
    else
    {
        _2135 = _1831;
    }
    out.m_1109 = float4(_2135, 1.0);
    return out;
}

