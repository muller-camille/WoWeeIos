#pragma once

#include <cstdint>

namespace MTL {
class CommandBuffer;
class RenderPipelineState;
class Texture;
}  // namespace MTL

namespace wowee {
namespace rendering {

class MetalContext;

/// The full-screen passes of the Vulkan build's PostProcessPipeline that the
/// Metal frame has (docs/plan-metal.md, M4): FSR 1's upscale, for a device
/// MetalFX does not run on, and FXAA. Each is one triangle from
/// postprocess_vert over the whole of its target, in a pass of its own.
class MetalPostProcess {
public:
    MetalPostProcess() = default;
    ~MetalPostProcess();

    MetalPostProcess(const MetalPostProcess&) = delete;
    MetalPostProcess& operator=(const MetalPostProcess&) = delete;

    /// Both pipelines, drawing into colorFormat (an MTL::PixelFormat). False
    /// when either is refused; nothing is drawn then.
    [[nodiscard]] bool initialize(MetalContext* ctx, uint32_t colorFormat);
    void shutdown();

    /// FSR 1's edge-adaptive upscale (fsr_easu), as PostProcessPipeline's
    /// renderFSRUpscale draws it: source, inW x inH, drawn up to fill target,
    /// outW x outH. sharpness is the menu's, 0 to 2.
    void encodeUpscale(MTL::CommandBuffer* commandBuffer, MTL::Texture* source, uint32_t inW,
                       uint32_t inH, MTL::Texture* target, uint32_t outW, uint32_t outH,
                       float sharpness);

    /// FXAA 3.11 from source into target, both w x h. intoxication, 0 to 1,
    /// is the drunk blur the Vulkan pass carries in the same push block.
    void encodeFxaa(MTL::CommandBuffer* commandBuffer, MTL::Texture* source, MTL::Texture* target,
                    uint32_t w, uint32_t h, float intoxication);

private:
    MetalContext* metal_ = nullptr;
    MTL::RenderPipelineState* easu_ = nullptr;
    MTL::RenderPipelineState* fxaa_ = nullptr;
    int easuTexture_ = -1, easuSampler_ = -1, easuPush_ = -1;
    int fxaaTexture_ = -1, fxaaSampler_ = -1, fxaaPush_ = -1;
};

}  // namespace rendering
}  // namespace wowee
