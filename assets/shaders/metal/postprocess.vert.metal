#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct postprocess_vert_out
{
    float2 m_9 [[user(locn0)]];
    float4 gl_Position [[position]];
};

vertex postprocess_vert_out postprocess_vert(uint gl_VertexIndex [[vertex_id]])
{
    postprocess_vert_out out = {};
    out.m_9 = float2(float((int(gl_VertexIndex) << 1) & 2), float(int(gl_VertexIndex) & 2));
    out.gl_Position = float4((out.m_9 * 2.0) - float2(1.0), 0.0, 1.0);
    out.gl_Position.y = -(out.gl_Position.y);    // Invert Y-axis for Metal
    return out;
}

