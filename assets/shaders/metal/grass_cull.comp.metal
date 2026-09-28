#pragma clang diagnostic ignored "-Wunused-variable"

#include <metal_stdlib>
#include <simd/simd.h>
#include <metal_atomic>

using namespace metal;

struct _21
{
    float4 _m0[6];
    float4 _m1;
    uint _m2;
    uint _m3;
};

struct _37
{
    float4 _m0;
    float4 _m1;
    float4 _m2;
    float4 _m3;
};

struct _39
{
    _37 _m0[1];
};

struct _157
{
    uint _m0;
    uint _m1;
    uint _m2;
    int _m3;
    uint _m4;
};

struct _163
{
    uint _m0[1];
};

constant uint3 gl_WorkGroupSize [[maybe_unused]] = uint3(64u, 1u, 1u);

kernel void grass_cull_comp(constant _21& _23 [[buffer(0)]], const device _39& _41 [[buffer(1)]], device _157& _159 [[buffer(2)]], device _163& _165 [[buffer(3)]], uint3 gl_GlobalInvocationID [[thread_position_in_grid]])
{
    do
    {
        if (gl_GlobalInvocationID.x >= _23._m2)
        {
            break;
        }
        float _72 = abs(_41._m0[gl_GlobalInvocationID.x]._m3.w);
        float3 _79 = _41._m0[gl_GlobalInvocationID.x]._m0.xyz - _23._m1.xyz;
        bool _88 = (_23._m3 & 2u) == 0u;
        bool _100;
        if (_88)
        {
            _100 = dot(_79, _79) > fast::min(_23._m1.w, _72 * _72);
        }
        else
        {
            _100 = _88;
        }
        if (_100)
        {
            break;
        }
        float _109 = fast::max(_41._m0[gl_GlobalInvocationID.x]._m0.w, _41._m0[gl_GlobalInvocationID.x]._m1.y);
        float3 _119 = _41._m0[gl_GlobalInvocationID.x]._m0.xyz + float3(0.0, 0.0, _41._m0[gl_GlobalInvocationID.x]._m0.w * 0.5);
        if ((_23._m3 & 1u) == 0u)
        {
            bool _191;
            int _188 = 0;
            for (;;)
            {
                if (_188 < 6)
                {
                    if ((dot(_23._m0[_188].xyz, _119) + _23._m0[_188].w) < (_109 * (-0.75)))
                    {
                        _191 = true;
                        break;
                    }
                    _188++;
                    continue;
                }
                else
                {
                    _191 = false;
                    break;
                }
            }
            if (_191)
            {
                break;
            }
        }
        uint _161 = atomic_fetch_add_explicit((device atomic_uint*)&_159._m1, 1u, memory_order_relaxed);
        _165._m0[_161] = gl_GlobalInvocationID.x;
        break;
    } while(false);
}

