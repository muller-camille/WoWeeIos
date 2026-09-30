#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>

#ifdef WOWEE_METAL
namespace MTL {
class Buffer;
class RenderCommandEncoder;
class RenderPipelineState;
}  // namespace MTL
#endif

namespace wowee {
namespace rendering {

class VkContext;
class MetalContext;
struct SkyParams;

/**
 * Procedural cloud renderer (Vulkan)
 *
 * Renders animated procedural clouds on a sky hemisphere using FBM noise.
 * Sun-lit edges, self-shadowing, and DBC-driven cloud colors for realistic appearance.
 *
 * Pipeline layout:
 *   set 0 = perFrameLayout  (camera UBO - view, projection, etc.)
 *   push  = CloudPush       (3 x vec4 = 48 bytes)
 */
class Clouds {
public:
    Clouds();
    ~Clouds();

    bool initialize(VkContext* ctx, VkDescriptorSetLayout perFrameLayout);
    void shutdown();
    void recreatePipelines();
    /// The pipeline both initialize() and recreatePipelines() need.
    void buildPipeline(VkDevice device,
                       const VkPipelineShaderStageCreateInfo& vertStage,
                       const VkPipelineShaderStageCreateInfo& fragStage);

    /**
     * Render clouds using DBC-driven colors and sun lighting.
     * @param cmd         Command buffer to record into
     * @param perFrameSet Per-frame descriptor set (set 0, camera UBO)
     * @param params      Sky parameters with DBC colors and sun direction
     */
    void render(VkCommandBuffer cmd, VkDescriptorSet perFrameSet, const SkyParams& params);

    /**
     * Update cloud animation (wind drift).
     */
    void update(float deltaTime);

    // --- Enable / disable ---
    void setEnabled(bool enabled) { enabled_ = enabled; }
    [[nodiscard]] bool isEnabled() const { return enabled_; }

    // --- Cloud parameters ---
    void setDensity(float density);
    [[nodiscard]] float getDensity() const { return density_; }

    void setWindSpeed(float speed) { windSpeed_ = speed; }
    [[nodiscard]] float getWindSpeed() const { return windSpeed_; }

private:
    // Push constant block - must match clouds.frag.glsl
    struct CloudPush {
        glm::vec4 cloudColor;     // xyz = DBC-derived base cloud color, w = unused
        glm::vec4 sunDirDensity;  // xyz = sun direction, w = density
        glm::vec4 windAndLight;   // x = windOffset, y = sunIntensity, z = ambient, w = unused
    };
    static_assert(sizeof(CloudPush) == 48, "CloudPush size mismatch");
    /// This frame's constants, the same for both backends.
    [[nodiscard]] CloudPush makePush(const SkyParams& params) const;

    void generateMesh();
    void createBuffers();
    void destroyBuffers();

    // Vulkan objects
    VkContext*       vkCtx_          = nullptr;
    VkPipeline       pipeline_       = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkBuffer         vertexBuffer_   = VK_NULL_HANDLE;
    VmaAllocation    vertexAlloc_    = VK_NULL_HANDLE;
    VkBuffer         indexBuffer_    = VK_NULL_HANDLE;
    VmaAllocation    indexAlloc_     = VK_NULL_HANDLE;

    // Mesh data (CPU side, used during initialization only)
    std::vector<glm::vec3>   vertices_;
    std::vector<uint32_t>    indices_;
    int                      indexCount_ = 0;

    // Cloud parameters
    bool  enabled_    = true;
    float density_    = 0.35f;
    float windSpeed_  = 1.0f;
    float windOffset_ = 0.0f;

    // Mesh generation parameters
    static constexpr int   SEGMENTS = 32;
    static constexpr int   RINGS    = 8;
    static constexpr float RADIUS   = 900.0f;

#ifdef WOWEE_METAL
public:
    /// The same on the Metal renderer (docs/plan-metal.md).
    bool initializeMetal(MetalContext* ctx, uint32_t colorFormat, uint32_t depthFormat,
                         uint32_t sampleCount);
    void renderMetal(MTL::RenderCommandEncoder* encoder, MTL::Buffer* perFrame, size_t offset,
                     const SkyParams& params);

private:
    void releaseMetal();
    MetalContext* metal_ = nullptr;
    MTL::RenderPipelineState* mtlPipeline_ = nullptr;
    MTL::Buffer* mtlVertices_ = nullptr;
    MTL::Buffer* mtlIndices_ = nullptr;
    int mtlVertPerFrame_ = -1, mtlFragPerFrame_ = -1, mtlFragPush_ = -1;
    int mtlFragFog_ = -1, mtlFragFogSampler_ = -1;
#endif
};

} // namespace rendering
} // namespace wowee
