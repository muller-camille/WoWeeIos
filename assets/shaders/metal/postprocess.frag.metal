#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct postprocess_frag_out
{
    float4 m_63 [[color(0)]];
};

struct postprocess_frag_in
{
    float2 m_17 [[user(locn0)]];
};

fragment postprocess_frag_out postprocess_frag(postprocess_frag_in in [[stage_in]], texture2d<float> _13 [[texture(0)]], sampler _13Smplr [[sampler(0)]])
{
    postprocess_frag_out out = {};
    float3 _22 = _13.sample(_13Smplr, in.m_17).xyz;
    for (int _70 = 0; _70 < 3; _70++)
    {
        if (_22[_70] > 0.89999997615814208984375)
        {
            _22[_70] = 0.89999997615814208984375 + ((0.100000001490116119384765625 * (_22[_70] - 0.89999997615814208984375)) / (_22[_70] + (-0.7999999523162841796875)));
        }
    }
    out.m_63 = float4(_22, 1.0);
    return out;
}

