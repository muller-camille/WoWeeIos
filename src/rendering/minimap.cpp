#include "rendering/minimap.hpp"
#include "rendering/vk_context.hpp"
#include "rendering/vk_texture.hpp"
#include "rendering/vk_render_target.hpp"
#include "rendering/vk_pipeline.hpp"
#include "rendering/vk_shader.hpp"
#include "rendering/vk_utils.hpp"
#include "rendering/camera.hpp"
#include "pipeline/asset_manager.hpp"
#include "pipeline/blp_loader.hpp"
#include "core/coordinates.hpp"
#include "core/logger.hpp"
#ifdef WOWEE_METAL
#include <Metal/Metal.hpp>
#include "rendering/metal/metal_context.hpp"
#include "rendering/metal/metal_pipeline.hpp"
#endif
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <sstream>
#include <cmath>

namespace wowee {
namespace rendering {

// Push constant for tile composite vertex shader
struct MinimapTilePush {
    glm::vec2 gridOffset;  // 8 bytes
};

// Push constant for display vertex + fragment shaders. Declared in the header
// too, for fillDisplayPush.
struct MinimapDisplayPush {
    glm::vec4 rect;         // x, y, w, h in 0..1 screen space
    glm::vec2 playerUV;
    float rotation;
    float arrowRotation;
    float zoomRadius;
    int32_t squareShape;
    float opacity;
};  // 44 bytes

Minimap::Minimap() = default;

Minimap::~Minimap() {
    shutdown();
}

/// The pipeline that draws the minimap's assembled texture to the screen.
///
/// initialize() builds it beside the tile pipeline that composes that texture;
/// recreatePipelines() rebuilds only this one, because the tile pipeline draws
/// into an offscreen pass a settings change does not touch. Both described it
/// identically, vertex layout included.
void Minimap::buildDisplayPipeline(VkDevice device,
                                   const VkPipelineShaderStageCreateInfo& vertStage,
                                   const VkPipelineShaderStageCreateInfo& fragStage) {
    // Two vec2s per vertex: position then texture coordinate.
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = 4 * sizeof(float);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attrs(2);
    attrs[0] = { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0 };
    attrs[1] = { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 2 * sizeof(float) };

    displayPipeline = PipelineBuilder()
        .setShaders(vertStage, fragStage)
        .setVertexInput({ binding }, attrs)
        .setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE)
        .setNoDepthTest()
        .setColorBlendAttachment(PipelineBuilder::blendAlpha())
        .setMultisample(targetPass_ != VK_NULL_HANDLE ? targetSamples_
                                                      : vkCtx->getMsaaSamples())
        .setLayout(displayPipelineLayout)
        .setRenderPass(targetPass_ != VK_NULL_HANDLE ? targetPass_
                                                     : vkCtx->getImGuiRenderPass())
        .setDynamicStates(viewportAndScissorDynamic())
        .build(device, vkCtx->getPipelineCache());
}

bool Minimap::initialize(VkContext* ctx, VkDescriptorSetLayout /*perFrameLayout*/, int size) {
    vkCtx = ctx;
    mapSize = size;
    VkDevice device = vkCtx->getDevice();

    // --- Composite render target (768x768) ---
    compositeTarget = std::make_unique<VkRenderTarget>();
    if (!compositeTarget->create(*vkCtx, COMPOSITE_PX, COMPOSITE_PX)) {
        LOG_ERROR("Minimap: failed to create composite render target");
        return false;
    }

    // --- No-data fallback texture (dark blue-gray, 1x1) ---
    noDataTexture = std::make_unique<VkTexture>();
    uint8_t darkPixel[4] = { 12, 20, 30, 255 };
    noDataTexture->upload(*vkCtx, darkPixel, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, false);
    noDataTexture->createSampler(device, VK_FILTER_NEAREST, VK_FILTER_NEAREST,
                                 VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f);

    // --- Shared quad vertex buffer (unit quad: pos2 + uv2) ---
    float quadVerts[] = {
        // pos (x,y), uv (u,v)
        0.0f, 0.0f,  0.0f, 0.0f,
        1.0f, 0.0f,  1.0f, 0.0f,
        1.0f, 1.0f,  1.0f, 1.0f,
        0.0f, 0.0f,  0.0f, 0.0f,
        1.0f, 1.0f,  1.0f, 1.0f,
        0.0f, 1.0f,  0.0f, 1.0f,
    };
    auto quadBuf = uploadBuffer(*vkCtx, quadVerts, sizeof(quadVerts),
                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    quadVB = quadBuf.buffer;
    quadVBAlloc = quadBuf.allocation;

    // --- Descriptor set layout: 1 combined image sampler at binding 0 (fragment) ---
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerSetLayout = createDescriptorSetLayout(device, { samplerBinding });

    // --- Descriptor pool ---
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = MAX_DESC_SETS;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = MAX_DESC_SETS;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &descPool);

    // --- Allocate all descriptor sets ---
    // 18 tile sets (2 frames × 9 tiles) + 1 display set = 19 total
    std::vector<VkDescriptorSetLayout> layouts(19, samplerSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool;
    allocInfo.descriptorSetCount = 19;
    allocInfo.pSetLayouts = layouts.data();

    // Checked, not assumed. On failure allSets holds nothing this code put
    // there, and every one of the nineteen is then written and bound - which
    // is an invalid handle in a live descriptor rather than a minimap that
    // does not draw.
    VkDescriptorSet allSets[19];
    if (vkAllocateDescriptorSets(device, &allocInfo, allSets) != VK_SUCCESS) {
        LOG_ERROR("Minimap: failed to allocate descriptor sets");
        return false;
    }

    for (int f = 0; f < 2; f++)
        for (int t = 0; t < 9; t++)
            tileDescSets[f][t] = allSets[f * 9 + t];
    displayDescSet = allSets[18];

    // --- Write display descriptor set → composite render target ---
    VkDescriptorImageInfo compositeImgInfo = compositeTarget->descriptorInfo();
    VkWriteDescriptorSet displayWrite{};
    displayWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    displayWrite.dstSet = displayDescSet;
    displayWrite.dstBinding = 0;
    displayWrite.descriptorCount = 1;
    displayWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    displayWrite.pImageInfo = &compositeImgInfo;
    vkUpdateDescriptorSets(device, 1, &displayWrite, 0, nullptr);

    // --- Tile pipeline layout: samplerSetLayout + 8-byte push constant (vertex) ---
    VkPushConstantRange tilePush{};
    tilePush.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    tilePush.offset = 0;
    tilePush.size = sizeof(MinimapTilePush);
    tilePipelineLayout = createPipelineLayout(device, { samplerSetLayout }, { tilePush });

    // --- Display pipeline layout: samplerSetLayout + 40-byte push constant (vert+frag) ---
    VkPushConstantRange displayPush{};
    displayPush.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    displayPush.offset = 0;
    displayPush.size = sizeof(MinimapDisplayPush);
    displayPipelineLayout = createPipelineLayout(device, { samplerSetLayout }, { displayPush });

    // --- Vertex input: pos2 (loc 0) + uv2 (loc 1), stride 16 ---
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = 4 * sizeof(float);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attrs(2);
    attrs[0] = { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0 };                    // aPos
    attrs[1] = { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 2 * sizeof(float) };    // aUV

    // --- Load tile shaders ---
    {
        VkShaderModule vs, fs;
        if (!vs.loadFromFile(device, "assets/shaders/minimap_tile.vert.spv") ||
            !fs.loadFromFile(device, "assets/shaders/minimap_tile.frag.spv")) {
            LOG_ERROR("Minimap: failed to load tile shaders");
            return false;
        }

        tilePipeline = PipelineBuilder()
            .setShaders(vs.stageInfo(VK_SHADER_STAGE_VERTEX_BIT),
                        fs.stageInfo(VK_SHADER_STAGE_FRAGMENT_BIT))
            .setVertexInput({ binding }, attrs)
            .setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .setRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE)
            .setNoDepthTest()
            .setColorBlendAttachment(PipelineBuilder::blendDisabled())
            .setLayout(tilePipelineLayout)
            .setRenderPass(compositeTarget->getRenderPass())
            .setDynamicStates(viewportAndScissorDynamic())
            .build(device, vkCtx->getPipelineCache());

        vs.destroy();
        fs.destroy();
    }

    // --- Load display shaders ---
    {
        VkShaderModule vs, fs;
        if (!vs.loadFromFile(device, "assets/shaders/minimap_display.vert.spv") ||
            !fs.loadFromFile(device, "assets/shaders/minimap_display.frag.spv")) {
            LOG_ERROR("Minimap: failed to load display shaders");
            return false;
        }

        buildDisplayPipeline(device, vs.stageInfo(VK_SHADER_STAGE_VERTEX_BIT),
                             fs.stageInfo(VK_SHADER_STAGE_FRAGMENT_BIT));

        vs.destroy();
        fs.destroy();
    }

    if (!tilePipeline || !displayPipeline) {
        LOG_ERROR("Minimap: failed to create pipelines");
        return false;
    }

    LOG_INFO("Minimap initialized (", mapSize, "x", mapSize, " screen, ",
             COMPOSITE_PX, "x", COMPOSITE_PX, " composite)");
    return true;
}

void Minimap::shutdown() {
#ifdef WOWEE_METAL
    if (metal_) {
        tileTextureCache.clear();
        tileInsertionOrder.clear();
        noDataTexture.reset();
        if (mtlComposite_) { mtlComposite_->release(); mtlComposite_ = nullptr; }
        if (mtlQuad_) { mtlQuad_->release(); mtlQuad_ = nullptr; }
        if (mtlTilePipeline_) { mtlTilePipeline_->release(); mtlTilePipeline_ = nullptr; }
        if (mtlDisplayPipeline_) { mtlDisplayPipeline_->release(); mtlDisplayPipeline_ = nullptr; }
        metal_ = nullptr;
    }
#endif
    if (!vkCtx) return;
    VkDevice device = vkCtx->getDevice();
    VmaAllocator alloc = vkCtx->getAllocator();

    vkDeviceWaitIdle(device);

    destroy(device, tilePipeline);
    destroy(device, displayPipeline);
    destroy(device, tilePipelineLayout);
    destroy(device, displayPipelineLayout);
    destroy(device, descPool);
    destroy(device, samplerSetLayout);

    destroy(alloc, quadVB, quadVBAlloc);

    for (auto& [hash, tex] : tileTextureCache) {
        if (tex) tex->destroy(device, alloc);
    }
    tileTextureCache.clear();
    tileInsertionOrder.clear();

    if (noDataTexture) { noDataTexture->destroy(device, alloc); noDataTexture.reset(); }
    if (compositeTarget) { compositeTarget->destroy(device, alloc); compositeTarget.reset(); }

    vkCtx = nullptr;
}

void Minimap::recreatePipelines() {
    if (!vkCtx || !displayPipelineLayout) return;
    VkDevice device = vkCtx->getDevice();

    destroy(device, displayPipeline);

    VkShaderModule vs, fs;
    if (!vs.loadFromFile(device, "assets/shaders/minimap_display.vert.spv") ||
        !fs.loadFromFile(device, "assets/shaders/minimap_display.frag.spv")) {
        LOG_ERROR("Minimap: failed to reload display shaders for pipeline recreation");
        return;
    }

    buildDisplayPipeline(device, vs.stageInfo(VK_SHADER_STAGE_VERTEX_BIT),
                         fs.stageInfo(VK_SHADER_STAGE_FRAGMENT_BIT));

    vs.destroy();
    fs.destroy();

    LOG_INFO("Minimap: display pipeline recreated with MSAA ", static_cast<int>(vkCtx->getMsaaSamples()), "x");
}

void Minimap::setMapName(const std::string& name) {
    if (mapName != name) {
        mapName = name;
        hasCachedFrame = false;
        lastCenterTileX = -1;
        lastCenterTileY = -1;
    }
}

// --------------------------------------------------------
// TRS parsing
// --------------------------------------------------------

void Minimap::parseTRS() {
    if (trsParsed || !assetManager) return;
    trsParsed = true;

    auto data = assetManager->readFile("Textures\\Minimap\\md5translate.trs");
    if (data.empty()) {
        LOG_WARNING("Failed to load md5translate.trs");
        return;
    }

    std::string content(reinterpret_cast<const char*>(data.data()), data.size());
    std::istringstream stream(content);
    std::string line;
    int count = 0;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line.substr(0, 4) == "dir:") continue;

        auto tabPos = line.find('\t');
        if (tabPos == std::string::npos) continue;

        std::string key = line.substr(0, tabPos);
        std::string hashFile = line.substr(tabPos + 1);

        if (key.size() > 4 && key.substr(key.size() - 4) == ".blp")
            key = key.substr(0, key.size() - 4);
        if (hashFile.size() > 4 && hashFile.substr(hashFile.size() - 4) == ".blp")
            hashFile = hashFile.substr(0, hashFile.size() - 4);

        trsLookup[key] = hashFile;
        count++;
    }

    LOG_INFO("Parsed md5translate.trs: ", count, " entries");
}

// --------------------------------------------------------
// Tile texture loading
// --------------------------------------------------------

VkTexture* Minimap::getOrLoadTileTexture(int tileX, int tileY) {
    if (!trsParsed) parseTRS();

    std::string key = mapName + "\\map" + std::to_string(tileX) + "_" + std::to_string(tileY);

    auto trsIt = trsLookup.find(key);
    if (trsIt == trsLookup.end())
        return noDataTexture.get();

    const std::string& hash = trsIt->second;

    auto cacheIt = tileTextureCache.find(hash);
    if (cacheIt != tileTextureCache.end())
        return cacheIt->second.get();

    // Load from MPQ
    std::string blpPath = "Textures\\Minimap\\" + hash + ".blp";
    auto blpImage = assetManager->loadTexture(blpPath);
    if (!blpImage.isValid()) {
        tileTextureCache[hash] = nullptr;  // Mark as failed
        return noDataTexture.get();
    }

    auto tex = std::make_unique<VkTexture>();
#ifdef WOWEE_METAL
    if (metal_) {
        if (!tex->uploadMetal(*metal_, blpImage.data.data(), blpImage.width, blpImage.height,
                              false)) {
            tileTextureCache[hash] = nullptr;
            return noDataTexture.get();
        }
        tex->setMetalSampler(metal_->sampler(MetalContext::Filter::Linear,
                                             MetalContext::Address::ClampToEdge));
    } else
#endif
    {
        tex->upload(*vkCtx, blpImage.data.data(), blpImage.width, blpImage.height,
                    VK_FORMAT_R8G8B8A8_UNORM, false);
        tex->createSampler(vkCtx->getDevice(), VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                           VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f);
    }

    VkTexture* ptr = tex.get();
    tileTextureCache[hash] = std::move(tex);
    tileInsertionOrder.push_back(hash);

    // Evict oldest tiles when cache grows too large to bound GPU memory usage.
    while (tileInsertionOrder.size() > MAX_TILE_CACHE) {
        const std::string& oldest = tileInsertionOrder.front();
        tileTextureCache.erase(oldest);
        tileInsertionOrder.pop_front();
    }

    return ptr;
}

// --------------------------------------------------------
// Update tile descriptor sets for composite pass
// --------------------------------------------------------

void Minimap::updateTileDescriptors(uint32_t frameIdx, int centerTileX, int centerTileY) {
    constexpr int kTileCount = 9; // 3x3 grid
    VkDevice device = vkCtx->getDevice();
    std::array<VkDescriptorImageInfo, kTileCount> imgInfos{};
    std::array<VkWriteDescriptorSet, kTileCount> writes{};
    int slot = 0;

    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            int tx = centerTileX + dr;
            int ty = centerTileY + dc;

            VkTexture* tileTex = getOrLoadTileTexture(tx, ty);
            if (!tileTex || !tileTex->isValid())
                tileTex = noDataTexture.get();

            imgInfos[slot] = tileTex->descriptorInfo();

            writes[slot] = {};
            writes[slot].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[slot].dstSet = tileDescSets[frameIdx][slot];
            writes[slot].dstBinding = 0;
            writes[slot].descriptorCount = 1;
            writes[slot].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[slot].pImageInfo = &imgInfos[slot];
            slot++;
        }
    }

    vkUpdateDescriptorSets(device, kTileCount, writes.data(), 0, nullptr);
}

// --------------------------------------------------------
// Off-screen composite pass (call BEFORE main render pass)
// --------------------------------------------------------

void Minimap::compositePass(VkCommandBuffer cmd, const glm::vec3& centerWorldPos) {
    if (!enabled || !assetManager || !compositeTarget || !compositeTarget->isValid()) return;

    if (!trsParsed) parseTRS();

    int curTileX = 0, curTileY = 0;
    if (!compositeNeedsRefresh(centerWorldPos, curTileX, curTileY)) return;

    uint32_t frameIdx = vkCtx->getCurrentFrame();

    // Update tile descriptor sets
    updateTileDescriptors(frameIdx, curTileX, curTileY);

    // Begin off-screen render pass
    VkClearColorValue clearColor = {{ 0.05f, 0.08f, 0.12f, 1.0f }};
    compositeTarget->beginPass(cmd, clearColor);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, tilePipeline);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &quadVB, &offset);

    // Draw 3x3 tile grid
    int slot = 0;
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    tilePipelineLayout, 0, 1,
                                    &tileDescSets[frameIdx][slot], 0, nullptr);

            MinimapTilePush push{};
            push.gridOffset = glm::vec2(static_cast<float>(dc + 1),
                                        static_cast<float>(dr + 1));
            vkCmdPushConstants(cmd, tilePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(push), &push);

            vkCmdDraw(cmd, 6, 1, 0, 0);
            slot++;
        }
    }

    compositeTarget->endPass(cmd);
    markComposited(centerWorldPos, curTileX, curTileY);
}

bool Minimap::compositeNeedsRefresh(const glm::vec3& centerWorldPos,
                                    int& tileX, int& tileY) const {
    bool needsRefresh = !hasCachedFrame;
    if (!needsRefresh) {
        float mdx = centerWorldPos.x - lastUpdatePos.x;
        float mdy = centerWorldPos.y - lastUpdatePos.y;
        float movedSq = mdx * mdx + mdy * mdy;
        float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() -
                                                     lastUpdateTime).count();
        needsRefresh = (movedSq >= updateDistance * updateDistance) || (elapsed >= updateIntervalSec);
    }

    // Also refresh if player crossed a tile boundary
    auto [curTileX, curTileY] = core::coords::worldToTile(centerWorldPos.x, centerWorldPos.y);
    tileX = curTileX;
    tileY = curTileY;
    return needsRefresh || curTileX != lastCenterTileX || curTileY != lastCenterTileY;
}

void Minimap::markComposited(const glm::vec3& centerWorldPos, int tileX, int tileY) {
    lastCenterTileX = tileX;
    lastCenterTileY = tileY;
    lastUpdateTime = std::chrono::steady_clock::now();
    lastUpdatePos = centerWorldPos;
    hasCachedFrame = true;
}

// --------------------------------------------------------
// Display quad (call INSIDE main render pass)
// --------------------------------------------------------

void Minimap::render(VkCommandBuffer cmd, const Camera& playerCamera,
                     const glm::vec3& centerWorldPos,
                     int screenWidth, int screenHeight,
                     float playerOrientation, bool hasPlayerOrientation) {
    if (!enabled || !hasCachedFrame || !displayPipeline) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, displayPipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            displayPipelineLayout, 0, 1,
                            &displayDescSet, 0, nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &quadVB, &offset);

    MinimapDisplayPush push{};
    fillDisplayPush(push, playerCamera, centerWorldPos, screenWidth, screenHeight,
                    playerOrientation, hasPlayerOrientation);
    vkCmdPushConstants(cmd, displayPipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(push), &push);

    vkCmdDraw(cmd, 6, 1, 0, 0);
}

void Minimap::fillDisplayPush(MinimapDisplayPush& push, const Camera& playerCamera,
                              const glm::vec3& centerWorldPos,
                              int screenWidth, int screenHeight,
                              float playerOrientation, bool hasPlayerOrientation) const {
    // Top-right corner, unless something asked for a particular rect - which
    // is what happens when FrameXML owns the minimap and the map has to sit
    // inside the frame it drew.
    float margin = 10.0f;
    float pixelW, pixelH, x, y;
    if (haveRect_ && rectW_ > 0.0f && rectH_ > 0.0f) {
        pixelW = rectW_ / screenWidth;
        pixelH = rectH_ / screenHeight;
        x = rectX_ / screenWidth;
        y = rectY_ / screenHeight;   // y=0 is the top edge in Vulkan
    } else {
        pixelW = static_cast<float>(mapSize) / screenWidth;
        pixelH = static_cast<float>(mapSize) / screenHeight;
        x = 1.0f - pixelW - margin / screenWidth;
        y = margin / screenHeight;
    }

    // Compute player's UV in the composite texture
    constexpr float TILE_SIZE = core::coords::TILE_SIZE;
    auto [tileX, tileY] = core::coords::worldToTile(centerWorldPos.x, centerWorldPos.y);

    float fracNS = 32.0f - static_cast<float>(tileX) - centerWorldPos.y / TILE_SIZE;
    float fracEW = 32.0f - static_cast<float>(tileY) - centerWorldPos.x / TILE_SIZE;

    float playerU = (1.0f + fracEW) / 3.0f;
    float playerV = (1.0f + fracNS) / 3.0f;

    float zoomRadius = viewRadius / (TILE_SIZE * 3.0f);

    // Rotating with the camera is off everywhere: the saved setting is read and
    // dropped in loadSettings, since "Stabilize transports and correct minimap
    // orientation". This is why, as far as it can be worked out without the
    // client on screen.
    //
    // The two modes disagree by half a turn. Take the heading where
    // atan2(-fwd.x, fwd.y) is 0. North-up draws the arrow at pi - that same
    // expression, which is pi: pointing down, so that heading renders as south.
    // Rotating mode turns the map by the expression itself, which is 0 - no
    // rotation at all - while pinning the arrow up. So the map says the player
    // faces north and the arrow agrees, and both are half a turn from where
    // north-up puts them.
    //
    // North-up is the mode that has been looked at, so its arrow is the one to
    // trust: the map wants turning by the negative of where that arrow points,
    // not by the expression the arrow is built from.
    //
    // The direction is a second question and not settled here. What the shader
    // does with this angle is a mirror as well as a turn - the matrix it builds
    // has determinant -1 - so the map may also rotate the wrong way once the
    // half turn is accounted for. That mirror is right for north-up, where the
    // angle is zero, which is what makes this hard to reason about and easy to
    // check by turning it on and walking north.
    float rotation = 0.0f;
    if (rotateWithCamera) {
        glm::vec3 fwd = playerCamera.getForward();
        rotation = std::atan2(-fwd.x, fwd.y);
    }

    float arrowRotation = 0.0f;
    if (!rotateWithCamera) {
        if (hasPlayerOrientation) {
            arrowRotation = playerOrientation;
        } else {
            glm::vec3 fwd = playerCamera.getForward();
            arrowRotation = glm::pi<float>() - std::atan2(-fwd.x, fwd.y);
        }
    } else if (hasPlayerOrientation) {
        // Show character facing relative to the rotated map
        arrowRotation = playerOrientation + rotation;
    }

    push = {};
    push.rect = glm::vec4(x, y, pixelW, pixelH);
    push.playerUV = glm::vec2(playerU, playerV);
    push.rotation = rotation;
    push.arrowRotation = arrowRotation;
    push.zoomRadius = zoomRadius;
    push.squareShape = squareShape ? 1 : 0;
    push.opacity = opacity_;
}

#ifdef WOWEE_METAL
bool Minimap::initializeMetal(MetalContext* ctx, uint32_t colorFormat, uint32_t depthFormat,
                              uint32_t sampleCount, int size) {
    if (!ctx) return false;
    mapSize = size;

    const MetalBindings tileVert("minimap_tile_vert");
    const MetalBindings tileFrag("minimap_tile_frag");
    const MetalBindings displayVert("minimap_display_vert");
    const MetalBindings displayFrag("minimap_display_frag");
    mtlTileVertPush_ = tileVert.pushConstants();
    mtlTileTex_ = tileFrag.texture(0, 0);
    mtlTileSampler_ = tileFrag.sampler(0, 0);
    mtlDisplayVertPush_ = displayVert.pushConstants();
    mtlDisplayFragPush_ = displayFrag.pushConstants();
    mtlDisplayTex_ = displayFrag.texture(0, 0);
    mtlDisplaySampler_ = displayFrag.sampler(0, 0);
    if (!tileVert.valid() || !tileFrag.valid() || !displayVert.valid() || !displayFrag.valid())
        return false;

    // Two vec2s per vertex, position then texture coordinate, as on Vulkan.
    auto* vd = MTL::VertexDescriptor::alloc()->init();
    vd->attributes()->object(0)->setFormat(MTL::VertexFormatFloat2);
    vd->attributes()->object(0)->setOffset(0);
    vd->attributes()->object(0)->setBufferIndex(kMetalVertexBufferIndex);
    vd->attributes()->object(1)->setFormat(MTL::VertexFormatFloat2);
    vd->attributes()->object(1)->setOffset(2 * sizeof(float));
    vd->attributes()->object(1)->setBufferIndex(kMetalVertexBufferIndex);
    vd->layouts()->object(kMetalVertexBufferIndex)->setStride(4 * sizeof(float));

    MetalPipelineDesc desc;
    desc.vertexDescriptor = vd;
    desc.vertexFunction = "minimap_tile_vert";
    desc.fragmentFunction = "minimap_tile_frag";
    desc.colorFormat = MTL::PixelFormatRGBA8Unorm;
    desc.label = "minimap tiles";
    mtlTilePipeline_ = buildMetalPipeline(*ctx, desc);

    desc.vertexFunction = "minimap_display_vert";
    desc.fragmentFunction = "minimap_display_frag";
    desc.colorFormat = colorFormat;
    // Drawn inside the world pass, so it declares that pass's depth, unused.
    desc.depthFormat = depthFormat;
    desc.sampleCount = sampleCount;
    desc.blend = MetalBlend::Alpha;
    desc.label = "minimap";
    mtlDisplayPipeline_ = buildMetalPipeline(*ctx, desc);
    vd->release();
    if (!mtlTilePipeline_ || !mtlDisplayPipeline_) return false;

    auto* texDesc = MTL::TextureDescriptor::texture2DDescriptor(
        MTL::PixelFormatRGBA8Unorm, COMPOSITE_PX, COMPOSITE_PX, false);
    texDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
    texDesc->setStorageMode(MTL::StorageModePrivate);
    mtlComposite_ = ctx->getDevice()->newTexture(texDesc);

    const float quadVerts[] = {
        0.0f, 0.0f,  0.0f, 0.0f,
        1.0f, 0.0f,  1.0f, 0.0f,
        1.0f, 1.0f,  1.0f, 1.0f,
        0.0f, 0.0f,  0.0f, 0.0f,
        1.0f, 1.0f,  1.0f, 1.0f,
        0.0f, 1.0f,  0.0f, 1.0f,
    };
    mtlQuad_ = ctx->newBuffer(quadVerts, sizeof(quadVerts));
    if (!mtlComposite_ || !mtlQuad_) return false;

    metal_ = ctx;
    noDataTexture = std::make_unique<VkTexture>();
    const uint8_t darkPixel[4] = { 12, 20, 30, 255 };
    noDataTexture->uploadMetal(*ctx, darkPixel, 1, 1, false);
    noDataTexture->setMetalSampler(ctx->sampler(MetalContext::Filter::Nearest,
                                                MetalContext::Address::ClampToEdge));
    LOG_INFO("Minimap initialized (Metal, ", COMPOSITE_PX, "x", COMPOSITE_PX, " composite)");
    return true;
}

void Minimap::compositeMetal(MTL::CommandBuffer* commandBuffer, const glm::vec3& centerWorldPos) {
    if (!enabled || !assetManager || !metal_ || !mtlComposite_ || !commandBuffer) return;
    if (!trsParsed) parseTRS();

    int curTileX = 0, curTileY = 0;
    if (!compositeNeedsRefresh(centerWorldPos, curTileX, curTileY)) return;

    auto* pass = MTL::RenderPassDescriptor::renderPassDescriptor();
    auto* color = pass->colorAttachments()->object(0);
    color->setTexture(mtlComposite_);
    color->setLoadAction(MTL::LoadActionClear);
    color->setClearColor(MTL::ClearColor::Make(0.05, 0.08, 0.12, 1.0));
    color->setStoreAction(MTL::StoreActionStore);
    MTL::RenderCommandEncoder* encoder = commandBuffer->renderCommandEncoder(pass);
    encoder->setLabel(NS::String::string("minimap composite", NS::UTF8StringEncoding));
    encoder->setRenderPipelineState(mtlTilePipeline_);
    encoder->setCullMode(MTL::CullModeNone);
    encoder->setVertexBuffer(mtlQuad_, 0, kMetalVertexBufferIndex);

    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            VkTexture* tileTex = getOrLoadTileTexture(curTileX + dr, curTileY + dc);
            if (!tileTex || !tileTex->metalTexture()) tileTex = noDataTexture.get();
            encoder->setFragmentTexture(tileTex->metalTexture(), mtlTileTex_);
            encoder->setFragmentSamplerState(tileTex->metalSampler(), mtlTileSampler_);

            MinimapTilePush push{};
            push.gridOffset = glm::vec2(static_cast<float>(dc + 1),
                                        static_cast<float>(dr + 1));
            encoder->setVertexBytes(&push, sizeof(push), mtlTileVertPush_);
            encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(6));
        }
    }
    encoder->endEncoding();
    markComposited(centerWorldPos, curTileX, curTileY);
}

void Minimap::renderMetal(MTL::RenderCommandEncoder* encoder, const Camera& playerCamera,
                          const glm::vec3& centerWorldPos, int screenWidth, int screenHeight,
                          float playerOrientation, bool hasPlayerOrientation) {
    if (!enabled || !hasCachedFrame || !mtlDisplayPipeline_ || !encoder) return;

    MinimapDisplayPush push{};
    fillDisplayPush(push, playerCamera, centerWorldPos, screenWidth, screenHeight,
                    playerOrientation, hasPlayerOrientation);

    encoder->setRenderPipelineState(mtlDisplayPipeline_);
    encoder->setDepthStencilState(metal_->depthState(false, false));
    encoder->setCullMode(MTL::CullModeNone);
    encoder->setVertexBuffer(mtlQuad_, 0, kMetalVertexBufferIndex);
    encoder->setVertexBytes(&push, sizeof(push), mtlDisplayVertPush_);
    encoder->setFragmentBytes(&push, sizeof(push), mtlDisplayFragPush_);
    encoder->setFragmentTexture(mtlComposite_, mtlDisplayTex_);
    encoder->setFragmentSamplerState(metal_->sampler(MetalContext::Filter::Linear,
                                                     MetalContext::Address::ClampToEdge),
                                     mtlDisplaySampler_);
    encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(6));
}
#endif

} // namespace rendering
} // namespace wowee
