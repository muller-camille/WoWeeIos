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
class VkContext;
class MetalContext;

class MountDust {
public:
    MountDust();
    ~MountDust();

    [[nodiscard]] bool initialize(VkContext* ctx, VkDescriptorSetLayout perFrameLayout);
    void shutdown();
    void recreatePipelines();
    /// The pipeline state both initialize() and
    /// recreatePipelines() need, described once.
    void buildPipelines(VkDevice device,
                        const VkPipelineShaderStageCreateInfo& vertStage,
                        const VkPipelineShaderStageCreateInfo& fragStage);

    // Spawn dust particles at mount feet when moving on ground
    void spawnDust(const glm::vec3& position, const glm::vec3& velocity, bool isMoving);

    void update(float deltaTime);
    void render(VkCommandBuffer cmd, VkDescriptorSet perFrameSet);

private:
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        float lifetime;
        float maxLifetime;
        float size;
        float alpha;
    };

    static constexpr int MAX_DUST_PARTICLES = 300;
    std::vector<Particle> particles;

    // Vulkan objects
    VkContext* vkCtx = nullptr;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    // Dynamic mapped buffer for particle vertex data (updated every frame)
    ::VkBuffer dynamicVB = VK_NULL_HANDLE;
    VmaAllocation dynamicVBAlloc = VK_NULL_HANDLE;
    VmaAllocationInfo dynamicVBAllocInfo{};
    VkDeviceSize dynamicVBSize = 0;

    std::vector<float> vertexData;
    float spawnAccum = 0.0f;

    /// vertexData from the particles: position, size, alpha.
    void fillVertexData();

#ifdef WOWEE_METAL
public:
    /// The same on the Metal renderer (docs/plan-metal.md).
    bool initializeMetal(MetalContext* ctx, uint32_t colorFormat, uint32_t depthFormat,
                         uint32_t sampleCount);
    void renderMetal(MTL::RenderCommandEncoder* encoder, MTL::Buffer* perFrame, size_t offset);

private:
    MetalContext* metal_ = nullptr;
    MTL::RenderPipelineState* mtlPipeline_ = nullptr;
    MetalVertexRing mtlVertices_;
    int mtlVertPerFrame_ = -1;
#endif
};

} // namespace rendering
} // namespace wowee
