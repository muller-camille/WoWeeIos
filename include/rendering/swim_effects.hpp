#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>
#ifdef WOWEE_METAL
#include "rendering/metal/metal_pipeline.hpp"
#endif

namespace wowee {
namespace rendering {

class Camera;
class CameraController;
class WaterRenderer;
class M2Renderer;
class VkContext;
class MetalContext;

class SwimEffects {
public:
    SwimEffects();
    ~SwimEffects();

    [[nodiscard]] bool initialize(VkContext* ctx, VkDescriptorSetLayout perFrameLayout);
    void shutdown();
    void recreatePipelines();
    /// The three pipelines both initialize() and recreatePipelines() need.
    void buildRipplePipeline(
        VkDevice device, const VkPipelineShaderStageCreateInfo& vertStage,
        const VkPipelineShaderStageCreateInfo& fragStage,
        const VkVertexInputBindingDescription& binding,
        const std::vector<VkVertexInputAttributeDescription>& attrs);
    void buildBubblePipeline(
        VkDevice device, const VkPipelineShaderStageCreateInfo& vertStage,
        const VkPipelineShaderStageCreateInfo& fragStage,
        const VkVertexInputBindingDescription& binding,
        const std::vector<VkVertexInputAttributeDescription>& attrs);
    void buildInsectPipeline(
        VkDevice device, const VkPipelineShaderStageCreateInfo& vertStage,
        const VkPipelineShaderStageCreateInfo& fragStage,
        const VkVertexInputBindingDescription& binding,
        const std::vector<VkVertexInputAttributeDescription>& attrs);

    /// Select the render pass these particles will be recorded into. Water now
    /// draws after them in a pass of its own, so the spray has to move into that
    /// pass too or the water sheet is painted straight over it. Call before
    /// initialize() or recreatePipelines().
    void setTargetPass(VkRenderPass pass, VkSampleCountFlagBits samples) {
        targetPass_ = pass;
        targetSamples_ = samples;
    }
    void update(const Camera& camera, const CameraController& cc,
                const WaterRenderer& water, float deltaTime);
    void render(VkCommandBuffer cmd, VkDescriptorSet perFrameSet);
    void spawnFootSplash(const glm::vec3& footPos, float waterH);
    void setM2Renderer(M2Renderer* renderer) { m2Renderer = renderer; }

private:
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        float lifetime;
        float maxLifetime;
        float size;
        float alpha;
    };

    struct InsectParticle {
        glm::vec3 position;
        glm::vec3 orbitCenter;  // vegetation position to orbit around
        float lifetime;
        float maxLifetime;
        float size;
        float alpha;
        float phase;       // random phase offset for erratic motion
        float orbitRadius;
        float orbitSpeed;
        float heightOffset; // height above plant
    };

    static constexpr int MAX_RIPPLE_PARTICLES = 200;
    static constexpr int MAX_BUBBLE_PARTICLES = 150;
    static constexpr int MAX_INSECT_PARTICLES = 50;

    std::vector<Particle> ripples;
    std::vector<Particle> bubbles;
    std::vector<InsectParticle> insects;

    // Vulkan objects
    VkContext* vkCtx = nullptr;
    M2Renderer* m2Renderer = nullptr;

    // Ripple pipeline + dynamic buffer
    VkPipeline ripplePipeline = VK_NULL_HANDLE;
    VkPipelineLayout ripplePipelineLayout = VK_NULL_HANDLE;
    ::VkBuffer rippleDynamicVB = VK_NULL_HANDLE;
    VmaAllocation rippleDynamicVBAlloc = VK_NULL_HANDLE;
    VmaAllocationInfo rippleDynamicVBAllocInfo{};
    VkDeviceSize rippleDynamicVBSize = 0;

    // Bubble pipeline + dynamic buffer
    VkPipeline bubblePipeline = VK_NULL_HANDLE;
    VkPipelineLayout bubblePipelineLayout = VK_NULL_HANDLE;
    ::VkBuffer bubbleDynamicVB = VK_NULL_HANDLE;
    VmaAllocation bubbleDynamicVBAlloc = VK_NULL_HANDLE;
    VmaAllocationInfo bubbleDynamicVBAllocInfo{};
    VkDeviceSize bubbleDynamicVBSize = 0;

    // Insect pipeline + dynamic buffer
    VkPipeline insectPipeline = VK_NULL_HANDLE;
    VkPipelineLayout insectPipelineLayout = VK_NULL_HANDLE;
    ::VkBuffer insectDynamicVB = VK_NULL_HANDLE;
    VmaAllocation insectDynamicVBAlloc = VK_NULL_HANDLE;
    VmaAllocationInfo insectDynamicVBAllocInfo{};
    VkDeviceSize insectDynamicVBSize = 0;

    std::vector<float> rippleVertexData;
    std::vector<float> bubbleVertexData;
    std::vector<float> insectVertexData;

    VkRenderPass targetPass_ = VK_NULL_HANDLE;
    VkSampleCountFlagBits targetSamples_ = VK_SAMPLE_COUNT_1_BIT;

    float wadeSpawnAccum = 0.0f;
    float rippleSpawnAccum = 0.0f;
    float bubbleSpawnAccum = 0.0f;
    float insectSpawnAccum = 0.0f;

    void spawnRipple(const glm::vec3& pos, const glm::vec3& moveDir, float waterH);
    void spawnBubble(const glm::vec3& pos, float waterH);
    void spawnInsect(const glm::vec3& vegPos);

#ifdef WOWEE_METAL
public:
    /// The same on the Metal renderer (docs/plan-metal.md): drawn after the
    /// water, over the surface the spray is thrown off, as Vulkan draws it in
    /// the water's continuation pass.
    bool initializeMetal(MetalContext* ctx, uint32_t colorFormat, uint32_t depthFormat,
                         uint32_t sampleCount);
    void renderMetal(MTL::RenderCommandEncoder* encoder, MTL::Buffer* perFrame, size_t offset);

private:
    MetalContext* metal_ = nullptr;
    MTL::RenderPipelineState* mtlRipplePipeline_ = nullptr;
    MTL::RenderPipelineState* mtlBubblePipeline_ = nullptr;
    MTL::RenderPipelineState* mtlInsectPipeline_ = nullptr;
    MetalVertexRing mtlRippleVerts_;
    MetalVertexRing mtlBubbleVerts_;
    MetalVertexRing mtlInsectVerts_;
    int mtlRipplePerFrame_ = -1, mtlBubblePerFrame_ = -1;
#endif
};

} // namespace rendering
} // namespace wowee
