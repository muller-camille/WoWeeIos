#include "rendering/metal/metal_pipeline.hpp"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <mutex>

#include "core/logger.hpp"
#include "rendering/metal/metal_context.hpp"
#include "rendering/vertex_layout.hpp"

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
    if (desc.blend == MetalBlend::Multiply) {
        // result = src * dst + dst: pushing (scale - 1) scales what is there.
        color->setBlendingEnabled(true);
        color->setRgbBlendOperation(MTL::BlendOperationAdd);
        color->setAlphaBlendOperation(MTL::BlendOperationAdd);
        color->setSourceRGBBlendFactor(MTL::BlendFactorDestinationColor);
        color->setDestinationRGBBlendFactor(MTL::BlendFactorOne);
        color->setSourceAlphaBlendFactor(MTL::BlendFactorZero);
        color->setDestinationAlphaBlendFactor(MTL::BlendFactorOne);
    } else if (desc.blend != MetalBlend::None) {
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

MTL::ComputePipelineState* buildMetalComputePipeline(MetalContext& ctx, const char* kernel) {
    MTL::Library* library = ctx.getLibrary();
    if (!library) {
        LOG_ERROR("Metal: no default.metallib to build ", kernel, " from");
        return nullptr;
    }
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
    MTL::Function* fn = library->newFunction(NS::String::string(kernel, NS::UTF8StringEncoding));
    MTL::ComputePipelineState* state = nullptr;
    if (!fn) {
        LOG_ERROR("Metal: default.metallib has no ", kernel);
    } else {
        NS::Error* error = nullptr;
        state = ctx.getDevice()->newComputePipelineState(fn, &error);
        if (!state) {
            LOG_ERROR("Metal: kernel ", kernel, " refused: ",
                      error ? error->localizedDescription()->utf8String() : "no reason given");
        }
        fn->release();
    }
    pool->release();
    return state;
}

MTL::VertexDescriptor* newPackedFloatVertexDescriptor(uint32_t extraFloats) {
    auto* vd = MTL::VertexDescriptor::alloc()->init();
    vd->attributes()->object(0)->setFormat(MTL::VertexFormatFloat3);
    vd->attributes()->object(0)->setOffset(0);
    vd->attributes()->object(0)->setBufferIndex(kMetalVertexBufferIndex);
    for (uint32_t i = 0; i < extraFloats; ++i) {
        auto* attr = vd->attributes()->object(1 + i);
        attr->setFormat(MTL::VertexFormatFloat);
        attr->setOffset((3 + i) * sizeof(float));
        attr->setBufferIndex(kMetalVertexBufferIndex);
    }
    vd->layouts()->object(kMetalVertexBufferIndex)->setStride((3 + extraFloats) * sizeof(float));
    return vd;
}

bool MetalVertexRing::create(MetalContext& ctx, size_t capacityBytes) {
    release();
    static_assert(sizeof(buffers_) / sizeof(buffers_[0]) == MetalContext::kRingSize,
                  "one buffer per frame the ring spans");
    for (auto*& buffer : buffers_) {
        buffer = ctx.newBuffer(nullptr, capacityBytes);
        if (!buffer) {
            release();
            return false;
        }
    }
    capacity_ = capacityBytes;
    return true;
}

void MetalVertexRing::release() {
    for (auto*& buffer : buffers_) {
        if (buffer) { buffer->release(); buffer = nullptr; }
    }
    capacity_ = 0;
}

MTL::Buffer* MetalVertexRing::write(const MetalContext& ctx, const void* data, size_t bytes,
                                    size_t* written) {
    if (written) *written = 0;
    if (!buffers_[0]) return nullptr;
    MTL::Buffer* buffer = buffers_[ctx.frameNumber() % MetalContext::kRingSize];
    const size_t n = std::min(bytes, capacity_);
    if (n && data) std::memcpy(buffer->contents(), data, n);
    if (written) *written = n;
    return buffer;
}

bool MetalShadowSlots::load() {
    const MetalBindings vert("shadow_vert");
    const MetalBindings frag("shadow_frag");
    vertPush = vert.pushConstants();
    fragPush = frag.pushConstants();
    fragTexture = frag.texture(0, 0);
    fragSampler = frag.sampler(0, 0);
    return vert.valid() && frag.valid();
}

MTL::RenderPipelineState* buildMetalShadowPipeline(MetalContext& ctx, const VertexAttribute* attrs,
                                                   size_t count, uint32_t stride,
                                                   const char* label) {
    auto* vd = MTL::VertexDescriptor::alloc()->init();
    for (size_t i = 0; i < count; ++i) {
        const VertexAttribute& a = attrs[i];
        if (a.location > 1) continue;  // the bone inputs shadow.vert no longer has
        auto* attr = vd->attributes()->object(a.location);
        attr->setFormat(a.componentCount == 2   ? MTL::VertexFormatFloat2
                        : a.componentCount == 3 ? MTL::VertexFormatFloat3
                                                : MTL::VertexFormatFloat4);
        attr->setOffset(a.offset);
        attr->setBufferIndex(kMetalVertexBufferIndex);
    }
    vd->layouts()->object(kMetalVertexBufferIndex)->setStride(stride);
    MetalPipelineDesc desc;
    desc.vertexFunction = "shadow_vert";
    desc.fragmentFunction = "shadow_frag";
    desc.vertexDescriptor = vd;
    desc.colorFormat = 0;  // depth only
    desc.depthFormat = kMetalShadowDepthFormat;
    desc.label = label;
    MTL::RenderPipelineState* pipeline = buildMetalPipeline(ctx, desc);
    vd->release();
    return pipeline;
}

void beginMetalShadowDraws(MetalContext& ctx, MTL::RenderCommandEncoder* encoder,
                           MTL::RenderPipelineState* pipeline, const MetalShadowSlots& slots) {
    encoder->setRenderPipelineState(pipeline);
    encoder->setDepthStencilState(ctx.depthState(true, true, /*lessEqual=*/true));
    // buildShadowPipeline's setDepthBias(0.05, 0.20).
    encoder->setDepthBias(0.05f, 0.20f, 0.0f);
    encoder->setCullMode(MTL::CullModeNone);
    encoder->setFragmentTexture(ctx.whiteTexture(), slots.fragTexture);
    encoder->setFragmentSamplerState(ctx.sampler(MetalContext::Filter::Linear,
                                                 MetalContext::Address::ClampToEdge),
                                     slots.fragSampler);
}

}  // namespace rendering
}  // namespace wowee
