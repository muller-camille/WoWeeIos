#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct _11
{
    float _m0;
};

struct lightning_flash_frag_out
{
    float4 m_9 [[color(0)]];
};

fragment lightning_flash_frag_out lightning_flash_frag(constant _11& _13 [[buffer(0)]])
{
    lightning_flash_frag_out out = {};
    out.m_9 = float4(1.0, 1.0, 1.0, _13._m0 * 0.60000002384185791015625);
    return out;
}

