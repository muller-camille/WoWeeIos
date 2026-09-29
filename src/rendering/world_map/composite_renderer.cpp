// composite_renderer.cpp - Vulkan off-screen composite rendering for the world map.
// Extracted from WorldMap::initialize, shutdown, compositePass, loadZoneTextures,
// loadOverlayTextures, destroyZoneTextures (Phase 7 of refactoring plan).
#include "rendering/world_map/composite_renderer.hpp"
#include "rendering/vk_context.hpp"
#include "rendering/vk_texture.hpp"
#include "rendering/vk_render_target.hpp"
#include "rendering/vk_pipeline.hpp"
#include "rendering/vk_shader.hpp"
#include "rendering/vk_utils.hpp"
#include "pipeline/asset_manager.hpp"
#include "core/logger.hpp"
#ifdef WOWEE_METAL
#include <Metal/Metal.hpp>
#include "rendering/metal/metal_context.hpp"
#include "rendering/metal/metal_pipeline.hpp"
#endif

#include <algorithm>
#include <cstddef>

namespace wowee {
namespace rendering {
namespace world_map {

CompositeRenderer::CompositeRenderer() = default;

CompositeRenderer::~CompositeRenderer() {
    shutdown();
}

void CompositeRenderer::ensureTextureSlots(size_t zoneCount, const std::vector<Zone>& zones) {
    if (zoneTextureSlots_.size() >= zoneCount) return;
    zoneTextureSlots_.resize(zoneCount);
    for (size_t i = 0; i < zoneCount; i++) {
        auto& slots = zoneTextureSlots_[i];
        if (slots.overlays.size() != zones[i].overlays.size()) {
            slots.overlays.resize(zones[i].overlays.size());
            for (size_t oi = 0; oi < zones[i].overlays.size(); oi++) {
                const auto& ov = zones[i].overlays[oi];
                slots.overlays[oi].tiles.resize(static_cast<size_t>(ov.tileCols) * static_cast<size_t>(ov.tileRows), nullptr);
            }
        }
    }
}

bool CompositeRenderer::initialize(VkContext* ctx, pipeline::AssetManager* am) {
    if (initialized) return true;
    vkCtx = ctx;
    assetManager = am;
    VkDevice device = vkCtx->getDevice();

    // --- Composite render target (1024x768) ---
    compositeTarget = std::make_unique<VkRenderTarget>();
    if (!compositeTarget->create(*vkCtx, FBO_W, FBO_H)) {
        LOG_ERROR("CompositeRenderer: failed to create composite render target");
        return false;
    }

    // --- Quad vertex buffer (unit quad: pos2 + uv2) ---
    float quadVerts[] = {
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

    // --- Descriptor set layout: 1 combined image sampler at binding 0 ---
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

    // --- Allocate descriptor sets ---
    constexpr uint32_t tileSetCount = 24;
    constexpr uint32_t overlaySetCount = MAX_OVERLAY_TILES * 2;
    constexpr uint32_t totalSets = tileSetCount + 1 + 1 + overlaySetCount;
    std::vector<VkDescriptorSetLayout> layouts(totalSets, samplerSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool;
    allocInfo.descriptorSetCount = totalSets;
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> allSets(totalSets);
    vkAllocateDescriptorSets(device, &allocInfo, allSets.data());

    uint32_t si = 0;
    for (auto& tileRow : tileDescSets)
        for (auto& tileSet : tileRow)
            tileSet = allSets[si++];
    imguiDisplaySet = allSets[si++];
    fogDescSet_ = allSets[si++];
    for (auto& overlayRow : overlayDescSets_)
        for (auto& overlaySet : overlayRow)
            overlaySet = allSets[si++];

    // --- Write display descriptor set → composite render target ---
    VkDescriptorImageInfo compositeImgInfo = compositeTarget->descriptorInfo();
    VkWriteDescriptorSet displayWrite{};
    displayWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    displayWrite.dstSet = imguiDisplaySet;
    displayWrite.dstBinding = 0;
    displayWrite.descriptorCount = 1;
    displayWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    displayWrite.pImageInfo = &compositeImgInfo;
    vkUpdateDescriptorSets(device, 1, &displayWrite, 0, nullptr);

    // --- Pipeline layout: samplerSetLayout + push constant (16 bytes, vertex) ---
    VkPushConstantRange tilePush{};
    tilePush.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    tilePush.offset = 0;
    tilePush.size = sizeof(WorldMapTilePush);
    tilePipelineLayout = createPipelineLayout(device, { samplerSetLayout }, { tilePush });

    // --- Vertex input: pos2 (loc 0) + uv2 (loc 1), stride 16 ---
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = 4 * sizeof(float);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attrs(2);
    attrs[0] = { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0 };
    attrs[1] = { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 2 * sizeof(float) };

    // --- Load tile shaders and build pipeline ---
    {
        auto shaders = loadShaderPair(device, "assets/shaders/world_map.vert.spv", "assets/shaders/world_map.frag.spv", "world_map tile");
        if (!shaders) return false;

        tilePipeline = PipelineBuilder()
            .setShaders(shaders.vertStage, shaders.fragStage)
            .setVertexInput({ binding }, attrs)
            .setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .setRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE)
            .setNoDepthTest()
            .setColorBlendAttachment(PipelineBuilder::blendDisabled())
            .setLayout(tilePipelineLayout)
            .setRenderPass(compositeTarget->getRenderPass())
            .setDynamicStates(viewportAndScissorDynamic())
            .build(device, vkCtx->getPipelineCache());

    }

    if (!tilePipeline) {
        LOG_ERROR("CompositeRenderer: failed to create tile pipeline");
        return false;
    }

    // --- Overlay pipeline (alpha-blended) ---
    {
        VkPushConstantRange overlayPushVert{};
        overlayPushVert.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        overlayPushVert.offset = 0;
        overlayPushVert.size = sizeof(WorldMapTilePush);

        VkPushConstantRange overlayPushFrag{};
        overlayPushFrag.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        overlayPushFrag.offset = offsetof(OverlayPush, tintColor);
        overlayPushFrag.size = sizeof(glm::vec4);

        overlayPipelineLayout_ = createPipelineLayout(device, { samplerSetLayout },
                                                       { overlayPushVert, overlayPushFrag });

        auto shaders = loadShaderPair(device, "assets/shaders/world_map.vert.spv", "assets/shaders/world_map_fog.frag.spv", "world_map overlay");
        if (!shaders) return false;

        overlayPipeline_ = PipelineBuilder()
            .setShaders(shaders.vertStage, shaders.fragStage)
            .setVertexInput({ binding }, attrs)
            .setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .setRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE)
            .setNoDepthTest()
            .setColorBlendAttachment(PipelineBuilder::blendAlpha())
            .setLayout(overlayPipelineLayout_)
            .setRenderPass(compositeTarget->getRenderPass())
            .setDynamicStates(viewportAndScissorDynamic())
            .build(device, vkCtx->getPipelineCache());

    }

    if (!overlayPipeline_) {
        LOG_ERROR("CompositeRenderer: failed to create overlay pipeline");
        return false;
    }

    // --- 1×1 white fog texture ---
    {
        uint8_t white[] = { 255, 255, 255, 255 };
        fogTexture_ = std::make_unique<VkTexture>();
        fogTexture_->upload(*vkCtx, white, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, false);
        fogTexture_->createSampler(device, VK_FILTER_NEAREST, VK_FILTER_NEAREST,
                                   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f);

        // upload and createSampler both answer nothing, and the image view is
        // created without being checked either, so a 1x1 texture is not
        // guaranteed sampleable just because it is tiny. Writing an
        // unsampleable one into this set costs the device rather than the fog.
        // See #123.
        if (!fogTexture_->isValid()) {
            LOG_ERROR("CompositeRenderer: the fog texture is not sampleable");
            return false;
        }
        VkDescriptorImageInfo fogImgInfo = fogTexture_->descriptorInfo();
        VkWriteDescriptorSet fogWrite{};
        fogWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        fogWrite.dstSet = fogDescSet_;
        fogWrite.dstBinding = 0;
        fogWrite.descriptorCount = 1;
        fogWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        fogWrite.pImageInfo = &fogImgInfo;
        vkUpdateDescriptorSets(device, 1, &fogWrite, 0, nullptr);
    }

    initialized = true;
    LOG_INFO("CompositeRenderer initialized (", FBO_W, "x", FBO_H, " composite)");
    return true;
}

void CompositeRenderer::shutdown() {
#ifdef WOWEE_METAL
    if (metal_) {
        zoneTextures.clear();
        zoneTextureSlots_.clear();
        fogTexture_.reset();
        if (mtlComposite_) { mtlComposite_->release(); mtlComposite_ = nullptr; }
        if (mtlQuad_) { mtlQuad_->release(); mtlQuad_ = nullptr; }
        if (mtlTilePipeline_) { mtlTilePipeline_->release(); mtlTilePipeline_ = nullptr; }
        if (mtlOverlayPipeline_) { mtlOverlayPipeline_->release(); mtlOverlayPipeline_ = nullptr; }
        initialized = false;
        metal_ = nullptr;
    }
#endif
    if (!vkCtx) return;
    VkDevice device = vkCtx->getDevice();
    VmaAllocator alloc = vkCtx->getAllocator();

    vkDeviceWaitIdle(device);

    destroy(device, tilePipeline);
    destroy(device, tilePipelineLayout);
    destroy(device, overlayPipeline_);
    destroy(device, overlayPipelineLayout_);
    destroy(device, descPool);
    destroy(device, samplerSetLayout);
    destroy(alloc, quadVB, quadVBAlloc);

    for (auto& tex : zoneTextures) {
        if (tex) tex->destroy(device, alloc);
    }
    zoneTextures.clear();
    zoneTextureSlots_.clear();

    if (fogTexture_) { fogTexture_->destroy(device, alloc); fogTexture_.reset(); }
    if (compositeTarget) { compositeTarget->destroy(device, alloc); compositeTarget.reset(); }

    initialized = false;
    vkCtx = nullptr;
}

void CompositeRenderer::loadZoneTextures(int zoneIdx, std::vector<Zone>& zones,
                                          const std::string& mapName) {
    if (zoneIdx < 0 || zoneIdx >= static_cast<int>(zones.size())) return;
    ensureTextureSlots(zones.size(), zones);
    auto& slots = zoneTextureSlots_[zoneIdx];
    if (slots.tilesLoaded) return;
    slots.tilesLoaded = true;

    const auto& zone = zones[zoneIdx];
    const std::string& folder = zone.areaName;
    if (folder.empty()) return;

    LOG_INFO("loadZoneTextures: zone[", zoneIdx, "] areaName='", zone.areaName,
             "' areaID=", zone.areaID, " mapName='", mapName, "'");

    int loaded = 0;

    for (int i = 0; i < 12; i++) {
        std::string path = "Interface\\WorldMap\\" + folder + "\\" +
                           folder + std::to_string(i + 1) + ".blp";
        auto blpImage = assetManager->loadTexture(path);
        if (!blpImage.isValid()) {
            slots.tileTextures[i] = nullptr;
            continue;
        }

        auto tex = uploadTile(blpImage);
        if (!tex) {
            slots.tileTextures[i] = nullptr;
            continue;
        }
        slots.tileTextures[i] = tex.get();
        zoneTextures.push_back(std::move(tex));
        loaded++;
    }

    LOG_INFO("CompositeRenderer: loaded ", loaded, "/12 tiles for '", folder, "'");
}

void CompositeRenderer::loadOverlayTextures(int zoneIdx, std::vector<Zone>& zones) {
    if (zoneIdx < 0 || zoneIdx >= static_cast<int>(zones.size())) return;
    ensureTextureSlots(zones.size(), zones);

    const auto& zone = zones[zoneIdx];
    auto& slots = zoneTextureSlots_[zoneIdx];
    if (zone.overlays.empty()) return;

    const std::string& folder = zone.areaName;
    if (folder.empty()) return;

    int totalLoaded = 0;

    for (size_t oi = 0; oi < zone.overlays.size(); oi++) {
        const auto& ov = zone.overlays[oi];
        auto& ovSlots = slots.overlays[oi];
        if (ovSlots.tilesLoaded) continue;
        ovSlots.tilesLoaded = true;

        int tileCount = ov.tileCols * ov.tileRows;
        for (int t = 0; t < tileCount; t++) {
            std::string tileName = ov.textureName + std::to_string(t + 1);
            std::string path = "Interface\\WorldMap\\" + folder + "\\" + tileName + ".blp";
            auto blpImage = assetManager->loadTexture(path);
            if (!blpImage.isValid()) {
                ovSlots.tiles[t] = nullptr;
                continue;
            }

            auto tex = uploadTile(blpImage);
            if (!tex) {
                ovSlots.tiles[t] = nullptr;
                continue;
            }
            ovSlots.tiles[t] = tex.get();
            zoneTextures.push_back(std::move(tex));
            totalLoaded++;
        }
    }

    LOG_INFO("CompositeRenderer: loaded ", totalLoaded, " overlay tiles for '", folder, "'");
}

std::unique_ptr<VkTexture> CompositeRenderer::uploadTile(const pipeline::BLPImage& image) {
    auto tex = std::make_unique<VkTexture>();
#ifdef WOWEE_METAL
    if (metal_) {
        if (!tex->uploadMetal(*metal_, image.data.data(), image.width, image.height, false))
            return nullptr;
        tex->setMetalSampler(metal_->sampler(MetalContext::Filter::Linear,
                                             MetalContext::Address::ClampToEdge));
        return tex;
    }
#endif
    tex->upload(*vkCtx, image.data.data(), image.width, image.height,
                VK_FORMAT_R8G8B8A8_UNORM, false);
    tex->createSampler(vkCtx->getDevice(), VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                       VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f);
    return tex;
}

VkDescriptorSet CompositeRenderer::displayDescriptorSet() const {
#ifdef WOWEE_METAL
    if (metal_) return reinterpret_cast<VkDescriptorSet>(mtlComposite_);
#endif
    return imguiDisplaySet;
}

void CompositeRenderer::detachZoneTextures() {
    if (!zoneTextures.empty() && vkCtx) {
        // Defer destruction until all in-flight frames have completed.
        // This avoids calling vkDeviceWaitIdle mid-frame, which can trigger
        // driver TDR (GPU device lost) under heavy rendering load.
        VkDevice device = vkCtx->getDevice();
        VmaAllocator alloc = vkCtx->getAllocator();
        auto captured = std::make_shared<std::vector<std::unique_ptr<VkTexture>>>(
            std::move(zoneTextures));
        vkCtx->deferAfterAllFrameFences([device, alloc, captured]() {
            for (auto& tex : *captured) {
                if (tex) tex->destroy(device, alloc);
            }
        });
    }
    zoneTextures.clear();

    // Clear CPU-side tracking immediately so new zones get fresh loads
    for (auto& slots : zoneTextureSlots_) {
        for (auto& tex : slots.tileTextures) tex = nullptr;
        slots.tilesLoaded = false;
        for (auto& ov : slots.overlays) {
            for (auto& t : ov.tiles) t = nullptr;
            ov.tilesLoaded = false;
        }
    }
    zoneTextureSlots_.clear();
}

void CompositeRenderer::flushStaleTextures() {
    // No-op: texture cleanup is now handled by deferAfterAllFrameFences
    // in detachZoneTextures. Kept for API compatibility.
}

void CompositeRenderer::requestComposite(int zoneIdx) {
    pendingCompositeIdx_ = zoneIdx;
}

bool CompositeRenderer::hasAnyTile(int zoneIdx) const {
    if (zoneIdx < 0 || zoneIdx >= static_cast<int>(zoneTextureSlots_.size()))
        return false;
    const auto& slots = zoneTextureSlots_[zoneIdx];
    for (auto tileTexture : slots.tileTextures) {
        if (tileTexture != nullptr) return true;
    }
    return false;
}

bool CompositeRenderer::planComposite(const std::vector<Zone>& zones,
                                      const std::unordered_set<int>& exploredOverlays,
                                      bool hasServerMask, int& zoneIdx,
                                      std::vector<CompositeDraw>& draws) {
    if (!initialized || pendingCompositeIdx_ < 0) return false;
    if (pendingCompositeIdx_ >= static_cast<int>(zones.size())) {
        pendingCompositeIdx_ = -1;
        return false;
    }

    zoneIdx = pendingCompositeIdx_;
    pendingCompositeIdx_ = -1;

    if (compositedIdx_ == zoneIdx) return false;
    ensureTextureSlots(zones.size(), zones);

    const auto& zone = zones[zoneIdx];
    const auto& slots = zoneTextureSlots_[zoneIdx];
    draws.clear();

    // --- Pass 1: Draw base map tiles (opaque) ---
    for (int i = 0; i < 12; i++) {
        if (!slots.tileTextures[i] || !slots.tileTextures[i]->isValid()) continue;

        int col = i % GRID_COLS;
        int row = i / GRID_COLS;

        CompositeDraw draw{CompositeDraw::Kind::Tile, slots.tileTextures[i]};
        draw.push.gridOffset = glm::vec2(static_cast<float>(col), static_cast<float>(row));
        draw.push.gridCols = static_cast<float>(GRID_COLS);
        draw.push.gridRows = static_cast<float>(GRID_ROWS);
        draws.push_back(draw);
    }

    // --- Draw explored overlay textures on top of the base map ---
    bool hasOverlays = !zone.overlays.empty() && zone.areaID != 0;
    if (hasOverlays) {
        uint32_t descSlot = 0;
        for (int oi = 0; oi < static_cast<int>(zone.overlays.size()); oi++) {
            if (exploredOverlays.count(oi) == 0) continue;
            const auto& ov = zone.overlays[oi];
            const auto& ovSlots = slots.overlays[oi];

            for (int t = 0; t < static_cast<int>(ovSlots.tiles.size()); t++) {
                if (!ovSlots.tiles[t] || !ovSlots.tiles[t]->isValid()) continue;
                if (descSlot >= MAX_OVERLAY_TILES) break;

                int tileCol = t % ov.tileCols;
                int tileRow = t / ov.tileCols;

                // An overlay is cut into 256px pieces and the last one in each
                // direction is only as wide as the overlay has left. Drawing
                // every piece as a full cell stretched those edges up to 256,
                // which is why the explored region did not sit at the same
                // scale as the map under it.
                const int pieceW = std::min<int>(TILE_PX, ov.texWidth - tileCol * TILE_PX);
                const int pieceH = std::min<int>(TILE_PX, ov.texHeight - tileRow * TILE_PX);
                if (pieceW <= 0 || pieceH <= 0) continue;

                // And the file holding a piece is padded out to the next power
                // of two, so only part of it is the piece. Sixteen is the
                // smallest the tools emit.
                auto fileExtent = [](int pixels) {
                    int e = 16;
                    while (e < pixels) e *= 2;
                    return e;
                };

                float px = static_cast<float>(ov.offsetX + tileCol * TILE_PX);
                float py = static_cast<float>(ov.offsetY + tileRow * TILE_PX);

                CompositeDraw draw{CompositeDraw::Kind::Overlay, ovSlots.tiles[t]};
                OverlayPush& ovPush = draw.push;
                ovPush.gridOffset = glm::vec2(px / static_cast<float>(TILE_PX),
                                              py / static_cast<float>(TILE_PX));
                ovPush.gridCols = static_cast<float>(GRID_COLS);
                ovPush.gridRows = static_cast<float>(GRID_ROWS);
                ovPush.gridScale = glm::vec2(static_cast<float>(pieceW) / TILE_PX,
                                             static_cast<float>(pieceH) / TILE_PX);
                ovPush.uvScale = glm::vec2(static_cast<float>(pieceW) / fileExtent(pieceW),
                                           static_cast<float>(pieceH) / fileExtent(pieceH));
                ovPush.tintColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                draws.push_back(draw);

                descSlot++;
            }
        }
    }

    // --- Draw fog of war overlay over unexplored areas ---
    if (hasServerMask && zone.areaID != 0) {
        bool hasAnyExplored = false;
        for (int oi = 0; oi < static_cast<int>(zone.overlays.size()); oi++) {
            if (exploredOverlays.count(oi) > 0) { hasAnyExplored = true; break; }
        }
        if (!hasAnyExplored && !zone.overlays.empty()) {
            CompositeDraw draw{CompositeDraw::Kind::Fog, nullptr};
            draw.push.gridOffset = glm::vec2(0.0f, 0.0f);
            draw.push.gridCols = 1.0f;
            draw.push.gridRows = 1.0f;
            draw.push.tintColor = glm::vec4(0.15f, 0.15f, 0.2f, 0.55f);
            draws.push_back(draw);
        }
    }
    return true;
}

void CompositeRenderer::compositePass(VkCommandBuffer cmd,
                                       const std::vector<Zone>& zones,
                                       const std::unordered_set<int>& exploredOverlays,
                                       bool hasServerMask) {
    if (!compositeTarget) return;
    int zoneIdx = -1;
    std::vector<CompositeDraw> draws;
    if (!planComposite(zones, exploredOverlays, hasServerMask, zoneIdx, draws)) return;

    uint32_t frameIdx = vkCtx->getCurrentFrame();
    VkDevice device = vkCtx->getDevice();

    // Begin off-screen render pass
    VkClearColorValue clearColor = {{ 0.05f, 0.08f, 0.12f, 1.0f }};
    compositeTarget->beginPass(cmd, clearColor);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &quadVB, &offset);

    auto writeSet = [&](VkDescriptorSet set, const VkTexture& texture) {
        VkDescriptorImageInfo imgInfo = texture.descriptorInfo();
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &imgInfo;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    };

    VkPipeline bound = VK_NULL_HANDLE;
    uint32_t tileSlot = 0, overlaySlot = 0;
    for (const CompositeDraw& draw : draws) {
        const bool tile = draw.kind == CompositeDraw::Kind::Tile;
        VkPipeline pipeline = tile ? tilePipeline : overlayPipeline_;
        if (!pipeline) continue;
        if (pipeline != bound) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            bound = pipeline;
        }
        VkDescriptorSet set = VK_NULL_HANDLE;
        if (draw.kind == CompositeDraw::Kind::Fog) {
            set = fogDescSet_;
        } else if (tile) {
            if (tileSlot >= 12) continue;
            set = tileDescSets[frameIdx][tileSlot++];
            writeSet(set, *draw.texture);
        } else {
            set = overlayDescSets_[frameIdx][overlaySlot++];
            writeSet(set, *draw.texture);
        }
        if (!set) continue;
        VkPipelineLayout layout = tile ? tilePipelineLayout : overlayPipelineLayout_;
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &set,
                                0, nullptr);
        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(WorldMapTilePush), &draw.push);
        if (!tile) {
            vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_FRAGMENT_BIT,
                               offsetof(OverlayPush, tintColor), sizeof(glm::vec4),
                               &draw.push.tintColor);
        }
        vkCmdDraw(cmd, 6, 1, 0, 0);
    }

    compositeTarget->endPass(cmd);
    compositedIdx_ = zoneIdx;
    everComposited_ = true;
}

#ifdef WOWEE_METAL
bool CompositeRenderer::initializeMetal(MetalContext* ctx, pipeline::AssetManager* am) {
    if (initialized) return true;
    if (!ctx) return false;
    assetManager = am;

    const MetalBindings vert("world_map_vert");
    const MetalBindings tileFrag("world_map_frag");
    const MetalBindings fogFrag("world_map_fog_frag");
    mtlVertPush_ = vert.pushConstants();
    mtlTileTex_ = tileFrag.texture(0, 0);
    mtlTileSampler_ = tileFrag.sampler(0, 0);
    mtlFogFragPush_ = fogFrag.pushConstants();
    mtlFogTex_ = fogFrag.texture(0, 0);
    mtlFogSampler_ = fogFrag.sampler(0, 0);
    if (!vert.valid() || !tileFrag.valid() || !fogFrag.valid()) return false;

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
    desc.vertexFunction = "world_map_vert";
    desc.fragmentFunction = "world_map_frag";
    desc.colorFormat = MTL::PixelFormatRGBA8Unorm;
    desc.label = "world map tiles";
    mtlTilePipeline_ = buildMetalPipeline(*ctx, desc);
    desc.fragmentFunction = "world_map_fog_frag";
    desc.blend = MetalBlend::Alpha;
    desc.label = "world map overlays";
    mtlOverlayPipeline_ = buildMetalPipeline(*ctx, desc);
    vd->release();
    if (!mtlTilePipeline_ || !mtlOverlayPipeline_) return false;

    auto* texDesc = MTL::TextureDescriptor::texture2DDescriptor(
        MTL::PixelFormatRGBA8Unorm, FBO_W, FBO_H, false);
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
    initialized = true;
    LOG_INFO("CompositeRenderer initialized (Metal, ", FBO_W, "x", FBO_H, " composite)");
    return true;
}

void CompositeRenderer::compositeMetal(MTL::CommandBuffer* commandBuffer,
                                       const std::vector<Zone>& zones,
                                       const std::unordered_set<int>& exploredOverlays,
                                       bool hasServerMask) {
    if (!metal_ || !mtlComposite_ || !commandBuffer) return;
    int zoneIdx = -1;
    std::vector<CompositeDraw> draws;
    if (!planComposite(zones, exploredOverlays, hasServerMask, zoneIdx, draws)) return;

    auto* pass = MTL::RenderPassDescriptor::renderPassDescriptor();
    auto* color = pass->colorAttachments()->object(0);
    color->setTexture(mtlComposite_);
    color->setLoadAction(MTL::LoadActionClear);
    color->setClearColor(MTL::ClearColor::Make(0.05, 0.08, 0.12, 1.0));
    color->setStoreAction(MTL::StoreActionStore);
    MTL::RenderCommandEncoder* encoder = commandBuffer->renderCommandEncoder(pass);
    encoder->setLabel(NS::String::string("world map composite", NS::UTF8StringEncoding));
    encoder->setCullMode(MTL::CullModeNone);
    encoder->setVertexBuffer(mtlQuad_, 0, kMetalVertexBufferIndex);

    MTL::RenderPipelineState* bound = nullptr;
    for (const CompositeDraw& draw : draws) {
        const bool tile = draw.kind == CompositeDraw::Kind::Tile;
        MTL::RenderPipelineState* pipeline = tile ? mtlTilePipeline_ : mtlOverlayPipeline_;
        if (pipeline != bound) {
            encoder->setRenderPipelineState(pipeline);
            bound = pipeline;
        }
        MTL::Texture* texture = draw.texture ? draw.texture->metalTexture()
                                             : metal_->whiteTexture();
        MTL::SamplerState* sampler = draw.texture
            ? draw.texture->metalSampler()
            : metal_->sampler(MetalContext::Filter::Nearest, MetalContext::Address::ClampToEdge);
        if (!texture || !sampler) continue;
        encoder->setVertexBytes(&draw.push, sizeof(WorldMapTilePush), mtlVertPush_);
        if (tile) {
            encoder->setFragmentTexture(texture, mtlTileTex_);
            encoder->setFragmentSamplerState(sampler, mtlTileSampler_);
        } else {
            encoder->setFragmentBytes(&draw.push, sizeof(OverlayPush), mtlFogFragPush_);
            encoder->setFragmentTexture(texture, mtlFogTex_);
            encoder->setFragmentSamplerState(sampler, mtlFogSampler_);
        }
        encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(6));
    }
    encoder->endEncoding();
    compositedIdx_ = zoneIdx;
    everComposited_ = true;
}
#endif

} // namespace world_map
} // namespace rendering
} // namespace wowee
