#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _11
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
};

struct _56
{
    float _m0;
};

struct m2_smoke_vert_out
{
    float m_70 [[user(locn0)]];
    float m_73 [[user(locn1)]];
    float4 gl_Position [[position]];
    float gl_PointSize [[point_size]];
};

struct m2_smoke_vert_in
{
    float3 m_21 [[attribute(0)]];
    float m_71 [[attribute(1)]];
    float m_52 [[attribute(2)]];
    float m_38 [[attribute(3)]];
};

vertex m2_smoke_vert_out m2_smoke_vert(m2_smoke_vert_in in [[stage_in]], constant _11& _13 [[buffer(0)]], constant _56& _58 [[buffer(1)]])
{
    m2_smoke_vert_out out = {};
    float4 _28 = _13._m0 * float4(in.m_21, 1.0);
    out.gl_PointSize = fast::clamp(((in.m_52 * ((in.m_38 > 0.5) ? 0.119999997317790985107421875 : 0.300000011920928955078125)) * _58._m0) / fast::max(-_28.z, 1.0), 1.0, 64.0);
    out.m_70 = in.m_71;
    out.m_73 = in.m_38;
    out.gl_Position = _13._m1 * _28;
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

