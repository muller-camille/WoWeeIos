#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _47
{
    float4 _m0;
    float4 _m1;
};

constant uint3 gl_WorkGroupSize [[maybe_unused]] = uint3(8u, 8u, 1u);

kernel void sun_shafts_comp(constant _47& _49 [[buffer(0)]], texture2d<float, access::write> _22 [[texture(0)]], texture2d<float> _127 [[texture(1)]], sampler _127Smplr [[sampler(0)]], uint3 gl_GlobalInvocationID [[thread_position_in_grid]])
{
    do
    {
        int2 _17 = int2(gl_GlobalInvocationID.xy);
        int2 _24 = int2(_22.get_width(), _22.get_height());
        if (any(_17 >= _24))
        {
            break;
        }
        float2 _38 = float2(_17);
        float2 _44 = (_38 + float2(0.5)) / float2(_24);
        float2 _58 = (_49._m0.xy - _44) * 0.01666666753590106964111328125;
        float3 _214;
        float _215;
        float2 _217;
        _217 = _44 + (_58 * fract(52.98291778564453125 * fract(dot(_38, float2(0.067110560834407806396484375, 0.005837149918079376220703125)))));
        _215 = 0.0;
        _214 = float3(0.0);
        float _97;
        float _173;
        float2 _176;
        float3 _233;
        int _213 = 0;
        float _216 = 1.0;
        for (; _213 < 48; _217 = _176, _216 = _173, _215 = _97, _214 = _233, _213++)
        {
            _97 = _215 + _216;
            bool _101 = _217.x >= 0.0;
            bool _107;
            if (_101)
            {
                _107 = _217.x <= 1.0;
            }
            else
            {
                _107 = _101;
            }
            bool _114;
            if (_107)
            {
                _114 = _217.y >= 0.0;
            }
            else
            {
                _114 = _107;
            }
            bool _120;
            if (_114)
            {
                _120 = _217.y <= 1.0;
            }
            else
            {
                _120 = _114;
            }
            if (_120)
            {
                float3 _131 = _127.sample(_127Smplr, _217, level(0.0)).xyz;
                float2 _153 = (_217 - _49._m0.xy) * float2(_49._m0.z, 1.0);
                _233 = _214 + (_131 * ((smoothstep(0.550000011920928955078125, 0.949999988079071044921875, dot(_131, float3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875))) * exp(dot(_153, _153) * (-6.25))) * _216));
            }
            else
            {
                _233 = _214;
            }
            _173 = _216 * 0.980000019073486328125;
            _176 = _217 + _58;
        }
        _22.write(float4((_214 * (_49._m0.w / _215)) * _49._m1.xyz, 1.0), uint2(_17));
        break;
    } while(false);
}

