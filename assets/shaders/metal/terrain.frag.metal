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

struct _657
{
    int _m0;
    int _m1;
    int _m2;
    int _m3;
    uint _m4; // which local lights reach this chunk (local_light_mask.hpp)
};

struct terrain_frag_out
{
    float4 m_1041 [[color(0)]];
};

struct terrain_frag_in
{
    float3 m_644 [[user(locn0)]];
    float3 m_729 [[user(locn1)]];
    float2 m_652 [[user(locn2)]];
    float2 m_667 [[user(locn3)]];
};

fragment terrain_frag_out terrain_frag(terrain_frag_in in [[stage_in]], constant _76& _78 [[buffer(0)]], constant _657& _659 [[buffer(1)]], texture2d<float> _141 [[texture(0)]], texture2d<float> _274 [[texture(1)]], texture3d<float> _584 [[texture(2)]], texture2d<float> _649 [[texture(3)]], texture2d<float> _666 [[texture(4)]], texture2d<float> _677 [[texture(5)]], texture2d<float> _690 [[texture(6)]], texture2d<float> _699 [[texture(7)]], texture2d<float> _712 [[texture(8)]], texture2d<float> _721 [[texture(9)]], depth2d<float> _982 [[texture(10)]], sampler _141Smplr [[sampler(0)]], sampler _274Smplr [[sampler(1)]], sampler _584Smplr [[sampler(2)]], sampler _649Smplr [[sampler(3)]], sampler _666Smplr [[sampler(4)]], sampler _677Smplr [[sampler(5)]], sampler _690Smplr [[sampler(6)]], sampler _699Smplr [[sampler(7)]], sampler _712Smplr [[sampler(8)]], sampler _721Smplr [[sampler(9)]], sampler _982Smplr [[sampler(10)]])
{
    terrain_frag_out out = {};
    float _647 = length(_78._m6.xyz - in.m_644);
    float4 _654 = _649.sample(_649Smplr, in.m_652);
    float4 _1691;
    if (_659._m1 != 0)
    {
        float4 _1063 = _666.sample(_666Smplr, in.m_667);
        float _1064 = _1063.x;
        float4 _1692;
        if (_1064 > 0.00200000009499490261077880859375)
        {
            _1692 = mix(_654, _677.sample(_677Smplr, in.m_652), float4(_1064));
        }
        else
        {
            _1692 = _654;
        }
        _1691 = _1692;
    }
    else
    {
        _1691 = _654;
    }
    float4 _1693;
    if (_659._m2 != 0)
    {
        float4 _1069 = _690.sample(_690Smplr, in.m_667);
        float _1070 = _1069.x;
        float4 _1694;
        if (_1070 > 0.00200000009499490261077880859375)
        {
            _1694 = mix(_1691, _699.sample(_699Smplr, in.m_652), float4(_1070));
        }
        else
        {
            _1694 = _1691;
        }
        _1693 = _1694;
    }
    else
    {
        _1693 = _1691;
    }
    float4 _1695;
    if (_659._m3 != 0)
    {
        float4 _1075 = _712.sample(_712Smplr, in.m_667);
        float _1076 = _1075.x;
        float4 _1696;
        if (_1076 > 0.00200000009499490261077880859375)
        {
            _1696 = mix(_1693, _721.sample(_721Smplr, in.m_652), float4(_1076));
        }
        else
        {
            _1696 = _1693;
        }
        _1695 = _1696;
    }
    else
    {
        _1695 = _1693;
    }
    float3 _731 = fast::normalize(in.m_729);
    float _757 = (1.0 - smoothstep(50.0, 125.0, _647)) * smoothstep(0.0, 0.0599999986588954925537109375, fast::min(fast::min(in.m_667.x, 1.0 - in.m_667.x), fast::min(in.m_667.y, 1.0 - in.m_667.y)));
    float3 _1704;
    if (_757 > 0.001000000047497451305389404296875)
    {
        float _769 = dot(_1695.xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
        float _772 = dfdx(_769);
        float _775 = dfdy(_769);
        float3 _778 = dfdx(in.m_644);
        float3 _781 = dfdy(in.m_644);
        float3 _785 = cross(_731, _781);
        float3 _789 = cross(_778, _731);
        float _792 = length(_785);
        float _795 = length(_789);
        float3 _1705;
        if ((_792 > 9.9999999747524270787835121154785e-07) && (_795 > 9.9999999747524270787835121154785e-07))
        {
            float3 _821 = (((_785 / float3(_792)) * _772) + ((_789 / float3(_795)) * _775)) * (9.0 * _757);
            float _824 = length(_821);
            float3 _1698;
            if (_824 > 0.699999988079071044921875)
            {
                _1698 = _821 * (0.699999988079071044921875 / _824);
            }
            else
            {
                _1698 = _821;
            }
            float3 _837 = _731 - _1698;
            float _841 = dot(_837, _837);
            float3 _1699;
            if (_841 > 9.9999999392252902907785028219223e-09)
            {
                _1699 = _837 * rsqrt(_841);
            }
            else
            {
                _1699 = _731;
            }
            _1705 = _1699;
        }
        else
        {
            _1705 = _731;
        }
        _1704 = _1705;
    }
    else
    {
        _1704 = _731;
    }
    float _872 = dot(_1704, fast::normalize(-_78._m3.xyz));
    float _1746;
    if (_78._m9.x > 0.5)
    {
        float _1839 = (_78._m9.z > 0.0) ? _78._m9.z : 0.000244140625;
        float _904 = 1.0 - abs(_872);
        float4 _920 = _78._m2 * float4(in.m_644 + (_1704 * ((_1839 * 2.0) * _904)), 1.0);
        float3 _927 = _920.xyz / float3(_920.w);
        float2 _932 = (_927.xy * 0.5) + float2(0.5);
        float _934 = _932.x;
        float _936 = _932.y;
        bool _939 = _934 >= 0.0;
        bool _945;
        if (_939)
        {
            _945 = _934 <= 1.0;
        }
        else
        {
            _945 = _939;
        }
        bool _951;
        if (_945)
        {
            _951 = _936 >= 0.0;
        }
        else
        {
            _951 = _945;
        }
        bool _957;
        if (_951)
        {
            _957 = _936 <= 1.0;
        }
        else
        {
            _957 = _951;
        }
        bool _963;
        if (_957)
        {
            _963 = _927.z >= 0.0;
        }
        else
        {
            _963 = _957;
        }
        bool _969;
        if (_963)
        {
            _969 = _927.z <= 1.0;
        }
        else
        {
            _969 = _963;
        }
        float _1747;
        if (_969)
        {
            float _988 = _927.z - fast::max(0.0005000000237487256526947021484375 * _904, 4.9999998736893758177757263183594e-05);
            float _1093 = 0.5 * _1839;
            float2 _1096 = float3(_934, _936, _988).xy;
            float _1098 = _1839 * (-0.5);
            _1747 = mix(1.0, 0.25 * (((_982.sample_compare(_982Smplr, float3(_1096 + float2(_1098), _988).xy, _988) + _982.sample_compare(_982Smplr, float3(_1096 + float2(_1093, _1098), _988).xy, _988)) + _982.sample_compare(_982Smplr, float3(_1096 + float2(_1098, _1093), _988).xy, _988)) + _982.sample_compare(_982Smplr, float3(_1096 + float2(_1093), _988).xy, _988)), _78._m9.y);
        }
        else
        {
            _1747 = 1.0;
        }
        _1746 = _1747;
    }
    else
    {
        _1746 = 1.0;
    }
    float _1198;
    float3 _1741;
    float _1742;
    float _1743;
    float _1744;
    do
    {
        _1198 = _78._m18.x;
        if (_1198 < 0.5)
        {
            _1744 = 0.0;
            _1743 = 1.0;
            _1742 = 1.0;
            _1741 = float3(0.0);
            break;
        }
        float4 _1210 = _78._m16 * float4(in.m_644, 1.0);
        float _1212 = _1210.w;
        if (_1212 <= 0.0)
        {
            _1744 = 0.0;
            _1743 = 1.0;
            _1742 = 1.0;
            _1741 = float3(0.0);
            break;
        }
        float2 _1225 = ((_1210.xy / float2(_1212)) * 0.5) + float2(0.5);
        bool _1228 = any(_1225 < float2(0.0));
        bool _1235;
        if (!_1228)
        {
            _1235 = any(_1225 > float2(1.0));
        }
        else
        {
            _1235 = _1228;
        }
        if (_1235)
        {
            _1744 = 0.0;
            _1743 = 1.0;
            _1742 = 1.0;
            _1741 = float3(0.0);
            break;
        }
        int2 _1241 = int2(_141.get_width(), _141.get_height());
        float2 _1247 = fma(_1225, float2(_1241), float2(-0.5));
        int2 _1252 = int2(floor(_1247 + float2(0.5)));
        float _1258 = length(in.m_644 - _78._m17.xyz);
        float _1261 = fma(0.02999999932944774627685546875, _1258, 0.0500000007450580596923828125);
        bool _1264 = _1198 > 2.5;
        float _1715;
        float _1716;
        float2 _1717;
        float3 _1718;
        _1718 = float3(0.0);
        _1717 = float2(0.0);
        _1716 = 0.0;
        _1715 = 0.0;
        float _1731;
        float _1733;
        float2 _1735;
        float3 _1737;
        for (int _1714 = -1; _1714 <= 1; _1718 = _1737, _1717 = _1735, _1716 = _1731, _1715 = _1733, _1714++)
        {
            _1737 = _1718;
            _1735 = _1717;
            _1733 = _1715;
            _1731 = _1716;
            float _1820;
            float _1822;
            float2 _1824;
            float3 _1826;
            for (int _1727 = -1; _1727 <= 1; _1737 = _1826, _1735 = _1824, _1733 = _1822, _1731 = _1820, _1727++)
            {
                int2 _1283 = clamp(_1252 + int2(_1727, _1714), int2(0), _1241 - int2(1));
                float4 _1287 = _141.read(uint2(_1283), 0);
                float _1289 = _1287.w;
                float _1293 = (_1289 - _1258) / _1261;
                float _1729;
                if (_1289 > 0.0)
                {
                    _1729 = exp((-_1293) * _1293);
                }
                else
                {
                    _1729 = 0.0;
                }
                float2 _1309 = float2(_1283) - _1247;
                float _1315 = exp(-dot(_1309, _1309));
                float _1316 = _1729 * _1315;
                if (_1316 <= 9.9999997473787516355514526367188e-05)
                {
                    _1826 = _1737;
                    _1824 = _1735;
                    _1822 = _1733;
                    _1820 = _1731;
                    continue;
                }
                float3 _1827;
                if (_1264)
                {
                    _1827 = _1737 + (_274.read(uint2(_1283), 0).xyz * _1316);
                }
                else
                {
                    _1827 = _1737;
                }
                _1826 = _1827;
                _1824 = _1735 + (_1287.xy * _1316);
                _1822 = fma(_1729, _1315, _1733);
                _1820 = fast::max(_1731, _1729);
            }
        }
        if (_1715 <= 9.9999997473787516355514526367188e-05)
        {
            _1744 = 0.0;
            _1743 = 1.0;
            _1742 = 1.0;
            _1741 = float3(0.0);
            break;
        }
        _1744 = smoothstep(0.100000001490116119384765625, 0.60000002384185791015625, _1716);
        _1743 = _1717.x / _1715;
        _1742 = _1717.y / _1715;
        _1741 = _1718 / float3(_1715);
        break;
    } while(false);
    float3 _1761;
    do
    {
        if (_1198 < 1.5)
        {
            _1761 = _78._m5.xyz;
            break;
        }
        float3 _1760;
        if (_1198 > 2.5)
        {
            _1760 = _1741;
        }
        else
        {
            _1760 = _78._m5.xyz * _1742;
        }
        _1761 = mix(_78._m5.xyz, _1760, float3(_1744));
        break;
    } while(false);
    float3 _1795;
    _1795 = float3(0.0);
    float3 _1828;
    for (int _1794 = 0; _1794 < min(_78._m14.x, 64); _1795 = _1828, _1794++)
    {
        // Not a light the CPU found reaching this chunk: skipped before its
        // data is read. The same for every pixel of the draw.
        if (_1794 < 32 && ((_659._m4 >> uint(_1794)) & 1u) == 0u)
        {
            _1828 = _1795;
            continue;
        }
        float3 _1445 = _78._m12[_1794].xyz - in.m_644;
        float _1451 = dot(_1445, _1445);
        bool _1453 = _78._m12[_1794].w <= 0.0;
        bool _1462;
        if (!_1453)
        {
            _1462 = _1451 >= (_78._m12[_1794].w * _78._m12[_1794].w);
        }
        else
        {
            _1462 = _1453;
        }
        if (_1462)
        {
            _1828 = _1795;
            continue;
        }
        float _1466 = sqrt(_1451);
        float _1470 = 1.0 - (_1466 / _78._m12[_1794].w);
        _1828 = _1795 + ((_1695.xyz * _78._m13[_1794].xyz) * ((_78._m13[_1794].w * (_1470 * _1470)) * fma(0.7799999713897705078125, fast::max(dot(_1704, _1445 / float3(fast::max(_1466, 0.001000000047497451305389404296875))), 0.0), 0.2199999988079071044921875)));
    }
    float3 _1527 = mix(_78._m7.xyz, fma(_1761, _1695.xyz, ((_78._m4.xyz * fast::max(_872, 0.0)) * _1695.xyz) * fast::min(_1746, mix(1.0, _1743, _1744))) + _1795, float3(fast::clamp((_78._m8.y - _647) / (_78._m8.y - _78._m8.x), 0.0, 1.0)));
    float3 _1797;
    if (_78._m15.x > 0.5)
    {
        float4 _1559 = (_78._m1 * _78._m0) * float4(in.m_644, 1.0);
        float _1562 = fast::max(_1559.w, 9.9999997473787516355514526367188e-05);
        float4 _1592 = _584.sample(_584Smplr, float3(((_1559.xy / float2(_1562)) * 0.5) + float2(0.5), fma(log(fast::max(_1562, _78._m15.y) / _78._m15.y), _78._m15.z, (-0.5) / _78._m15.w)), level(0.0));
        _1797 = (_1527 * _1592.w) + _1592.xyz;
    }
    else
    {
        _1797 = _1527;
    }
    out.m_1041 = float4(_1797, 1.0);
    return out;
}

