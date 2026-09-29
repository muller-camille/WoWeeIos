#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace MTL {
class Buffer;
class ComputePipelineState;
class RenderCommandEncoder;
class RenderPipelineState;
class VertexDescriptor;
}  // namespace MTL

namespace wowee {
namespace rendering {

class MetalContext;
struct VertexAttribute;

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
    Multiply,  // dst * (1 + src), the brightness overlay's; alpha kept
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

/// A compute pipeline from default.metallib's kernel of that name. Null, and
/// logged, when it is missing or Metal refuses it. The caller releases it.
[[nodiscard]] MTL::ComputePipelineState* buildMetalComputePipeline(MetalContext& ctx,
                                                                   const char* kernel);

/// Packed floats in one buffer at kMetalVertexBufferIndex: a vec3 position at
/// location 0, then one float at each location from 1 - the particle
/// effects' layout (position, size, alpha and the like). The caller releases
/// it.
[[nodiscard]] MTL::VertexDescriptor* newPackedFloatVertexDescriptor(uint32_t extraFloats);

/// Vertices the CPU writes every frame and the GPU draws: a ring of
/// MetalContext::kRingSize shared buffers, one per frame that can be in
/// flight, so writing this frame's never touches what an earlier one reads.
class MetalVertexRing {
public:
    MetalVertexRing() = default;
    ~MetalVertexRing() { release(); }
    MetalVertexRing(const MetalVertexRing&) = delete;
    MetalVertexRing& operator=(const MetalVertexRing&) = delete;

    [[nodiscard]] bool create(MetalContext& ctx, size_t capacityBytes);
    void release();
    /// This frame's buffer, bytes of data at its start - no more than the
    /// capacity; how many were written goes to written. Null before create.
    MTL::Buffer* write(const MetalContext& ctx, const void* data, size_t bytes,
                       size_t* written = nullptr);
    [[nodiscard]] bool valid() const { return buffers_[0] != nullptr; }

private:
    MTL::Buffer* buffers_[3] = {};
    size_t capacity_ = 0;
};

/// The shadow map's format: depth alone, 32-bit float.
inline constexpr uint32_t kMetalShadowDepthFormat = 252;  // MTL::PixelFormatDepth32Float

/// Where shadow.vert and shadow.frag take their bindings: the ShadowPush
/// block, and the texture the foliage pass alpha-tests.
struct MetalShadowSlots {
    int vertPush = -1, fragPush = -1, fragTexture = -1, fragSampler = -1;
    /// From the manifest; false when it does not describe the shaders.
    bool load();
};

/// The depth-only pipeline shadow.vert and shadow.frag draw casters with, for
/// geometry laid out as attrs: only locations 0 (position) and 1 (texture
/// coordinate) are read. The caller releases it.
[[nodiscard]] MTL::RenderPipelineState* buildMetalShadowPipeline(
    MetalContext& ctx, const VertexAttribute* attrs, size_t count, uint32_t stride,
    const char* label);

/// What every caster's pass sets the same way: the pipeline, depth tested
/// and written, the Vulkan pipelines' depth bias, no culling, and a white
/// texture for the alpha test to pass on.
void beginMetalShadowDraws(MetalContext& ctx, MTL::RenderCommandEncoder* encoder,
                           MTL::RenderPipelineState* pipeline, const MetalShadowSlots& slots);

}  // namespace rendering
}  // namespace wowee
