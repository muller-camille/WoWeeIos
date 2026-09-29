#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _76
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

struct _633
{
    int _m0;
    int _m1;
    int _m2;
    int _m3;
};

struct terrain_frag_out
{
    float4 m_1016 [[color(0)]];
};

struct terrain_frag_in
{
    float3 m_620 [[user(locn0)]];
    float3 m_705 [[user(locn1)]];
    float2 m_628 [[user(locn2)]];
    float2 m_643 [[user(locn3)]];
};

fragment terrain_frag_out terrain_frag(terrain_frag_in in [[stage_in]], constant _76& _78 [[buffer(0)]], constant _633& _635 [[buffer(1)]], texture2d<float> _141 [[texture(0)]], texture2d<float> _274 [[texture(1)]], texture3d<float> _560 [[texture(2)]], texture2d<float> _625 [[texture(3)]], texture2d<float> _642 [[texture(4)]], texture2d<float> _653 [[texture(5)]], texture2d<float> _666 [[texture(6)]], texture2d<float> _675 [[texture(7)]], texture2d<float> _688 [[texture(8)]], texture2d<float> _697 [[texture(9)]], depth2d<float> _957 [[texture(10)]], sampler _141Smplr [[sampler(0)]], sampler _274Smplr [[sampler(1)]], sampler _560Smplr [[sampler(2)]], sampler _625Smplr [[sampler(3)]], sampler _642Smplr [[sampler(4)]], sampler _653Smplr [[sampler(5)]], sampler _666Smplr [[sampler(6)]], sampler _675Smplr [[sampler(7)]], sampler _688Smplr [[sampler(8)]], sampler _697Smplr [[sampler(9)]], sampler _957Smplr [[sampler(10)]])
{
    terrain_frag_out out = {};
    float _623 = length(_78._m6.xyz - in.m_620);
    float4 _630 = _625.sample(_625Smplr, in.m_628);
    float4 _1639;
    if (_635._m1 != 0)
    {
        float4 _1038 = _642.sample(_642Smplr, in.m_643);
        float _1039 = _1038.x;
        float4 _1640;
        if (_1039 > 0.00200000009499490261077880859375)
        {
            _1640 = mix(_630, _653.sample(_653Smplr, in.m_628), float4(_1039));
        }
        else
        {
            _1640 = _630;
        }
        _1639 = _1640;
    }
    else
    {
        _1639 = _630;
    }
    float4 _1641;
    if (_635._m2 != 0)
    {
        float4 _1044 = _666.sample(_666Smplr, in.m_643);
        float _1045 = _1044.x;
        float4 _1642;
        if (_1045 > 0.00200000009499490261077880859375)
        {
            _1642 = mix(_1639, _675.sample(_675Smplr, in.m_628), float4(_1045));
        }
        else
        {
            _1642 = _1639;
        }
        _1641 = _1642;
    }
    else
    {
        _1641 = _1639;
    }
    float4 _1643;
    if (_635._m3 != 0)
    {
        float4 _1050 = _688.sample(_688Smplr, in.m_643);
        float _1051 = _1050.x;
        float4 _1644;
        if (_1051 > 0.00200000009499490261077880859375)
        {
            _1644 = mix(_1641, _697.sample(_697Smplr, in.m_628), float4(_1051));
        }
        else
        {
            _1644 = _1641;
        }
        _1643 = _1644;
    }
    else
    {
        _1643 = _1641;
    }
    float3 _707 = fast::normalize(in.m_705);
    float _733 = (1.0 - smoothstep(50.0, 125.0, _623)) * smoothstep(0.0, 0.0599999986588954925537109375, fast::min(fast::min(in.m_643.x, 1.0 - in.m_643.x), fast::min(in.m_643.y, 1.0 - in.m_643.y)));
    float3 _1652;
    if (_733 > 0.001000000047497451305389404296875)
    {
        float _745 = dot(_1643.xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
        float _748 = dfdx(_745);
        float _751 = dfdy(_745);
        float3 _754 = dfdx(in.m_620);
        float3 _757 = dfdy(in.m_620);
        float3 _761 = cross(_707, _757);
        float3 _765 = cross(_754, _707);
        float _768 = length(_761);
        float _771 = length(_765);
        float3 _1653;
        if ((_768 > 9.9999999747524270787835121154785e-07) && (_771 > 9.9999999747524270787835121154785e-07))
        {
            float3 _796 = (((_761 / float3(_768)) * _748) + ((_765 / float3(_771)) * _751)) * (9.0 * _733);
            float _799 = length(_796);
            float3 _1646;
            if (_799 > 0.699999988079071044921875)
            {
                _1646 = _796 * (0.699999988079071044921875 / _799);
            }
            else
            {
                _1646 = _796;
            }
            float3 _812 = _707 - _1646;
            float _816 = dot(_812, _812);
            float3 _1647;
            if (_816 > 9.9999999392252902907785028219223e-09)
            {
                _1647 = _812 * rsqrt(_816);
            }
            else
            {
                _1647 = _707;
            }
            _1653 = _1647;
        }
        else
        {
            _1653 = _707;
        }
        _1652 = _1653;
    }
    else
    {
        _1652 = _707;
    }
    float _847 = dot(_1652, fast::normalize(-_78._m3.xyz));
    float _1695;
    if (_78._m9.x > 0.5)
    {
        float _1802 = (_78._m9.z > 0.0) ? _78._m9.z : 0.000244140625;
        float _879 = 1.0 - abs(_847);
        float4 _895 = _78._m2 * float4(in.m_620 + (_1652 * ((_1802 * 2.0) * _879)), 1.0);
        float3 _902 = _895.xyz / float3(_895.w);
        float2 _907 = (_902.xy * 0.5) + float2(0.5);
        float _909 = _907.x;
        float _911 = _907.y;
        bool _914 = _909 >= 0.0;
        bool _920;
        if (_914)
        {
            _920 = _909 <= 1.0;
        }
        else
        {
            _920 = _914;
        }
        bool _926;
        if (_920)
        {
            _926 = _911 >= 0.0;
        }
        else
        {
            _926 = _920;
        }
        bool _932;
        if (_926)
        {
            _932 = _911 <= 1.0;
        }
        else
        {
            _932 = _926;
        }
        bool _938;
        if (_932)
        {
            _938 = _902.z >= 0.0;
        }
        else
        {
            _938 = _932;
        }
        bool _944;
        if (_938)
        {
            _944 = _902.z <= 1.0;
        }
        else
        {
            _944 = _938;
        }
        float _1696;
        if (_944)
        {
            float _963 = _902.z - fast::max(0.0005000000237487256526947021484375 * _879, 4.9999998736893758177757263183594e-05);
            float3 _966 = float3(_909, _911, _963);
            int _1661;
            float _1662;
            _1662 = 0.0;
            _1661 = -1;
            float _1754;
            for (; _1661 <= 1; _1662 = _1754, _1661++)
            {
                _1754 = _1662;
                for (int _1750 = -1; _1750 <= 1; )
                {
                    _1754 += _957.sample_compare(_957Smplr, float3(_966.xy + (float2(float(_1661), float(_1750)) * _1802), _963).xy, _963);
                    _1750++;
                    continue;
                }
            }
            _1696 = mix(1.0, _1662 * 0.111111111938953399658203125, _78._m9.y);
        }
        else
        {
            _1696 = 1.0;
        }
        _1695 = _1696;
    }
    else
    {
        _1695 = 1.0;
    }
    float _1149;
    float3 _1690;
    float _1691;
    float _1692;
    float _1693;
    do
    {
        _1149 = _78._m18.x;
        if (_1149 < 0.5)
        {
            _1693 = 0.0;
            _1692 = 1.0;
            _1691 = 1.0;
            _1690 = float3(0.0);
            break;
        }
        float4 _1161 = _78._m16 * float4(in.m_620, 1.0);
        float _1163 = _1161.w;
        if (_1163 <= 0.0)
        {
            _1693 = 0.0;
            _1692 = 1.0;
            _1691 = 1.0;
            _1690 = float3(0.0);
            break;
        }
        float2 _1176 = ((_1161.xy / float2(_1163)) * 0.5) + float2(0.5);
        bool _1179 = any(_1176 < float2(0.0));
        bool _1186;
        if (!_1179)
        {
            _1186 = any(_1176 > float2(1.0));
        }
        else
        {
            _1186 = _1179;
        }
        if (_1186)
        {
            _1693 = 0.0;
            _1692 = 1.0;
            _1691 = 1.0;
            _1690 = float3(0.0);
            break;
        }
        int2 _1192 = int2(_141.get_width(), _141.get_height());
        float2 _1198 = fma(_1176, float2(_1192), float2(-0.5));
        int2 _1203 = int2(floor(_1198 + float2(0.5)));
        float _1209 = length(in.m_620 - _78._m17.xyz);
        float _1212 = fma(0.02999999932944774627685546875, _1209, 0.0500000007450580596923828125);
        bool _1215 = _1149 > 2.5;
        float _1664;
        float _1665;
        float2 _1666;
        float3 _1667;
        _1667 = float3(0.0);
        _1666 = float2(0.0);
        _1665 = 0.0;
        _1664 = 0.0;
        float _1680;
        float _1682;
        float2 _1684;
        float3 _1686;
        for (int _1663 = -1; _1663 <= 1; _1667 = _1686, _1666 = _1684, _1665 = _1680, _1664 = _1682, _1663++)
        {
            _1686 = _1667;
            _1684 = _1666;
            _1682 = _1664;
            _1680 = _1665;
            float _1775;
            float _1777;
            float2 _1779;
            float3 _1781;
            for (int _1676 = -1; _1676 <= 1; _1686 = _1781, _1684 = _1779, _1682 = _1777, _1680 = _1775, _1676++)
            {
                int2 _1234 = clamp(_1203 + int2(_1676, _1663), int2(0), _1192 - int2(1));
                float4 _1238 = _141.read(uint2(_1234), 0);
                float _1240 = _1238.w;
                float _1244 = (_1240 - _1209) / _1212;
                float _1678;
                if (_1240 > 0.0)
                {
                    _1678 = exp((-_1244) * _1244);
                }
                else
                {
                    _1678 = 0.0;
                }
                float2 _1260 = float2(_1234) - _1198;
                float _1266 = exp(-dot(_1260, _1260));
                float _1267 = _1678 * _1266;
                if (_1267 <= 9.9999997473787516355514526367188e-05)
                {
                    _1781 = _1686;
                    _1779 = _1684;
                    _1777 = _1682;
                    _1775 = _1680;
                    continue;
                }
                float3 _1782;
                if (_1215)
                {
                    _1782 = _1686 + (_274.read(uint2(_1234), 0).xyz * _1267);
                }
                else
                {
                    _1782 = _1686;
                }
                _1781 = _1782;
                _1779 = _1684 + (_1238.xy * _1267);
                _1777 = fma(_1678, _1266, _1682);
                _1775 = fast::max(_1680, _1678);
            }
        }
        if (_1664 <= 9.9999997473787516355514526367188e-05)
        {
            _1693 = 0.0;
            _1692 = 1.0;
            _1691 = 1.0;
            _1690 = float3(0.0);
            break;
        }
        _1693 = smoothstep(0.100000001490116119384765625, 0.60000002384185791015625, _1665);
        _1692 = _1666.x / _1664;
        _1691 = _1666.y / _1664;
        _1690 = _1667 / float3(_1664);
        break;
    } while(false);
    float3 _1710;
    do
    {
        if (_1149 < 1.5)
        {
            _1710 = _78._m5.xyz;
            break;
        }
        float3 _1709;
        if (_1149 > 2.5)
        {
            _1709 = _1690;
        }
        else
        {
            _1709 = _78._m5.xyz * _1691;
        }
        _1710 = mix(_78._m5.xyz, _1709, float3(_1693));
        break;
    } while(false);
    float3 _1744;
    _1744 = float3(0.0);
    float3 _1787;
    for (int _1743 = 0; _1743 < min(_78._m14.x, 64); _1744 = _1787, _1743++)
    {
        float3 _1396 = _78._m12[_1743].xyz - in.m_620;
        float _1402 = dot(_1396, _1396);
        bool _1404 = _78._m12[_1743].w <= 0.0;
        bool _1413;
        if (!_1404)
        {
            _1413 = _1402 >= (_78._m12[_1743].w * _78._m12[_1743].w);
        }
        else
        {
            _1413 = _1404;
        }
        if (_1413)
        {
            _1787 = _1744;
            continue;
        }
        float _1417 = sqrt(_1402);
        float _1421 = 1.0 - (_1417 / _78._m12[_1743].w);
        _1787 = _1744 + ((_1643.xyz * _78._m13[_1743].xyz) * ((_78._m13[_1743].w * (_1421 * _1421)) * fma(0.7799999713897705078125, fast::max(dot(_1652, _1396 / float3(fast::max(_1417, 0.001000000047497451305389404296875))), 0.0), 0.2199999988079071044921875)));
    }
    float3 _1478 = mix(_78._m7.xyz, fma(_1710, _1643.xyz, ((_78._m4.xyz * fast::max(_847, 0.0)) * _1643.xyz) * fast::min(_1695, mix(1.0, _1692, _1693))) + _1744, float3(fast::clamp((_78._m8.y - _623) / (_78._m8.y - _78._m8.x), 0.0, 1.0)));
    float3 _1746;
    if (_78._m15.x > 0.5)
    {
        float4 _1510 = (_78._m1 * _78._m0) * float4(in.m_620, 1.0);
        float _1513 = fast::max(_1510.w, 9.9999997473787516355514526367188e-05);
        float4 _1543 = _560.sample(_560Smplr, float3(((_1510.xy / float2(_1513)) * 0.5) + float2(0.5), fma(log(fast::max(_1513, _78._m15.y) / _78._m15.y), _78._m15.z, (-0.5) / _78._m15.w)), level(0.0));
        _1746 = (_1478 * _1543.w) + _1543.xyz;
    }
    else
    {
        _1746 = _1478;
    }
    out.m_1016 = float4(_1746, 1.0);
    return out;
}

