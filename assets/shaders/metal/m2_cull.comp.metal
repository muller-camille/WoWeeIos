#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _21
{
    float4 _m0[6];
    float4 _m1;
    uint _m2;
    uint _m3;
    uint _m4;
    uint _m5;
};

struct _37
{
    float4 _m0;
    float _m1;
    uint _m2;
    float _m3;
    float _m4;
};

struct _39
{
    _37 _m0[1];
};

struct _79
{
    uint _m0[1];
};

constant uint3 gl_WorkGroupSize [[maybe_unused]] = uint3(64u, 1u, 1u);

kernel void m2_cull_comp(constant _21& _23 [[buffer(0)]], const device _39& _41 [[buffer(1)]], device _79& _81 [[buffer(2)]], uint3 gl_GlobalInvocationID [[thread_position_in_grid]])
{
    do
    {
        if (gl_GlobalInvocationID.x >= _23._m2)
        {
            break;
        }
        float4 _186 = _41._m0[gl_GlobalInvocationID.x]._m0;
        float _188 = _41._m0[gl_GlobalInvocationID.x]._m1;
        bool _68 = (_41._m0[gl_GlobalInvocationID.x]._m2 & 1u) == 0u;
        bool _75;
        if (!_68)
        {
            _75 = (_41._m0[gl_GlobalInvocationID.x]._m2 & 6u) != 0u;
        }
        else
        {
            _75 = _68;
        }
        if (_75)
        {
            _81._m0[gl_GlobalInvocationID.x] = 0u;
            break;
        }
        float3 _95 = _186.xyz - _23._m1.xyz;
        float _99 = dot(_95, _95);
        if (_99 > _23._m1.w)
        {
            _81._m0[gl_GlobalInvocationID.x] = 0u;
            break;
        }
        if (_99 > _188)
        {
            _81._m0[gl_GlobalInvocationID.x] = 0u;
            break;
        }
        if (_186.w > 0.0)
        {
            bool _182;
            int _181 = 0;
            for (;;)
            {
                if (_181 < 6)
                {
                    if ((dot(_23._m0[_181].xyz, _186.xyz) + _23._m0[_181].w) < (-_186.w))
                    {
                        _81._m0[gl_GlobalInvocationID.x] = 0u;
                        _182 = true;
                        break;
                    }
                    _181++;
                    continue;
                }
                else
                {
                    _182 = false;
                    break;
                }
            }
            if (_182)
            {
                break;
            }
        }
        _81._m0[gl_GlobalInvocationID.x] = 1u;
        break;
    } while(false);
}

