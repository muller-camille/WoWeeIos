#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _45
{
    float4x4 _m0;
    float4 _m1;
};

struct selection_circle_frag_out
{
    float4 m_43 [[color(0)]];
};

struct selection_circle_frag_in
{
    float2 m_11 [[user(locn0)]];
};

fragment selection_circle_frag_out selection_circle_frag(selection_circle_frag_in in [[stage_in]], constant _45& _47 [[buffer(0)]])
{
    selection_circle_frag_out out = {};
    float _13 = length(in.m_11);
    float _33 = fast::max(smoothstep(0.930000007152557373046875, 0.9700000286102294921875, _13) * smoothstep(1.0, 0.9700000286102294921875, _13), (1.0 - smoothstep(0.0, 0.930000007152557373046875, _13)) * 0.1500000059604644775390625);
    if (_33 < 0.00999999977648258209228515625)
    {
        discard_fragment();
    }
    out.m_43 = float4(_47._m1.xyz, _33);
    return out;
}

