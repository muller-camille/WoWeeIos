// corpse_marker_layer.hpp - Death corpse marker on the world map.
#pragma once
#include "rendering/world_map/overlay_renderer.hpp"
#include "rendering/imgui_texture.hpp"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <memory>

namespace wowee {
namespace rendering {
class VkContext;
class MetalContext;
}
namespace pipeline { class AssetManager; }
namespace rendering {
namespace world_map {

class CorpseMarkerLayer : public IOverlayLayer {
public:
    ~CorpseMarkerLayer() override;
    void initialize(VkContext* ctx, pipeline::AssetManager* am);
#ifdef WOWEE_METAL
    void initializeMetal(MetalContext* ctx, pipeline::AssetManager* am) {
        metal_ = ctx;
        assetManager_ = am;
    }
#endif
    void clearTexture();
    void setCorpse(bool hasCorpse, glm::vec3 renderPos) {
        hasCorpse_ = hasCorpse;
        corpseRenderPos_ = renderPos;
    }
    /// Where a release would put the player - the nearest spirit healer, as
    /// the server names it in SMSG_DEATH_RELEASE_LOC. Drawn beside the corpse
    /// because the two together are the whole of a corpse run: where the body
    /// is, and where the alternative is.
    void setGraveyard(bool hasGraveyard, glm::vec3 renderPos) {
        hasGraveyard_ = hasGraveyard;
        graveyardRenderPos_ = renderPos;
    }
    void render(const LayerContext& ctx) override;
private:
    void ensureTexture();

    VkContext* vkCtx_ = nullptr;
    MetalContext* metal_ = nullptr;  // only ever set on the Metal build
    pipeline::AssetManager* assetManager_ = nullptr;
    ImGuiTexture marker_;
    bool loadAttempted_ = false;
    bool hasCorpse_ = false;
    glm::vec3 corpseRenderPos_ = {};
    bool hasGraveyard_ = false;
    glm::vec3 graveyardRenderPos_ = {};
};

} // namespace world_map
} // namespace rendering
} // namespace wowee
