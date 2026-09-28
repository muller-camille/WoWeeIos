#include "rendering/metal/metal_pipeline.hpp"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <mutex>

#include "core/logger.hpp"
#include "rendering/metal/metal_context.hpp"

namespace wowee {
namespace rendering {

namespace {

/// The manifest, read once. Relative to the bundle root, which is the working
/// directory on iOS, as every other asset is.
const nlohmann::json& manifest() {
    static nlohmann::json loaded;
    static std::once_flag once;
    std::call_once(once, [] {
        constexpr const char* kPath = "assets/shaders/metal/manifest.json";
        std::ifstream in(kPath);
        if (!in) {
            LOG_ERROR("Metal: ", kPath, " is missing - no shader binding can be found");
            loaded = nlohmann::json::object();
            return;
        }
        try {
            in >> loaded;
        } catch (const std::exception& e) {
            LOG_ERROR("Metal: ", kPath, " could not be read: ", e.what());
            loaded = nlohmann::json::object();
        }
    });
    return loaded;
}

}  // namespace

MetalBindings::MetalBindings(const std::string& function) : function_(function) {
    // Found in place rather than through value(), which copies: entry_ points
    // into the manifest, which lives as long as the program.
    const nlohmann::json& all = manifest();
    auto functions = all.find("functions");
    if (functions == all.end() || functions->find(function) == functions->end()) {
        LOG_ERROR("Metal: ", function, " is not in the shader manifest");
        valid_ = false;
        return;
    }
    entry_ = &*functions->find(function);
}

int MetalBindings::lookup(uint32_t set, uint32_t binding, const char* key) const {
    if (!entry_) return -1;
    const auto& entry = *static_cast<const nlohmann::json*>(entry_);
    for (const auto& b : entry.value("bindings", nlohmann::json::array())) {
        if (b.value("set", -1) == static_cast<int>(set) &&
            b.value("binding", -1) == static_cast<int>(binding) && b.contains(key)) {
            return b[key].get<int>();
        }
    }
    LOG_ERROR("Metal: ", function_, " has no ", key, " for set ", set, " binding ", binding);
    valid_ = false;
    return -1;
}

int MetalBindings::buffer(uint32_t set, uint32_t binding) const {
    return lookup(set, binding, "buffer");
}

int MetalBindings::texture(uint32_t set, uint32_t binding) const {
    return lookup(set, binding, "texture");
}

int MetalBindings::sampler(uint32_t set, uint32_t binding) const {
    return lookup(set, binding, "sampler");
}

int MetalBindings::pushConstants() const {
    if (entry_) {
        const auto& entry = *static_cast<const nlohmann::json*>(entry_);
        if (auto it = entry.find("push_constants"); it != entry.end()) {
            return (*it).value("buffer", -1);
        }
    }
    LOG_ERROR("Metal: ", function_, " has no push constants");
    valid_ = false;
    return -1;
}

MTL::RenderPipelineState* buildMetalPipeline(MetalContext& ctx, const MetalPipelineDesc& desc) {
    MTL::Library* library = ctx.getLibrary();
    if (!library) {
        LOG_ERROR("Metal: no default.metallib to build ", desc.label ? desc.label : "a pipeline",
                  " from");
        return nullptr;
    }

    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
    MTL::Function* vertexFn = library->newFunction(
        NS::String::string(desc.vertexFunction, NS::UTF8StringEncoding));
    MTL::Function* fragmentFn = desc.fragmentFunction
        ? library->newFunction(NS::String::string(desc.fragmentFunction, NS::UTF8StringEncoding))
        : nullptr;
    if (!vertexFn || (desc.fragmentFunction && !fragmentFn)) {
        LOG_ERROR("Metal: default.metallib has no ",
                  !vertexFn ? desc.vertexFunction : desc.fragmentFunction);
        if (vertexFn) vertexFn->release();
        if (fragmentFn) fragmentFn->release();
        pool->release();
        return nullptr;
    }

    auto* pd = MTL::RenderPipelineDescriptor::alloc()->init();
    if (desc.label) pd->setLabel(NS::String::string(desc.label, NS::UTF8StringEncoding));
    pd->setVertexFunction(vertexFn);
    pd->setFragmentFunction(fragmentFn);
    pd->setVertexDescriptor(desc.vertexDescriptor);
    pd->setRasterSampleCount(desc.sampleCount);
    pd->setAlphaToCoverageEnabled(desc.alphaToCoverage);
    if (desc.depthFormat != 0) {
        pd->setDepthAttachmentPixelFormat(static_cast<MTL::PixelFormat>(desc.depthFormat));
    }

    auto* color = pd->colorAttachments()->object(0);
    color->setPixelFormat(static_cast<MTL::PixelFormat>(desc.colorFormat));
    if (desc.blend != MetalBlend::None) {
        color->setBlendingEnabled(true);
        color->setRgbBlendOperation(MTL::BlendOperationAdd);
        color->setAlphaBlendOperation(MTL::BlendOperationAdd);
        color->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
        color->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
        if (desc.blend == MetalBlend::Alpha) {
            color->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
            color->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
        } else {
            color->setDestinationRGBBlendFactor(MTL::BlendFactorOne);
            color->setDestinationAlphaBlendFactor(MTL::BlendFactorOne);
        }
    }

    NS::Error* error = nullptr;
    MTL::RenderPipelineState* state = ctx.getDevice()->newRenderPipelineState(pd, &error);
    if (!state) {
        LOG_ERROR("Metal: pipeline ", desc.label ? desc.label : desc.vertexFunction,
                  " refused: ",
                  error ? error->localizedDescription()->utf8String() : "no reason given");
    }
    pd->release();
    vertexFn->release();
    if (fragmentFn) fragmentFn->release();
    pool->release();
    return state;
}

}  // namespace rendering
}  // namespace wowee
