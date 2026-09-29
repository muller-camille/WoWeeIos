#pragma once
namespace simd {
struct float2 { float x, y; };
struct float3 { float x, y, z, _p; };
struct float4 { float x, y, z, w; };
struct float4x4 { float4 columns[4]; };
struct float3x3 { float3 columns[3]; };
struct packed_float3 { float x, y, z; };
}
typedef simd::float2 simd_float2;
typedef simd::float4x4 simd_float4x4;
