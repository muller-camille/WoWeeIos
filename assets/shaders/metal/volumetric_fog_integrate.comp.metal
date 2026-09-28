#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _27
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
};

struct _52
{
    float4 _m0;
    float4 _m1[4];
    float4x4 _m2;
    float4 _m3;
    float4 _m4;
    float4 _m5;
    float4 _m6;
    int4 _m7;
};

constant uint3 gl_WorkGroupSize [[maybe_unused]] = uint3(8u, 8u, 1u);

kernel void volumetric_fog_integrate_comp(constant _27& _29 [[buffer(0)]], constant _52& _54 [[buffer(1)]], texture3d<float> _165 [[texture(0)]], texture3d<float, access::write> _205 [[texture(1)]], sampler _165Smplr [[sampler(0)]], uint3 gl_GlobalInvocationID [[thread_position_in_grid]])
{
    do
    {
        int2 _99 = int2(gl_GlobalInvocationID.xy);
        if (any(_99 >= _54._m7.xy))
        {
            break;
        }
        float2 _123 = (float2(_99) + float2(0.5)) / float2(_54._m7.xy);
        float3 _240 = float3(_123.x);
        float _126 = length(mix(mix(_54._m1[0].xyz, _54._m1[1].xyz, _240), mix(_54._m1[2].xyz, _54._m1[3].xyz, _240), float3(_123.y)));
        float _267 = _29._m15.w * _29._m15.z;
        float3 _291;
        _291 = float3(0.0);
        int _288 = 0;
        float _289 = _29._m15.y;
        float _290 = 1.0;
        for (; _288 < _54._m7.z; )
        {
            int _149 = _288 + 1;
            float _283 = _29._m15.y * exp(float(_149) / _267);
            int3 _172 = int3(_99, _288);
            float4 _174 = _165.read(uint3(_172), 0);
            float _179 = fast::max(_174.w, 9.9999999747524270787835121154785e-07);
            float _185 = exp((-_179) * ((_283 - _289) * _126));
            float3 _188 = _174.xyz;
            float3 _199 = _291 + (((_188 - (_188 * _185)) * _290) / float3(_179));
            float _202 = _290 * _185;
            _205.write(float4(_199, _202), uint3(_172));
            _291 = _199;
            _290 = _202;
            _289 = _283;
            _288 = _149;
            continue;
        }
        break;
    } while(false);
}

