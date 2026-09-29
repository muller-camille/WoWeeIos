#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <cstdint>

#ifdef WOWEE_METAL
namespace MTL {
class CommandBuffer;
class ComputePipelineState;
class RenderPipelineState;
class Texture;
}  // namespace MTL
#endif

namespace wowee {
namespace rendering {

class VkContext;
class MetalContext;

/**
 * Crepuscular rays drawn over the finished picture: the bright sky around the
 * sun smeared outward from the sun's place on screen, so every gap in a canopy
 * or between two roofs pours a streak of light toward the camera.
 *
 * It complements the volumetric fog rather than repeating it. The fog's shafts
 * are real light in real air and show only where there is air enough to see
 * them; these are what the eye does looking into the sun, and show whenever it
 * is on screen or just off it.
 *
 * Built from the frame itself rather than its depth. Depth lives in a different
 * image on every path this renderer has - the swapchain's, the multisampled
 * resolve, FSR's or FXAA's own targets, and with MSAA and no depth resolve in
 * nothing a shader can read - while the finished frame is always on the
 * swapchain, in one layout, before the interface goes over it. Bright pixels
 * near the sun are what the rays come from; a tree against the sky is dark and
 * casts none, which is the whole effect.
 *
 * Each frame: the swapchain image is blitted to a quarter-size copy, a compute
 * pass marches every pixel of that copy toward the sun gathering what it
 * passes, and the result is added into the overlay pass ahead of the UI.
 * Double-buffered per frame in flight.
 */
class SunShafts {
public:
    /// What one frame's rays come from.
    struct FrameInputs {
        /// The sun's place on screen, framebuffer uv. May be off the edge.
        glm::vec2 sunUV{0.5f};
        /// The sun's colour, which the rays take on.
        glm::vec3 tint{1.0f};
        /// 0 draws nothing this frame.
        float strength = 0.0f;
    };

    SunShafts() = default;
    ~SunShafts();
    SunShafts(const SunShafts&) = delete;
    SunShafts& operator=(const SunShafts&) = delete;

    [[nodiscard]] bool initialize(VkContext* ctx);
    void shutdown();

    /// Build this frame's rays from the finished swapchain image. Outside any
    /// render pass, after the scene's last one has closed - the image is in
    /// PRESENT_SRC and goes back there. Returns whether there is anything for
    /// composite() to add.
    bool record(VkCommandBuffer cmd, uint32_t frame, VkImage swapchainImage,
                VkExtent2D extent, const FrameInputs& in);

    /// Add them to the picture. Inside the overlay pass, viewport set, before
    /// the interface is drawn. Draws nothing unless record() said there was
    /// something this frame.
    void composite(VkCommandBuffer cmd, uint32_t frame);

#ifdef WOWEE_METAL
    /// The same on the Metal renderer (docs/plan-metal.md).
    [[nodiscard]] bool initializeMetal(MetalContext* ctx);
    /// This frame's rays, marched out of picture - the finished world, under
    /// the interface - into a target a quarter of outW x outH, the screen's.
    /// The picture is read as it is rather than from a quarter-size copy: a
    /// Metal blit does not scale, and the march takes its places from the
    /// rays' own size. Returns whether compositeMetal has anything to add.
    bool recordMetal(MTL::CommandBuffer* commandBuffer, MTL::Texture* picture, uint32_t outW,
                     uint32_t outH, const FrameInputs& in);
    /// Screen this frame's rays onto target, in a pass of their own.
    void compositeMetal(MTL::CommandBuffer* commandBuffer, MTL::Texture* target);
#endif

private:
    static constexpr uint32_t MAX_FRAMES = 2;

    struct Target {
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation alloc = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };

    bool createTarget(Target& t, VkFormat format, VkImageUsageFlags usage);
    void destroyTarget(Target& t);
    bool ensureTargets(VkExtent2D extent);
    void destroyTargets();
    bool ensureCompositePipeline();

    VkContext* ctx_ = nullptr;
    bool usable_ = false;

    // Quarter the swapchain's size, per frame slot: the copy of the frame and
    // the rays marched out of it.
    Target frameCopy_[MAX_FRAMES];
    Target rays_[MAX_FRAMES];
    VkExtent2D targetExtent_{.width = 0, .height = 0};
    VkExtent2D sourceExtent_{.width = 0, .height = 0};
    bool drawThisFrame_[MAX_FRAMES] = {};

    VkSampler sampler_ = VK_NULL_HANDLE;  // owned by the context cache

    VkDescriptorSetLayout marchSetLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout marchLayout_ = VK_NULL_HANDLE;
    VkPipeline marchPipeline_ = VK_NULL_HANDLE;

    VkDescriptorSetLayout compositeSetLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout compositeLayout_ = VK_NULL_HANDLE;
    VkPipeline compositePipeline_ = VK_NULL_HANDLE;
    /// The swapchain format the composite pipeline was built for. The overlay
    /// pass is rebuilt with the swapchain, and a pipeline stays valid for any
    /// pass with the same attachment - so only a format change rebuilds it.
    VkFormat compositeFormat_ = VK_FORMAT_UNDEFINED;

    VkDescriptorPool descPool_ = VK_NULL_HANDLE;
    VkDescriptorSet marchSets_[MAX_FRAMES] = {};
    VkDescriptorSet compositeSets_[MAX_FRAMES] = {};

#ifdef WOWEE_METAL
    MetalContext* metal_ = nullptr;
    MTL::ComputePipelineState* mtlMarch_ = nullptr;
    MTL::RenderPipelineState* mtlComposite_ = nullptr;
    MTL::Texture* mtlRays_[3] = {};  // a ring of MetalContext::kRingSize
    uint32_t mtlRaysW_ = 0, mtlRaysH_ = 0;
    MTL::Texture* mtlRaysThisFrame_ = nullptr;  // what recordMetal left to composite
    int mtlMarchFrame_ = -1, mtlMarchFrameSampler_ = -1, mtlMarchRays_ = -1, mtlMarchPush_ = -1;
    int mtlCompositeRays_ = -1, mtlCompositeSampler_ = -1;
#endif
};

} // namespace rendering
} // namespace wowee
