#include "rendering/volumetric_fog.hpp"
#include "rendering/volumetric_fog_math.hpp"
#include "rendering/vk_context.hpp"
#include "rendering/vk_shader.hpp"
#include "rendering/vk_utils.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"
#ifdef WOWEE_METAL
#include <Metal/Metal.hpp>
#include "rendering/metal/metal_context.hpp"
#include "rendering/metal/metal_pipeline.hpp"
#endif
#include <algorithm>
#include <cmath>
#include <cstring>

namespace wowee {
namespace rendering {

namespace {

constexpr VkFormat kVolumeFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

/// Must match VolumeParams in volumetric_fog_inject.comp.glsl and
/// volumetric_fog_integrate.comp.glsl (std140).
struct VolumeParamsGPU {
    glm::vec4 rayOrigin;      // xyz = camera position
    glm::vec4 rayCorner[4];   // world rays at one yard of view depth: uv (0,0), (1,0), (0,1), (1,1)
    glm::mat4 prevViewProj;   // last frame's, for reprojecting the history
    glm::vec4 medium;         // x = density, y = layer base z, z = 1 / layer height, w = floor
    glm::vec4 lighting;       // x = sun scatter, y = local light scatter, z = ambient scatter, w = unused
    glm::vec4 drift;          // xyz = how far the mist has blown, w = noise amount
    glm::vec4 jitter;         // xyz = where in its cell this frame samples, w = history weight
    glm::ivec4 dims;          // xyz = volume size
};

constexpr uint32_t kInjectGroup = 4;     // 4x4x4, volumetric_fog_inject.comp.glsl
constexpr uint32_t kIntegrateGroup = 8;  // 8x8x1, volumetric_fog_integrate.comp.glsl

/// The three volume sizes. Depth matters most - it is what resolves a shaft
/// from the air either side of it - so the smallest keeps every slice and
/// gives up screen tiles instead.
glm::uvec3 sizeForQuality(VolumetricFog::Quality q) {
    switch (q) {
        case VolumetricFog::Quality::Low:    return {96, 54, 64};
        case VolumetricFog::Quality::Medium: return {160, 90, 64};
        case VolumetricFog::Quality::High:   return {240, 135, 96};
        case VolumetricFog::Quality::Off:    break;
    }
    return {0, 0, 0};
}

float halton(uint32_t index, uint32_t base) {
    float f = 1.0f;
    float r = 0.0f;
    while (index > 0) {
        f /= static_cast<float>(base);
        r += f * static_cast<float>(index % base);
        index /= base;
    }
    return r;
}

/// The inject pass's block for one frame: where its cells lie, how the air
/// is made and lit, and how much of the history it blends in.
VolumeParamsGPU volumeParams(const VolumetricFog::FrameInputs& in, bool history,
                             const glm::mat4& prevViewProj, uint32_t frameCounter,
                             glm::uvec3 size) {
    VolumeParamsGPU p{};
    // The inject pass interpolates between these to place every cell.
    p.rayOrigin = glm::vec4(in.cameraPos, 0.0f);
    const auto rays = froxelCornerRays(in.view, in.projection);
    for (int i = 0; i < 4; i++) p.rayCorner[i] = glm::vec4(rays[i], 0.0f);
    p.prevViewProj = prevViewProj;

    p.medium = glm::vec4(in.density, in.layerBase, 1.0f / std::max(in.layerHeight, 1.0f),
                         glm::clamp(in.layerFloor, 0.0f, 1.0f));
    p.lighting = glm::vec4(in.sunScatter, in.localLightScatter, in.ambientScatter, 0.0f);
    // A slow breeze, a yard a second, so the mist drifts rather than sits.
    const float t = in.time;
    p.drift = glm::vec4(t * 0.9f, t * 0.45f, t * 0.12f, glm::clamp(in.noiseAmount, 0.0f, 1.0f));
    const uint32_t seq = (frameCounter % 16) + 1;
    p.jitter = glm::vec4(halton(seq, 2), halton(seq, 3), halton(seq, 5), history ? 0.9f : 0.0f);
    p.dims = glm::ivec4(size, 0);
    return p;
}

}  // namespace

VolumetricFog::~VolumetricFog() {
    shutdown();
}

bool VolumetricFog::initialize(VkContext* ctx) {
    if (!ctx) return false;
    ctx_ = ctx;

    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(ctx_->getPhysicalDevice(), kVolumeFormat, &props);
    const VkFormatFeatureFlags needed = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
                                        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
                                        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    if ((props.optimalTilingFeatures & needed) != needed) {
        // Both are required of every Vulkan device for this format, so this
        // is not expected - but a device that breaks the rule should cost the
        // fog, not the session. The neutral volume below is still made, since
        // every per-frame set binds it.
        LOG_WARNING("VolumetricFog: RGBA16F volumes cannot be stored to and filtered here (features ",
                    props.optimalTilingFeatures, ") - volumetric fog stays off");
        formatSupported_ = false;
    }

    VkSamplerCreateInfo sampCI{};
    sampCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampCI.magFilter = VK_FILTER_LINEAR;
    sampCI.minFilter = VK_FILTER_LINEAR;
    sampCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampCI.maxLod = 0.0f;
    sampler_ = ctx_->getOrCreateSampler(sampCI);

    sampCI.magFilter = VK_FILTER_NEAREST;
    sampCI.minFilter = VK_FILTER_NEAREST;
    depthSampler_ = ctx_->getOrCreateSampler(sampCI);
    if (sampler_ == VK_NULL_HANDLE || depthSampler_ == VK_NULL_HANDLE) {
        LOG_ERROR("VolumetricFog: failed to create samplers");
        return false;
    }

    if (!createVolume(neutral_, {1, 1, 1}, VK_IMAGE_USAGE_SAMPLED_BIT)) {
        LOG_ERROR("VolumetricFog: failed to create the neutral volume");
        return false;
    }
    return true;
}

bool VolumetricFog::createVolume(Volume& v, glm::uvec3 size, VkImageUsageFlags usage) {
    VkDevice device = ctx_->getDevice();

    VkImageCreateInfo imgCI{};
    imgCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgCI.imageType = VK_IMAGE_TYPE_3D;
    imgCI.format = kVolumeFormat;
    imgCI.extent = {.width = size.x, .height = size.y, .depth = size.z};
    imgCI.mipLevels = 1;
    imgCI.arrayLayers = 1;
    imgCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imgCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgCI.usage = usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imgCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    if (vmaCreateImage(ctx_->getAllocator(), &imgCI, &allocCI, &v.image, &v.alloc, nullptr) != VK_SUCCESS) {
        return false;
    }

    VkImageViewCreateInfo viewCI{};
    viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCI.image = v.image;
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_3D;
    viewCI.format = kVolumeFormat;
    viewCI.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1};
    if (vkCreateImageView(device, &viewCI, nullptr, &v.view) != VK_SUCCESS) {
        destroyVolume(v);
        return false;
    }

    // Straight to GENERAL, where every volume lives for good: written as a
    // storage image and sampled from the same layout, so no frame has to
    // move one between the two. Cleared to clear air - nothing scattered,
    // everything behind showing through - so a volume read before its first
    // frame has been built changes nothing.
    ctx_->immediateSubmit([&](VkCommandBuffer cmd) {
        VkImageMemoryBarrier2 toGeneral{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        toGeneral.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        toGeneral.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        toGeneral.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        toGeneral.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toGeneral.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        toGeneral.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toGeneral.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toGeneral.image = v.image;
        toGeneral.subresourceRange = viewCI.subresourceRange;
        VkDependencyInfo dep{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dep.imageMemoryBarrierCount = 1;
        dep.pImageMemoryBarriers = &toGeneral;
        cmdPipelineBarrier2(cmd, dep);

        VkClearColorValue clear{};
        clear.float32[3] = 1.0f;
        vkCmdClearColorImage(cmd, v.image, VK_IMAGE_LAYOUT_GENERAL, &clear, 1, &viewCI.subresourceRange);

        VkImageMemoryBarrier2 toRead = toGeneral;
        toRead.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        toRead.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        toRead.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
                              VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                              VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        toRead.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
        toRead.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        dep.pImageMemoryBarriers = &toRead;
        cmdPipelineBarrier2(cmd, dep);
    });
    return true;
}

void VolumetricFog::destroyVolume(Volume& v) {
    if (!ctx_) return;
    if (v.view) { vkDestroyImageView(ctx_->getDevice(), v.view, nullptr); v.view = VK_NULL_HANDLE; }
    if (v.image) { vmaDestroyImage(ctx_->getAllocator(), v.image, v.alloc); v.image = VK_NULL_HANDLE; v.alloc = VK_NULL_HANDLE; }
}

bool VolumetricFog::createPipelines(VkDescriptorSetLayout perFrameLayout,
                                    const VkImageView shadowViews[2]) {
    if (!ctx_ || perFrameLayout == VK_NULL_HANDLE) return false;
    VkDevice device = ctx_->getDevice();
    for (uint32_t i = 0; i < MAX_FRAMES; i++) shadowViews_[i] = shadowViews[i];

    // Set 1, shared by both passes; each reads the bindings it names.
    //   0  VolumeParams
    //   1  this slot's shadow map, as plain depth
    //   2  this slot's cells, written by inject
    //   3  the other slot's cells, last frame's, read by inject as history
    //   4  this slot's cells again, read by integrate
    //   5  this slot's running sum, written by integrate
    VkDescriptorSetLayoutBinding b[6]{};
    const VkDescriptorType types[6] = {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    };
    for (uint32_t i = 0; i < 6; i++) {
        b[i].binding = i;
        b[i].descriptorType = types[i];
        b[i].descriptorCount = 1;
        b[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    VkDescriptorSetLayoutCreateInfo layoutCI{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutCI.bindingCount = 6;
    layoutCI.pBindings = b;
    if (vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &setLayout_) != VK_SUCCESS) {
        LOG_ERROR("VolumetricFog: failed to create descriptor set layout");
        return false;
    }

    VkDescriptorSetLayout setLayouts[2] = {perFrameLayout, setLayout_};
    VkPipelineLayoutCreateInfo plCI{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plCI.setLayoutCount = 2;
    plCI.pSetLayouts = setLayouts;
    if (vkCreatePipelineLayout(device, &plCI, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        LOG_ERROR("VolumetricFog: failed to create pipeline layout");
        return false;
    }

    const auto makePipeline = [&](const char* path, VkPipeline& out) {
        VkShaderModule module;
        if (!module.loadFromFile(device, path)) {
            LOG_ERROR("VolumetricFog: failed to load ", path);
            return false;
        }
        VkComputePipelineCreateInfo cpCI{.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        cpCI.stage = module.stageInfo(VK_SHADER_STAGE_COMPUTE_BIT);
        cpCI.layout = pipelineLayout_;
        const bool ok = vkCreateComputePipelines(device, ctx_->getPipelineCache(), 1, &cpCI,
                                                 nullptr, &out) == VK_SUCCESS;
        module.destroy();
        if (!ok) LOG_ERROR("VolumetricFog: failed to create the pipeline for ", path);
        return ok;
    };
    if (!makePipeline("assets/shaders/volumetric_fog_inject.comp.spv", injectPipeline_) ||
        !makePipeline("assets/shaders/volumetric_fog_integrate.comp.spv", integratePipeline_)) {
        return false;
    }

    VkDescriptorPoolSize poolSizes[3] = {
        {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = MAX_FRAMES},
        {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = MAX_FRAMES * 3},
        {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = MAX_FRAMES * 2},
    };
    VkDescriptorPoolCreateInfo poolCI{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolCI.maxSets = MAX_FRAMES;
    poolCI.poolSizeCount = 3;
    poolCI.pPoolSizes = poolSizes;
    if (vkCreateDescriptorPool(device, &poolCI, nullptr, &descPool_) != VK_SUCCESS) {
        LOG_ERROR("VolumetricFog: failed to create descriptor pool");
        return false;
    }
    VkDescriptorSetLayout allocLayouts[MAX_FRAMES] = {setLayout_, setLayout_};
    VkDescriptorSetAllocateInfo setAlloc{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    setAlloc.descriptorPool = descPool_;
    setAlloc.descriptorSetCount = MAX_FRAMES;
    setAlloc.pSetLayouts = allocLayouts;
    if (vkAllocateDescriptorSets(device, &setAlloc, computeSets_) != VK_SUCCESS) {
        LOG_ERROR("VolumetricFog: failed to allocate descriptor sets");
        return false;
    }

    for (uint32_t i = 0; i < MAX_FRAMES; i++) {
        VkBufferCreateInfo bufCI{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufCI.size = sizeof(VolumeParamsGPU);
        bufCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VmaAllocationCreateInfo allocCI{};
        allocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo mapInfo{};
        if (vmaCreateBuffer(ctx_->getAllocator(), &bufCI, &allocCI, &paramsUBO_[i],
                            &paramsAlloc_[i], &mapInfo) != VK_SUCCESS) {
            LOG_ERROR("VolumetricFog: failed to create parameter buffer ", i);
            return false;
        }
        paramsMapped_[i] = mapInfo.pMappedData;
    }
    pipelinesReady_ = true;
    return true;
}

bool VolumetricFog::createVolumes(glm::uvec3 size) {
    constexpr VkImageUsageFlags usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    for (uint32_t i = 0; i < MAX_FRAMES; i++) {
        if (!createVolume(scatter_[i], size, usage) || !createVolume(integrated_[i], size, usage)) {
            destroyVolumes();
            return false;
        }
        setObjectName(ctx_->getDevice(), VK_OBJECT_TYPE_IMAGE,
                      reinterpret_cast<uint64_t>(scatter_[i].image), "volumetric fog cells");
        setObjectName(ctx_->getDevice(), VK_OBJECT_TYPE_IMAGE,
                      reinterpret_cast<uint64_t>(integrated_[i].image), "volumetric fog integrated");
    }
    size_ = size;
    return true;
}

void VolumetricFog::destroyVolumes() {
    for (uint32_t i = 0; i < MAX_FRAMES; i++) {
        destroyVolume(scatter_[i]);
        destroyVolume(integrated_[i]);
    }
    size_ = glm::uvec3(0);
    volumesReady_ = false;
}

void VolumetricFog::writeComputeSets() {
    for (uint32_t f = 0; f < MAX_FRAMES; f++) {
        const uint32_t other = (f + 1) % MAX_FRAMES;
        VkDescriptorBufferInfo params{.buffer = paramsUBO_[f], .offset = 0, .range = sizeof(VolumeParamsGPU)};
        VkDescriptorImageInfo shadow{.sampler = depthSampler_, .imageView = shadowViews_[f],
                                     .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorImageInfo cellsOut{.sampler = VK_NULL_HANDLE, .imageView = scatter_[f].view,
                                       .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
        VkDescriptorImageInfo history{.sampler = sampler_, .imageView = scatter_[other].view,
                                      .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
        VkDescriptorImageInfo cellsIn{.sampler = sampler_, .imageView = scatter_[f].view,
                                      .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
        VkDescriptorImageInfo sumOut{.sampler = VK_NULL_HANDLE, .imageView = integrated_[f].view,
                                     .imageLayout = VK_IMAGE_LAYOUT_GENERAL};

        VkWriteDescriptorSet w[6]{};
        for (uint32_t i = 0; i < 6; i++) {
            w[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w[i].dstSet = computeSets_[f];
            w[i].dstBinding = i;
            w[i].descriptorCount = 1;
        }
        w[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        w[0].pBufferInfo = &params;
        w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w[1].pImageInfo = &shadow;
        w[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        w[2].pImageInfo = &cellsOut;
        w[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w[3].pImageInfo = &history;
        w[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w[4].pImageInfo = &cellsIn;
        w[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        w[5].pImageInfo = &sumOut;
        vkUpdateDescriptorSets(ctx_->getDevice(), 6, w, 0, nullptr);
    }
}

bool VolumetricFog::applyPendingQuality() {
#ifdef WOWEE_METAL
    if (metal_ && pendingQuality_ != builtQuality_) {
        // No wait: a frame still in flight holds the volumes it reads.
        const Quality wanted = pendingQuality_;
        builtQuality_ = wanted;
        const bool wasOn = volumesReady_;
        destroyVolumesMetal();
        if (wanted == Quality::Off) {
            LOG_INFO("VolumetricFog (Metal): off");
            return wasOn;
        }
        if (!pipelinesReady_) {
            LOG_WARNING("VolumetricFog (Metal): its kernels did not build - staying off");
            return wasOn;
        }
        const glm::uvec3 size = sizeForQuality(wanted);
        if (!createVolumesMetal(size)) {
            LOG_WARNING("VolumetricFog (Metal): could not allocate ", size.x, "x", size.y, "x",
                        size.z, " volumes - staying off");
            return wasOn;
        }
        volumesReady_ = true;
        historyValid_ = false;
        lastRecordedFrame_ = -1;
        LOG_WARNING("VolumetricFog (Metal): on, ", size.x, "x", size.y, "x", size.z,
                    " cells over ", kNear, "-", kFar, " yards");
        return true;
    }
#endif
    if (!ctx_ || pendingQuality_ == builtQuality_) return false;
    const Quality wanted = pendingQuality_;
    builtQuality_ = wanted;

    // The volumes are bound in both frame slots' per-frame sets, and one of
    // those may still be in flight.
    vkDeviceWaitIdle(ctx_->getDevice());
    const bool wasOn = volumesReady_;
    destroyVolumes();

    if (wanted == Quality::Off) {
        LOG_INFO("VolumetricFog: off");
        return wasOn;
    }
    // Said at warning level, once per change, because it is the first thing
    // to know about a report of the fog looking wrong or costing too much -
    // and a warnings-only log is what arrives with one.
    if (!formatSupported_ || !pipelinesReady_) {
        LOG_WARNING("VolumetricFog: asked for quality ", static_cast<int>(wanted),
                    " but ", !formatSupported_ ? "the volume format is unsupported"
                                               : "its compute pipelines did not build",
                    " - staying off");
        return wasOn;
    }
    const glm::uvec3 size = sizeForQuality(wanted);
    if (!createVolumes(size)) {
        LOG_WARNING("VolumetricFog: could not allocate ", size.x, "x", size.y, "x", size.z,
                    " volumes - staying off");
        return wasOn;
    }
    writeComputeSets();
    volumesReady_ = true;
    historyValid_ = false;
    lastRecordedFrame_ = -1;
    LOG_WARNING("VolumetricFog: on, ", size.x, "x", size.y, "x", size.z, " cells over ",
                kNear, "-", kFar, " yards");
    return true;
}

glm::vec4 VolumetricFog::frameParams() const {
    if (!volumesReady_) return glm::vec4(0.0f);
    return {1.0f, kNear, 1.0f / std::log(kFar / kNear), static_cast<float>(size_.z)};
}

VkImageView VolumetricFog::getVolumeView(uint32_t frame) const {
    if (volumesReady_ && frame < MAX_FRAMES) return integrated_[frame].view;
    return neutral_.view;
}

void VolumetricFog::record(VkCommandBuffer cmd, uint32_t frame, VkDescriptorSet perFrameSet,
                           const FrameInputs& in) {
    ZoneScopedN("VolumetricFog::record");
    if (!volumesReady_ || frame >= MAX_FRAMES || cmd == VK_NULL_HANDLE) return;

    // Last frame's cells only when they are last frame's and of this place.
    const bool history = historyUsable(frame, in.cameraPos);
    const VolumeParamsGPU p = volumeParams(in, history, prevViewProj_, frameCounter_, size_);
    std::memcpy(paramsMapped_[frame], &p, sizeof(p));

    // Everything that last touched these volumes: last frame's inject and
    // integrate, which wrote the history this frame reads, and the shaders of
    // the frame before that, which sampled the volume this one overwrites.
    VkMemoryBarrier2 before{.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    before.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
                          VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                          VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    before.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    before.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    before.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    VkDependencyInfo dep{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dep.memoryBarrierCount = 1;
    dep.pMemoryBarriers = &before;
    cmdPipelineBarrier2(cmd, dep);

    VkDescriptorSet sets[2] = {perFrameSet, computeSets_[frame]};
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout_, 0, 2, sets, 0, nullptr);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, injectPipeline_);
    vkCmdDispatch(cmd, (size_.x + kInjectGroup - 1) / kInjectGroup,
                  (size_.y + kInjectGroup - 1) / kInjectGroup,
                  (size_.z + kInjectGroup - 1) / kInjectGroup);

    VkMemoryBarrier2 cells{.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    cells.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    cells.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    cells.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    cells.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    dep.pMemoryBarriers = &cells;
    cmdPipelineBarrier2(cmd, dep);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, integratePipeline_);
    vkCmdDispatch(cmd, (size_.x + kIntegrateGroup - 1) / kIntegrateGroup,
                  (size_.y + kIntegrateGroup - 1) / kIntegrateGroup, 1);

    // The scene pass reads it from both stages: particles and ribbons take
    // their fog per vertex.
    VkMemoryBarrier2 after{.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    after.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    after.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    after.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    after.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    dep.pMemoryBarriers = &after;
    cmdPipelineBarrier2(cmd, dep);

    recorded(frame, in);
}

bool VolumetricFog::historyUsable(uint32_t frame, const glm::vec3& cameraPos) const {
    // The other slot has to be the one built just before, and the camera
    // cannot have jumped. A teleport reprojects into nonsense.
    const uint32_t other = (frame + 1) % MAX_FRAMES;
    return historyValid_ && lastRecordedFrame_ == static_cast<int>(other) &&
           glm::length(cameraPos - prevCameraPos_) < 30.0f;
}

void VolumetricFog::recorded(uint32_t frame, const FrameInputs& in) {
    prevViewProj_ = in.projection * in.view;
    prevCameraPos_ = in.cameraPos;
    historyValid_ = true;
    lastRecordedFrame_ = static_cast<int>(frame);
    frameCounter_++;
}

void VolumetricFog::shutdown() {
#ifdef WOWEE_METAL
    if (metal_) {
        destroyVolumesMetal();
        if (mtlInject_) { mtlInject_->release(); mtlInject_ = nullptr; }
        if (mtlIntegrate_) { mtlIntegrate_->release(); mtlIntegrate_ = nullptr; }
        for (auto*& buffer : mtlParams_) {
            if (buffer) { buffer->release(); buffer = nullptr; }
        }
        builtQuality_ = Quality::Off;
        pipelinesReady_ = false;
        metal_ = nullptr;
    }
#endif
    if (!ctx_) return;
    VkDevice device = ctx_->getDevice();
    vkDeviceWaitIdle(device);

    destroyVolumes();
    destroyVolume(neutral_);
    for (uint32_t i = 0; i < MAX_FRAMES; i++) {
        destroy(ctx_->getAllocator(), paramsUBO_[i], paramsAlloc_[i]);
        paramsMapped_[i] = nullptr;
        computeSets_[i] = VK_NULL_HANDLE;
    }
    destroy(device, descPool_);
    destroy(device, injectPipeline_);
    destroy(device, integratePipeline_);
    destroy(device, pipelineLayout_);
    destroy(device, setLayout_);
    // Both samplers belong to the context's cache.
    sampler_ = VK_NULL_HANDLE;
    depthSampler_ = VK_NULL_HANDLE;
    builtQuality_ = Quality::Off;
    pipelinesReady_ = false;
    ctx_ = nullptr;
}

#ifdef WOWEE_METAL
bool VolumetricFog::initializeMetal(MetalContext* ctx) {
    if (!ctx) return false;
    const MetalBindings inject("volumetric_fog_inject_comp");
    const MetalBindings integrate("volumetric_fog_integrate_comp");
    mtlInjectPerFrame_ = inject.buffer(0, 0);
    mtlInjectParams_ = inject.buffer(1, 0);
    mtlInjectShadow_ = inject.texture(1, 1);
    mtlInjectShadowSampler_ = inject.sampler(1, 1);
    mtlInjectCellsOut_ = inject.texture(1, 2);
    mtlInjectHistory_ = inject.texture(1, 3);
    mtlInjectHistorySampler_ = inject.sampler(1, 3);
    mtlIntegratePerFrame_ = integrate.buffer(0, 0);
    mtlIntegrateParams_ = integrate.buffer(1, 0);
    mtlIntegrateCells_ = integrate.texture(1, 4);
    mtlIntegrateCellsSampler_ = integrate.sampler(1, 4);
    mtlIntegrateOut_ = integrate.texture(1, 5);
    if (!inject.valid() || !integrate.valid()) return false;
    metal_ = ctx;

    mtlInject_ = buildMetalComputePipeline(*ctx, "volumetric_fog_inject_comp");
    mtlIntegrate_ = buildMetalComputePipeline(*ctx, "volumetric_fog_integrate_comp");
    for (auto*& buffer : mtlParams_) buffer = ctx->newBuffer(nullptr, sizeof(VolumeParamsGPU));
    pipelinesReady_ = mtlInject_ && mtlIntegrate_ && mtlParams_[0] && mtlParams_[1] &&
                      mtlParams_[2];
    if (!pipelinesReady_) {
        LOG_WARNING("VolumetricFog (Metal): kernels unavailable - volumetric fog stays off");
    }
    return pipelinesReady_;
}

bool VolumetricFog::createVolumesMetal(glm::uvec3 size) {
    MTL::Device* device = metal_->getDevice();
    auto* desc = MTL::TextureDescriptor::alloc()->init();
    desc->setTextureType(MTL::TextureType3D);
    desc->setPixelFormat(MTL::PixelFormatRGBA16Float);
    desc->setWidth(size.x);
    desc->setHeight(size.y);
    desc->setDepth(size.z);
    desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);
    desc->setStorageMode(MTL::StorageModePrivate);
    bool ok = true;
    for (uint32_t i = 0; i < MAX_FRAMES && ok; i++) {
        mtlScatter_[i] = device->newTexture(desc);
        mtlIntegrated_[i] = device->newTexture(desc);
        ok = mtlScatter_[i] && mtlIntegrated_[i];
    }
    desc->release();
    if (!ok) {
        destroyVolumesMetal();
        return false;
    }

    // Cleared to clear air - nothing scattered, everything through - as the
    // Vulkan volumes are: the world reads one the frame the fog comes on.
    // One slice of it, copied into every slice of every volume.
    const size_t sliceBytes = static_cast<size_t>(size.x) * size.y * 4 * sizeof(uint16_t);
    std::vector<uint16_t> clearAir(static_cast<size_t>(size.x) * size.y * 4, 0);
    for (size_t i = 3; i < clearAir.size(); i += 4) clearAir[i] = 0x3C00;  // half 1.0
    MTL::Buffer* staging = metal_->newBuffer(clearAir.data(), sliceBytes);
    if (!staging) {
        destroyVolumesMetal();
        return false;
    }
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
    MTL::CommandBuffer* cmd = metal_->getQueue()->commandBuffer();
    MTL::BlitCommandEncoder* blit = cmd->blitCommandEncoder();
    for (uint32_t i = 0; i < MAX_FRAMES; i++) {
        for (MTL::Texture* volume : {mtlScatter_[i], mtlIntegrated_[i]}) {
            for (uint32_t z = 0; z < size.z; z++) {
                blit->copyFromBuffer(staging, 0, size.x * 4 * sizeof(uint16_t), sliceBytes,
                                     MTL::Size::Make(size.x, size.y, 1), volume, 0, 0,
                                     MTL::Origin::Make(0, 0, z));
            }
        }
    }
    blit->endEncoding();
    cmd->commit();
    pool->release();
    staging->release();  // the command buffer holds it until the copies are done
    size_ = size;
    return true;
}

void VolumetricFog::destroyVolumesMetal() {
    for (uint32_t i = 0; i < MAX_FRAMES; i++) {
        if (mtlScatter_[i]) { mtlScatter_[i]->release(); mtlScatter_[i] = nullptr; }
        if (mtlIntegrated_[i]) { mtlIntegrated_[i]->release(); mtlIntegrated_[i] = nullptr; }
    }
    size_ = glm::uvec3(0);
    volumesReady_ = false;
}

void VolumetricFog::recordMetal(MTL::CommandBuffer* commandBuffer, uint64_t frame,
                                MTL::Buffer* perFrame, size_t perFrameOffset,
                                MTL::Texture* shadowMap, const FrameInputs& in) {
    if (!metal_ || !volumesReady_ || !commandBuffer || !perFrame || !shadowMap) return;
    const uint32_t slot = static_cast<uint32_t>(frame % MAX_FRAMES);
    const uint32_t other = (slot + 1) % MAX_FRAMES;
    const bool history = historyUsable(slot, in.cameraPos);
    const VolumeParamsGPU p = volumeParams(in, history, prevViewProj_, frameCounter_, size_);
    MTL::Buffer* params = mtlParams_[frame % MetalContext::kRingSize];
    std::memcpy(params->contents(), &p, sizeof(p));

    MTL::SamplerState* linear =
        metal_->sampler(MetalContext::Filter::Linear, MetalContext::Address::ClampToEdge);
    // The shadow map's depth read as it is, not compared: nearest.
    MTL::SamplerState* nearest =
        metal_->sampler(MetalContext::Filter::Nearest, MetalContext::Address::ClampToEdge);

    // Two encoders: the boundary between them is the barrier between the
    // passes, and hazard tracking the ones either side of the frame.
    MTL::ComputeCommandEncoder* compute = commandBuffer->computeCommandEncoder();
    compute->setLabel(NS::String::string("volumetric fog inject", NS::UTF8StringEncoding));
    compute->setComputePipelineState(mtlInject_);
    compute->setBuffer(perFrame, perFrameOffset, mtlInjectPerFrame_);
    compute->setBuffer(params, 0, mtlInjectParams_);
    compute->setTexture(shadowMap, mtlInjectShadow_);
    compute->setSamplerState(nearest, mtlInjectShadowSampler_);
    compute->setTexture(mtlScatter_[slot], mtlInjectCellsOut_);
    compute->setTexture(mtlScatter_[other], mtlInjectHistory_);
    compute->setSamplerState(linear, mtlInjectHistorySampler_);
    compute->dispatchThreadgroups(
        MTL::Size::Make((size_.x + kInjectGroup - 1) / kInjectGroup,
                        (size_.y + kInjectGroup - 1) / kInjectGroup,
                        (size_.z + kInjectGroup - 1) / kInjectGroup),
        MTL::Size::Make(kInjectGroup, kInjectGroup, kInjectGroup));
    compute->endEncoding();

    compute = commandBuffer->computeCommandEncoder();
    compute->setLabel(NS::String::string("volumetric fog integrate", NS::UTF8StringEncoding));
    compute->setComputePipelineState(mtlIntegrate_);
    compute->setBuffer(perFrame, perFrameOffset, mtlIntegratePerFrame_);
    compute->setBuffer(params, 0, mtlIntegrateParams_);
    compute->setTexture(mtlScatter_[slot], mtlIntegrateCells_);
    compute->setSamplerState(linear, mtlIntegrateCellsSampler_);
    compute->setTexture(mtlIntegrated_[slot], mtlIntegrateOut_);
    compute->dispatchThreadgroups(
        MTL::Size::Make((size_.x + kIntegrateGroup - 1) / kIntegrateGroup,
                        (size_.y + kIntegrateGroup - 1) / kIntegrateGroup, 1),
        MTL::Size::Make(kIntegrateGroup, kIntegrateGroup, 1));
    compute->endEncoding();

    recorded(slot, in);
}

MTL::Texture* VolumetricFog::volumeMetal(uint64_t frame) const {
    return volumesReady_ ? mtlIntegrated_[frame % MAX_FRAMES] : nullptr;
}
#endif

} // namespace rendering
} // namespace wowee
