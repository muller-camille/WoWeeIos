// Motion vectors for MetalFX's temporal scaler, from depth alone.
//
// Written for Metal rather than translated: the Vulkan build's
// fsr2_motion.comp does the same reprojection for FSR 2, but reprojects with
// the jittered matrices, and MetalFX wants the motion without the jitter -
// it is handed the jitter separately.
//
// Each pixel's world position comes back from its depth through the inverse
// of the jittered view-projection the world was drawn with; that position is
// then projected by this frame's and the previous frame's unjittered
// view-projections, and the difference between the two, in input pixels, is
// the motion - from where the point is now to where it was.
//
// Camera motion only. Anything that moves on its own - a character, a
// swaying branch - gets the motion of the ground it stands in front of,
// which MetalFX takes as a disocclusion and resolves with less history.

#include <metal_stdlib>
using namespace metal;

struct MotionParams {
    float4x4 invCurrentJittered;  // inverse(jittered projection * view), this frame
    float4x4 currentViewProj;     // unjittered, this frame
    float4x4 previousViewProj;    // unjittered, the frame before
    float2 size;                  // input size, pixels
};

static float2 toUV(float4 clip) {
    const float2 ndc = clip.xy / clip.w;
    // Metal's clip space has y up; a texture's v runs down.
    return float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
}

kernel void metalfx_motion(depth2d<float, access::read> depth [[texture(0)]],
                           texture2d<half, access::write> motion [[texture(1)]],
                           constant MotionParams& p [[buffer(0)]],
                           uint2 gid [[thread_position_in_grid]]) {
    if (gid.x >= uint(p.size.x) || gid.y >= uint(p.size.y)) return;

    // The sky is at the far plane, where the inverse runs to infinity; a
    // hair nearer keeps it finite and still moves it with the camera's turn.
    const float d = min(depth.read(gid), 0.99999);
    const float2 uv = (float2(gid) + 0.5) / p.size;
    const float4 ndc = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, d, 1.0);
    float4 world = p.invCurrentJittered * ndc;
    if (abs(world.w) < 1e-6) {
        motion.write(half4(0.0h), gid);
        return;
    }
    world /= world.w;

    const float4 now = p.currentViewProj * world;
    const float4 before = p.previousViewProj * world;
    if (now.w <= 1e-6 || before.w <= 1e-6) {
        motion.write(half4(0.0h), gid);
        return;
    }
    const float2 delta = (toUV(before) - toUV(now)) * p.size;
    motion.write(half4(half2(delta), 0.0h, 0.0h), gid);
}
