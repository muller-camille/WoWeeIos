#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _23
{
    int2 _m0;
    int _m1;
};

constant uint3 gl_WorkGroupSize [[maybe_unused]] = uint3(8u, 8u, 1u);

kernel void hiz_build_comp(constant _23& _25 [[buffer(0)]], texture2d<float> _61 [[texture(0)]], texture2d<float, access::write> _141 [[texture(1)]], sampler _61Smplr [[sampler(0)]], uint3 gl_GlobalInvocationID [[thread_position_in_grid]])
{
    do
    {
        int2 _17 = int2(gl_GlobalInvocationID.xy);
        bool _30 = _17.x >= _25._m0.x;
        bool _40;
        if (!_30)
        {
            _40 = _17.y >= _25._m0.y;
        }
        else
        {
            _40 = _30;
        }
        if (_40)
        {
            break;
        }
        int2 _48 = _17 * int2(2);
        float _154;
        float _155;
        float _156;
        float _157;
        if (_25._m1 == 0)
        {
            _157 = _61.read(uint2((_48 + int2(1))), 0).x;
            _156 = _61.read(uint2((_48 + int2(0, 1))), 0).x;
            _155 = _61.read(uint2((_48 + int2(1, 0))), 0).x;
            _154 = _61.read(uint2(_48), 0).x;
        }
        else
        {
            int _100 = _25._m1 - 1;
            _157 = _61.read(uint2((_48 + int2(1))), _100).x;
            _156 = _61.read(uint2((_48 + int2(0, 1))), _100).x;
            _155 = _61.read(uint2((_48 + int2(1, 0))), _100).x;
            _154 = _61.read(uint2(_48), _100).x;
        }
        _141.write(float4(fast::max(fast::max(_154, _155), fast::max(_156, _157))), uint2(_17));
        break;
    } while(false);
}

