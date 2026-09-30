#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

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

/**
 * Celestial body renderer (Vulkan)
 *
 * Renders sun and moon that move across the sky based on time of day.
 * Sun rises at dawn, sets at dusk. Moon is visible at night.
 *
 * Pipeline layout:
 *   set 0  = perFrameLayout  (camera UBO - view, projection, etc.)
 *   push   = CelestialPush   (mat4 model + vec4 celestialColor + float intensity
 *                              + float moonPhase + float animTime = 96 bytes)
 */
class Celestial {
public:
    Celestial();
    ~Celestial();

    /**
     * Initialize the renderer.
     * @param ctx           Vulkan context
     * @param perFrameLayout Descriptor set layout for set 0 (camera UBO)
     */
    bool initialize(VkContext* ctx, VkDescriptorSetLayout perFrameLayout);
    void shutdown();
    void recreatePipelines();

private:
    /// The one pipeline description, so the first build and every rebuild
    /// after it cannot disagree. They did: initialize() asked for no depth
    /// test and recreatePipelines() asked for one, so the sun and moon changed
    /// behaviour the first time the swapchain was rebuilt.
    VkPipeline buildPipeline(VkDevice device,
                             const VkPipelineShaderStageCreateInfo& vertStage,
                             const VkPipelineShaderStageCreateInfo& fragStage);

public:

    /**
     * Render celestial bodies (sun and moons).
     * @param cmd         Command buffer to record into
     * @param perFrameSet Per-frame descriptor set (set 0, camera UBO)
     * @param timeOfDay   Time of day in hours (0-24)
     * @param sunDir      Optional sun direction from lighting system (normalized)
     * @param sunColor    Optional sun colour from lighting system
     * @param gameTime    Optional server game time in seconds (deterministic moon phases)
     */
    void render(VkCommandBuffer cmd, VkDescriptorSet perFrameSet,
                float timeOfDay,
                const glm::vec3* sunDir   = nullptr,
                const glm::vec3* sunColor = nullptr,
                float gameTime = -1.0f,
                float nightFactor = 1.0f);

    /**
     * Update celestial bodies (moon phase cycling, haze timer).
     */
    void update(float deltaTime);

    // --- Enable / disable ---
    void setEnabled(bool enabled) { renderingEnabled_ = enabled; }
    [[nodiscard]] bool isEnabled() const { return renderingEnabled_; }

    // --- Moon phases ---
    [[nodiscard]] float getMoonPhase() const { return whiteLadyPhase_; }

    /** Set Blue Child phase (secondary moon, 0 = new, 0.5 = full, 1 = new). */
    void setBlueChildPhase(float phase);
    [[nodiscard]] float getBlueChildPhase() const { return blueChildPhase_; }

    void setMoonPhaseCycling(bool enabled) { moonPhaseCycling_ = enabled; }
    [[nodiscard]] bool isMoonPhaseCycling() const { return moonPhaseCycling_; }

    /** Enable / disable two-moon rendering (White Lady + Blue Child). */
    void setDualMoonMode(bool enabled) { dualMoonMode_ = enabled; }
    [[nodiscard]] bool isDualMoonMode() const { return dualMoonMode_; }

    // --- Positional / colour queries (unchanged from GL version) ---
    [[nodiscard]] glm::vec3 getSunPosition(float timeOfDay) const;
    [[nodiscard]] glm::vec3 getMoonPosition(float timeOfDay) const;
    [[nodiscard]] glm::vec3 getSunColor(float timeOfDay) const;
    [[nodiscard]] float     getSunIntensity(float timeOfDay) const;

private:
    // Push constant block - MUST match celestial.vert.glsl / celestial.frag.glsl
    struct CelestialPush {
        glm::mat4 model;         // 64 bytes
        glm::vec4 celestialColor; // 16 bytes (xyz = colour, w unused)
        float     intensity;     //  4 bytes
        float     moonPhase;     //  4 bytes
        float     animTime;      //  4 bytes
        float     _pad;          //  4 bytes  (round to 16-byte boundary = 96 bytes total)
    };
    static_assert(sizeof(CelestialPush) == 96, "CelestialPush size mismatch");

    void createQuad();
    void destroyQuad();

    /// Each body's push constants, false when it is not up: what both
    /// backends draw the shared quad with.
    bool sunPush(CelestialPush& push, float timeOfDay,
                 const glm::vec3* sunDir, const glm::vec3* sunColor) const;
    bool moonPush(CelestialPush& push, float timeOfDay, float nightFactor) const;
    bool blueChildPush(CelestialPush& push, float timeOfDay, float nightFactor) const;
    /// The bodies to draw this frame, in order; the count.
    int bodyPushes(CelestialPush (&pushes)[3], float timeOfDay, const glm::vec3* sunDir,
                   const glm::vec3* sunColor, float gameTime, float nightFactor);

    [[nodiscard]] float calculateCelestialAngle(float timeOfDay, float riseTime, float setTime) const;
    [[nodiscard]] float computePhaseFromGameTime(float gameTime, float cycleDays) const;
    void  updatePhasesFromGameTime(float gameTime);

    // Vulkan objects
    VkContext*        vkCtx_          = nullptr;
    VkPipeline        pipeline_       = VK_NULL_HANDLE;
    VkPipelineLayout  pipelineLayout_ = VK_NULL_HANDLE;
    VkBuffer          vertexBuffer_   = VK_NULL_HANDLE;
    VmaAllocation     vertexAlloc_    = VK_NULL_HANDLE;
    VkBuffer          indexBuffer_    = VK_NULL_HANDLE;
    VmaAllocation     indexAlloc_     = VK_NULL_HANDLE;

    bool renderingEnabled_ = true;

    // Moon phase system (two moons in Azeroth lore)
    float whiteLadyPhase_ = 0.5f;   // 0-1, 0=new, 0.5=full
    float blueChildPhase_ = 0.25f;  // 0-1
    bool  moonPhaseCycling_ = true;
    float moonPhaseTimer_   = 0.0f; // Fallback deltaTime mode
    float sunHazeTimer_     = 0.0f; // Always-running haze animation timer
    bool  dualMoonMode_     = true;

    // WoW lunar cycle constants (game days; 1 game day = 24 real minutes)
    static constexpr float WHITE_LADY_CYCLE_DAYS = 30.0f;
    static constexpr float BLUE_CHILD_CYCLE_DAYS = 27.0f;
    static constexpr float MOON_CYCLE_DURATION   = 240.0f; // Fallback: 4 minutes

#ifdef WOWEE_METAL
public:
    /// The same on the Metal renderer (docs/plan-metal.md).
    bool initializeMetal(MetalContext* ctx, uint32_t colorFormat, uint32_t depthFormat,
                         uint32_t sampleCount);
    void renderMetal(MTL::RenderCommandEncoder* encoder, MTL::Buffer* perFrame, size_t offset,
                     float timeOfDay, const glm::vec3* sunDir, const glm::vec3* sunColor,
                     float gameTime, float nightFactor);

private:
    void releaseMetal();
    MetalContext* metal_ = nullptr;
    MTL::RenderPipelineState* mtlPipeline_ = nullptr;
    MTL::Buffer* mtlVertices_ = nullptr;
    MTL::Buffer* mtlIndices_ = nullptr;
    int mtlVertPerFrame_ = -1, mtlVertPush_ = -1, mtlFragPush_ = -1;
#endif
};

} // namespace rendering
} // namespace wowee
