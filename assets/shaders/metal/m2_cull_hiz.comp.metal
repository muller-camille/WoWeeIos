#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _22
{
    float4 _m0[6];
    float4 _m1;
    uint _m2;
    uint _m3;
    uint _m4;
    float4 _m5;
    char _m6_pad[64];
    float4x4 _m6;
};

struct _38
{
    float4 _m0;
    float _m1;
    uint _m2;
    float _m3;
    float _m4;
};

struct _40
{
    _38 _m0[1];
};

struct _80
{
    uint _m0[1];
};

constant uint3 gl_WorkGroupSize [[maybe_unused]] = uint3(64u, 1u, 1u);

kernel void m2_cull_hiz_comp(constant _22& _24 [[buffer(0)]], const device _40& _42 [[buffer(1)]], device _80& _82 [[buffer(2)]], texture2d<float> _351 [[texture(0)]], sampler _351Smplr [[sampler(0)]], uint3 gl_GlobalInvocationID [[thread_position_in_grid]])
{
    do
    {
        if (gl_GlobalInvocationID.x >= _24._m2)
        {
            break;
        }
        float4 _476 = _42._m0[gl_GlobalInvocationID.x]._m0;
        float _478 = _42._m0[gl_GlobalInvocationID.x]._m1;
        uint _480 = _42._m0[gl_GlobalInvocationID.x]._m2;
        bool _69 = (_480 & 1u) == 0u;
        bool _76;
        if (!_69)
        {
            _76 = (_480 & 6u) != 0u;
        }
        else
        {
            _76 = _69;
        }
        if (_76)
        {
            _82._m0[gl_GlobalInvocationID.x] = 0u;
            break;
        }
        float3 _96 = _476.xyz - _24._m1.xyz;
        float _100 = dot(_96, _96);
        if (_100 > _24._m1.w)
        {
            _82._m0[gl_GlobalInvocationID.x] = 0u;
            break;
        }
        if (_100 > _478)
        {
            _82._m0[gl_GlobalInvocationID.x] = 0u;
            break;
        }
        bool _126 = _476.w > 0.0;
        if (_126)
        {
            bool _449;
            int _445 = 0;
            for (;;)
            {
                if (_445 < 6)
                {
                    if ((dot(_24._m0[_445].xyz, _476.xyz) + _24._m0[_445].w) < (-_476.w))
                    {
                        _82._m0[gl_GlobalInvocationID.x] = 0u;
                        _449 = true;
                        break;
                    }
                    _445++;
                    continue;
                }
                else
                {
                    _449 = false;
                    break;
                }
            }
            if (_449)
            {
                break;
            }
        }
        if (((_24._m3 != 0u) && _126) && ((_480 & 8u) != 0u))
        {
            float _182 = _476.w * 1.5;
            float4 _196 = _24._m6 * float4(_476.xyz, 1.0);
            float _198 = _196.w;
            if (_198 > 0.0)
            {
                float3 _208 = _196.xyz / float3(_198);
                float2 _253 = (_208.xy * 0.5) + float2(0.5);
                float2 _260 = float2(fast::max((_182 * length(float3(_24._m6[0].x, _24._m6[1].x, _24._m6[2].x))) / _198, (_182 * length(float3(_24._m6[0].y, _24._m6[1].y, _24._m6[2].y))) / _198) * 0.5);
                float2 _261 = _253 - _260;
                float2 _266 = _253 + _260;
                float _268 = _261.x;
                bool _270 = _268 >= 0.0199999995529651641845703125;
                bool _276;
                if (_270)
                {
                    _276 = _261.y >= 0.0199999995529651641845703125;
                }
                else
                {
                    _276 = _270;
                }
                bool _283;
                if (_276)
                {
                    _283 = _266.x <= 0.980000019073486328125;
                }
                else
                {
                    _283 = _276;
                }
                bool _289;
                if (_283)
                {
                    _289 = _266.y <= 0.980000019073486328125;
                }
                else
                {
                    _289 = _283;
                }
                bool _297;
                if (_289)
                {
                    _297 = _266.x > _268;
                }
                else
                {
                    _297 = _289;
                }
                bool _305;
                if (_297)
                {
                    _305 = _266.y > _261.y;
                }
                else
                {
                    _305 = _297;
                }
                if (_305)
                {
                    float _310 = _266.x;
                    float _319 = _266.y;
                    float _321 = _261.y;
                    float _329 = fast::max((_310 - _268) * _24._m5.x, (_319 - _321) * _24._m5.y);
                    if (_329 >= 6.0)
                    {
                        float _346 = fast::clamp(ceil(log2(fast::max(_329, 1.0))) + 1.0, 0.0, float(_24._m4 - 1u));
                        float _390 = fast::max(fast::max(_351.sample(_351Smplr, _261, level(_346)).x, _351.sample(_351Smplr, float2(_310, _321), level(_346)).x), fast::max(_351.sample(_351Smplr, float2(_268, _319), level(_346)).x, _351.sample(_351Smplr, _266, level(_346)).x));
                        if ((((_208.z - ((_182 * length(float3(_24._m6[0].z, _24._m6[1].z, _24._m6[2].z))) / _198)) - 0.0199999995529651641845703125) > _390) && (_390 < 1.0))
                        {
                            _82._m0[gl_GlobalInvocationID.x] = 0u;
                            break;
                        }
                    }
                }
            }
        }
        _82._m0[gl_GlobalInvocationID.x] = 1u;
        break;
    } while(false);
}

