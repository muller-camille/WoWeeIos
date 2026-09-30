#include "rendering/metal/metal_post_process.hpp"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <glm/glm.hpp>

#include "rendering/metal/metal_context.hpp"
#include "rendering/metal/metal_pipeline.hpp"

namespace wowee {
namespace rendering {

namespace {

/// fsr_easu.frag's push block, filled as renderFSRUpscale fills it.
struct EasuPush {
    glm::vec4 con0;  // inputSize.xy, 1/inputSize.xy
    glm::vec4 con1;  // inputSize.xy / outputSize.xy, 0.5 * inputSize.xy / outputSize.xy
    glm::vec4 con2;  // outputSize.xy, 1/outputSize.xy
    glm::vec4 con3;  // sharpness, 0, 0, 0
};

/// fxaa.frag's: the pixel's size, the unsharp mask renderFXAAPass leaves at
/// zero now that RCAS sharpens ahead of it, and how drunk the player is.
struct FxaaPush {
    float rcpFrame[2];
    float sharpness;
    float intoxication;
};

/// A pass over target that its one triangle covers entirely: nothing of what
/// was there is loaded.
MTL::RenderCommandEncoder* beginFullScreenPass(MTL::CommandBuffer* commandBuffer,
                                               MTL::Texture* target, const char* label) {
    auto* pass = MTL::RenderPassDescriptor::renderPassDescriptor();
    auto* color = pass->colorAttachments()->object(0);
    color->setTexture(target);
    color->setLoadAction(MTL::LoadActionDontCare);
    color->setStoreAction(MTL::StoreActionStore);
    MTL::RenderCommandEncoder* encoder = commandBuffer->renderCommandEncoder(pass);
    encoder->setLabel(NS::String::string(label, NS::UTF8StringEncoding));
    encoder->setCullMode(MTL::CullModeNone);
    return encoder;
}

}  // namespace

MetalPostProcess::~MetalPostProcess() {
    shutdown();
}

bool MetalPostProcess::initialize(MetalContext* ctx, uint32_t colorFormat) {
    if (!ctx) return false;
    const MetalBindings easu("fsr_easu_frag");
    const MetalBindings fxaa("fxaa_frag");
    easuTexture_ = easu.texture(0, 0);
    easuSampler_ = easu.sampler(0, 0);
    easuPush_ = easu.pushConstants();
    fxaaTexture_ = fxaa.texture(0, 0);
    fxaaSampler_ = fxaa.sampler(0, 0);
    fxaaPush_ = fxaa.pushConstants();
    if (!easu.valid() || !fxaa.valid()) return false;

    // The full-screen triangle postprocess_vert makes from the vertex index;
    // no depth, no blending, one sample, as both Vulkan pipelines are.
    MetalPipelineDesc desc;
    desc.vertexFunction = "postprocess_vert";
    desc.fragmentFunction = "fsr_easu_frag";
    desc.colorFormat = colorFormat;
    desc.label = "FSR 1 upscale";
    easu_ = buildMetalPipeline(*ctx, desc);
    desc.fragmentFunction = "fxaa_frag";
    desc.label = "FXAA";
    fxaa_ = buildMetalPipeline(*ctx, desc);
    if (!easu_ || !fxaa_) {
        shutdown();
        return false;
    }
    metal_ = ctx;
    return true;
}

void MetalPostProcess::shutdown() {
    if (easu_) { easu_->release(); easu_ = nullptr; }
    if (fxaa_) { fxaa_->release(); fxaa_ = nullptr; }
    metal_ = nullptr;
}

void MetalPostProcess::encodeUpscale(MTL::CommandBuffer* commandBuffer, MTL::Texture* source,
                                     uint32_t inW, uint32_t inH, MTL::Texture* target,
                                     uint32_t outW, uint32_t outH, float sharpness) {
    if (!easu_ || !commandBuffer || !source || !target || !inW || !inH || !outW || !outH) return;
    const float iw = static_cast<float>(inW), ih = static_cast<float>(inH);
    const float ow = static_cast<float>(outW), oh = static_cast<float>(outH);
    const EasuPush push{
        glm::vec4(iw, ih, 1.0f / iw, 1.0f / ih),
        glm::vec4(iw / ow, ih / oh, 0.5f * iw / ow, 0.5f * ih / oh),
        glm::vec4(ow, oh, 1.0f / ow, 1.0f / oh),
        glm::vec4(sharpness, 0.0f, 0.0f, 0.0f),
    };
    MTL::RenderCommandEncoder* encoder = beginFullScreenPass(commandBuffer, target, "FSR 1 upscale");
    encoder->setRenderPipelineState(easu_);
    encoder->setFragmentTexture(source, easuTexture_);
    encoder->setFragmentSamplerState(
        metal_->sampler(MetalContext::Filter::Screen, MetalContext::Address::ClampToEdge),
        easuSampler_);
    encoder->setFragmentBytes(&push, sizeof(push), easuPush_);
    encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3));
    encoder->endEncoding();
}

void MetalPostProcess::encodeFxaa(MTL::CommandBuffer* commandBuffer, MTL::Texture* source,
                                  MTL::Texture* target, uint32_t w, uint32_t h,
                                  float intoxication) {
    if (!fxaa_ || !commandBuffer || !source || !target || !w || !h) return;
    const FxaaPush push{
        {1.0f / static_cast<float>(w), 1.0f / static_cast<float>(h)},
        0.0f,
        intoxication,
    };
    MTL::RenderCommandEncoder* encoder = beginFullScreenPass(commandBuffer, target, "FXAA");
    encoder->setRenderPipelineState(fxaa_);
    encoder->setFragmentTexture(source, fxaaTexture_);
    encoder->setFragmentSamplerState(
        metal_->sampler(MetalContext::Filter::Screen, MetalContext::Address::ClampToEdge),
        fxaaSampler_);
    encoder->setFragmentBytes(&push, sizeof(push), fxaaPush_);
    encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3));
    encoder->endEncoding();
}

}  // namespace rendering
}  // namespace wowee
