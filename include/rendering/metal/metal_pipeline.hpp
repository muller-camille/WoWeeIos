#pragma once

#include <cstdint>
#include <string>

namespace MTL {
class RenderPipelineState;
class VertexDescriptor;
}  // namespace MTL

namespace wowee {
namespace rendering {

class MetalContext;

/// Where the vertex buffers go: from index 30 down, clear of the shaders' own
/// buffers, of which no shader uses more than five (docs/plan-metal.md, 3.5).
inline constexpr uint32_t kMetalVertexBufferIndex = 30;

/// Which Metal buffer, texture and sampler each Vulkan (set, binding) of a
/// shader became, read from assets/shaders/metal/manifest.json - the one
/// source both the shaders and the renderers take their numbering from.
///
/// Every lookup of something the manifest does not have logs and answers -1,
/// and MetalBindings::valid() is false from then on, so a renderer finds out
/// when it builds its pipelines rather than by drawing black.
class MetalBindings {
public:
    /// The function's entry, e.g. "character_frag".
    MetalBindings(const std::string& function);

    [[nodiscard]] int buffer(uint32_t set, uint32_t binding) const;
    [[nodiscard]] int texture(uint32_t set, uint32_t binding) const;
    [[nodiscard]] int sampler(uint32_t set, uint32_t binding) const;
    /// The buffer push constants became.
    [[nodiscard]] int pushConstants() const;
    [[nodiscard]] bool valid() const { return valid_; }

private:
    [[nodiscard]] int lookup(uint32_t set, uint32_t binding, const char* key) const;

    std::string function_;
    const void* entry_ = nullptr;  // nlohmann::json*, owned by the loaded manifest
    mutable bool valid_ = true;
};

/// How a colour attachment blends, as the Vulkan pipelines' three presets do.
enum class MetalBlend : uint8_t {
    None,      // PipelineBuilder::blendDisabled
    Alpha,     // PipelineBuilder::blendAlpha
    Additive,  // PipelineBuilder::blendAdditive
};

struct MetalPipelineDesc {
    const char* vertexFunction = nullptr;
    const char* fragmentFunction = nullptr;
    MTL::VertexDescriptor* vertexDescriptor = nullptr;  // not taken over
    uint32_t colorFormat = 0;   // MTL::PixelFormat
    uint32_t depthFormat = 0;   // MTL::PixelFormat, 0 for none
    uint32_t sampleCount = 1;
    MetalBlend blend = MetalBlend::None;
    bool alphaToCoverage = false;
    const char* label = nullptr;
};

/// A render pipeline from default.metallib. Null, and logged, when a function
/// is missing or Metal refuses the combination. The caller releases it.
[[nodiscard]] MTL::RenderPipelineState* buildMetalPipeline(MetalContext& ctx,
                                                           const MetalPipelineDesc& desc);

}  // namespace rendering
}  // namespace wowee
