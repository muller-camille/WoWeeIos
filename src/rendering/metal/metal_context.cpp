#include "rendering/metal/metal_context.hpp"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>

#include "core/logger.hpp"
#include "stb_image_write.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace wowee {
namespace rendering {

MetalContext::~MetalContext() {
    shutdown();
}

bool MetalContext::initialize(SDL_Window* window) {
    window_ = window;

    device_ = MTL::CreateSystemDefaultDevice();
    if (!device_) {
        LOG_ERROR("Metal: no device");
        return false;
    }
    queue_ = device_->newCommandQueue();
    if (!queue_) {
        LOG_ERROR("Metal: could not make a command queue");
        return false;
    }

    // SDL makes the view, and with it the layer; this only configures it.
    view_ = SDL_Metal_CreateView(window);
    if (!view_) {
        LOG_ERROR("Metal: SDL_Metal_CreateView failed: ", SDL_GetError());
        return false;
    }
    layer_ = static_cast<CA::MetalLayer*>(SDL_Metal_GetLayer(view_));
    if (!layer_) {
        LOG_ERROR("Metal: the view has no CAMetalLayer");
        return false;
    }
    layer_->setDevice(device_);
    // The format the Vulkan swapchain asks for, B8G8R8A8_UNORM, so the
    // interface's colours come out the same in both builds.
    layer_->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    // Read back only when a screenshot was asked for: a framebuffer-only
    // drawable is cheaper to render to and present.
    readableDrawables_ = std::getenv("WOWEE_SCREENSHOT") != nullptr;
    layer_->setFramebufferOnly(!readableDrawables_);
    refreshDrawableSize();

    // Missing is not fatal in M1, where nothing draws with it: the interface
    // brings its own shaders through ImGui's backend.
    library_ = device_->newDefaultLibrary();
    if (library_) {
        LOG_INFO("Metal: default.metallib has ",
                 library_->functionNames()->count(), " functions");
    } else {
        LOG_WARNING("Metal: no default.metallib in the bundle");
    }

    frameSlots_ = dispatch_semaphore_create(kFramesInFlight);

    // The stand-ins every pass can bind where the shader declares something
    // the pass has none of.
    {
        const uint8_t whitePixel[4] = {255, 255, 255, 255};
        white_ = uploadTexture(whitePixel, 1, 1);

        auto* volume = MTL::TextureDescriptor::alloc()->init();
        volume->setTextureType(MTL::TextureType3D);
        volume->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
        volume->setWidth(1);
        volume->setHeight(1);
        volume->setDepth(1);
        volume->setUsage(MTL::TextureUsageShaderRead);
        volume->setStorageMode(MTL::StorageModeShared);
        neutralVolume_ = device_->newTexture(volume);
        volume->release();
        // Nothing scattered, everything let through.
        const uint8_t clearPixel[4] = {0, 0, 0, 255};
        if (neutralVolume_) {
            neutralVolume_->replaceRegion(MTL::Region::Make3D(0, 0, 0, 1, 1, 1), 0, 0,
                                          clearPixel, 4, 4);
        }

        auto* depth = MTL::TextureDescriptor::texture2DDescriptor(
            MTL::PixelFormatDepth32Float, 1, 1, false);
        depth->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageRenderTarget);
        depth->setStorageMode(MTL::StorageModePrivate);
        neutralDepth_ = device_->newTexture(depth);
        if (neutralDepth_) {
            // Far everywhere, so a comparison against it never shadows.
            NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
            auto* pass = MTL::RenderPassDescriptor::renderPassDescriptor();
            pass->depthAttachment()->setTexture(neutralDepth_);
            pass->depthAttachment()->setLoadAction(MTL::LoadActionClear);
            pass->depthAttachment()->setClearDepth(1.0);
            pass->depthAttachment()->setStoreAction(MTL::StoreActionStore);
            MTL::CommandBuffer* cmd = queue_->commandBuffer();
            cmd->renderCommandEncoder(pass)->endEncoding();
            cmd->commit();
            pool->release();
        }
    }

    LOG_INFO("Metal: ", device_->name()->utf8String(), ", drawable ",
             drawableWidth_, "x", drawableHeight_);
    return true;
}

void MetalContext::shutdown() {
    if (queue_ && frameSlots_) {
        // Everything in flight finishes before what it uses goes away.
        for (int i = 0; i < kFramesInFlight; ++i) {
            dispatch_semaphore_wait(frameSlots_, DISPATCH_TIME_FOREVER);
        }
        for (int i = 0; i < kFramesInFlight; ++i) {
            dispatch_semaphore_signal(frameSlots_);
        }
    }
    for (auto& [key, state] : samplers_) state->release();
    samplers_.clear();
    if (shadowSampler_) { shadowSampler_->release(); shadowSampler_ = nullptr; }
    for (auto*& state : depthStates_) {
        if (state) { state->release(); state = nullptr; }
    }
    for (MTL::Texture*& texture : interfaceTextures_) releaseTexture(texture);
    interfaceTextures_.clear();
    releaseTexture(white_);
    releaseTexture(neutralDepth_);
    releaseTexture(neutralVolume_);
    if (library_) { library_->release(); library_ = nullptr; }
    if (queue_) { queue_->release(); queue_ = nullptr; }
    if (view_) { SDL_Metal_DestroyView(view_); view_ = nullptr; layer_ = nullptr; }
    if (device_) { device_->release(); device_ = nullptr; }
    if (frameSlots_) { dispatch_release(frameSlots_); frameSlots_ = nullptr; }
}

void MetalContext::refreshDrawableSize() {
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(window_, &w, &h);
    if (w <= 0 || h <= 0) return;
    if (static_cast<uint32_t>(w) == drawableWidth_ &&
        static_cast<uint32_t>(h) == drawableHeight_) {
        return;
    }
    drawableWidth_ = static_cast<uint32_t>(w);
    drawableHeight_ = static_cast<uint32_t>(h);
    layer_->setDrawableSize(CGSizeMake(w, h));
}

bool MetalContext::beginFrame() {
    if (presentationPaused_ || !layer_) return false;

    // The slot of the frame before last: its command buffer is the one whose
    // completion hands it back.
    dispatch_semaphore_wait(frameSlots_, DISPATCH_TIME_FOREVER);

    // The drawable, the pass descriptor and the command buffer are all
    // autoreleased, and nothing else drains a pool once a frame on iOS.
    framePool_ = NS::AutoreleasePool::alloc()->init();

    refreshDrawableSize();
    drawable_ = layer_->nextDrawable();
    if (!drawable_) {
        framePool_->release();
        framePool_ = nullptr;
        dispatch_semaphore_signal(frameSlots_);
        return false;
    }

    ++frameNumber_;
    commandBuffer_ = queue_->commandBuffer();
    dispatch_semaphore_t slots = frameSlots_;
    commandBuffer_->addCompletedHandler([slots](MTL::CommandBuffer*) {
        dispatch_semaphore_signal(slots);
    });

    renderPass_ = MTL::RenderPassDescriptor::renderPassDescriptor();
    auto* color = renderPass_->colorAttachments()->object(0);
    color->setTexture(drawable_->texture());
    color->setLoadAction(MTL::LoadActionClear);
    color->setClearColor(MTL::ClearColor::Make(0.0, 0.0, 0.0, 1.0));
    color->setStoreAction(MTL::StoreActionStore);
    return true;
}

void MetalContext::endFrame() {
    if (!commandBuffer_) return;

    MTL::Buffer* capture = nullptr;
    const uint32_t captureW = drawableWidth_;
    const uint32_t captureH = drawableHeight_;
    if (!capturePath_.empty() && readableDrawables_) {
        capture = device_->newBuffer(static_cast<NS::UInteger>(captureW) * captureH * 4,
                                     MTL::ResourceStorageModeShared);
        MTL::BlitCommandEncoder* blit = commandBuffer_->blitCommandEncoder();
        blit->copyFromTexture(drawable_->texture(), 0, 0, MTL::Origin::Make(0, 0, 0),
                              MTL::Size::Make(captureW, captureH, 1), capture, 0,
                              static_cast<NS::UInteger>(captureW) * 4,
                              static_cast<NS::UInteger>(captureW) * captureH * 4);
        blit->endEncoding();
    }

    commandBuffer_->presentDrawable(drawable_);
    commandBuffer_->commit();

    if (capture) {
        commandBuffer_->waitUntilCompleted();
        // BGRA on the drawable, RGBA in a PNG.
        auto* px = static_cast<uint8_t*>(capture->contents());
        std::vector<uint8_t> rgba(px, px + static_cast<size_t>(captureW) * captureH * 4);
        for (size_t i = 0; i < rgba.size(); i += 4) {
            std::swap(rgba[i], rgba[i + 2]);
            rgba[i + 3] = 255;
        }
        if (stbi_write_png(capturePath_.c_str(), static_cast<int>(captureW),
                           static_cast<int>(captureH), 4, rgba.data(),
                           static_cast<int>(captureW) * 4)) {
            LOG_WARNING("Screenshot written: ", capturePath_);
        } else {
            LOG_WARNING("Screenshot: could not write ", capturePath_);
        }
        capture->release();
        capturePath_.clear();
    }
    commandBuffer_ = nullptr;
    renderPass_ = nullptr;
    drawable_ = nullptr;
    framePool_->release();
    framePool_ = nullptr;
}

void MetalContext::pausePresentation() {
    presentationPaused_ = true;
}

void MetalContext::resumePresentation() {
    presentationPaused_ = false;
}

MTL::Texture* MetalContext::uploadTexture(const uint8_t* rgba, uint32_t width, uint32_t height,
                                          bool mipmaps) {
    if (!device_ || !rgba || width == 0 || height == 0) return nullptr;
    auto* desc = MTL::TextureDescriptor::texture2DDescriptor(
        MTL::PixelFormatRGBA8Unorm, width, height, mipmaps);
    desc->setUsage(MTL::TextureUsageShaderRead);
    desc->setStorageMode(MTL::StorageModePrivate);
    MTL::Texture* texture = device_->newTexture(desc);
    if (!texture) {
        LOG_ERROR("Metal: could not make a ", width, "x", height, " texture");
        return nullptr;
    }

    // Through a shared staging buffer and a blit, on the queue every draw is
    // on: whatever samples it is committed after this, so it sees it filled.
    const size_t rowBytes = static_cast<size_t>(width) * 4;
    MTL::Buffer* staging = newBuffer(rgba, rowBytes * height);
    if (!staging) {
        texture->release();
        return nullptr;
    }
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
    MTL::CommandBuffer* cmd = queue_->commandBuffer();
    MTL::BlitCommandEncoder* blit = cmd->blitCommandEncoder();
    blit->copyFromBuffer(staging, 0, rowBytes, rowBytes * height,
                         MTL::Size::Make(width, height, 1), texture, 0, 0,
                         MTL::Origin::Make(0, 0, 0));
    if (mipmaps && texture->mipmapLevelCount() > 1) {
        blit->generateMipmaps(texture);
    }
    blit->endEncoding();
    cmd->commit();
    pool->release();
    // The command buffer holds it until the copy is done.
    staging->release();
    return texture;
}

MTL::Texture* MetalContext::uploadInterfaceTexture(const uint8_t* rgba, uint32_t width,
                                                   uint32_t height) {
    MTL::Texture* texture = uploadTexture(rgba, width, height);
    if (texture) interfaceTextures_.push_back(texture);
    return texture;
}

MTL::Buffer* MetalContext::newBuffer(const void* data, size_t bytes) {
    if (!device_ || bytes == 0) return nullptr;
    MTL::Buffer* buffer = data
        ? device_->newBuffer(data, bytes, MTL::ResourceStorageModeShared)
        : device_->newBuffer(bytes, MTL::ResourceStorageModeShared);
    if (!buffer) LOG_ERROR("Metal: could not make a ", bytes, "-byte buffer");
    return buffer;
}

MTL::SamplerState* MetalContext::sampler(Filter filter, Address address) {
    const uint16_t key = static_cast<uint16_t>((static_cast<uint16_t>(filter) << 8) |
                                               static_cast<uint16_t>(address));
    if (auto it = samplers_.find(key); it != samplers_.end()) return it->second;

    auto* desc = MTL::SamplerDescriptor::alloc()->init();
    const bool linear = filter == Filter::Linear;
    desc->setMinFilter(linear ? MTL::SamplerMinMagFilterLinear : MTL::SamplerMinMagFilterNearest);
    desc->setMagFilter(linear ? MTL::SamplerMinMagFilterLinear : MTL::SamplerMinMagFilterNearest);
    desc->setMipFilter(linear ? MTL::SamplerMipFilterLinear : MTL::SamplerMipFilterNotMipmapped);
    const auto mode = address == Address::Repeat ? MTL::SamplerAddressModeRepeat
                                                 : MTL::SamplerAddressModeClampToEdge;
    desc->setSAddressMode(mode);
    desc->setTAddressMode(mode);
    desc->setRAddressMode(mode);
    if (linear) desc->setMaxAnisotropy(4);
    MTL::SamplerState* state = device_->newSamplerState(desc);
    desc->release();
    samplers_[key] = state;
    return state;
}

MTL::SamplerState* MetalContext::shadowSampler() {
    if (shadowSampler_) return shadowSampler_;
    auto* desc = MTL::SamplerDescriptor::alloc()->init();
    desc->setMinFilter(MTL::SamplerMinMagFilterLinear);
    desc->setMagFilter(MTL::SamplerMinMagFilterLinear);
    desc->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
    desc->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
    desc->setCompareFunction(MTL::CompareFunctionLessEqual);
    shadowSampler_ = device_->newSamplerState(desc);
    desc->release();
    return shadowSampler_;
}

MTL::DepthStencilState* MetalContext::depthState(bool test, bool write) {
    const int index = (test ? 2 : 0) | (write ? 1 : 0);
    if (depthStates_[index]) return depthStates_[index];
    auto* desc = MTL::DepthStencilDescriptor::alloc()->init();
    desc->setDepthCompareFunction(test ? MTL::CompareFunctionLess : MTL::CompareFunctionAlways);
    desc->setDepthWriteEnabled(write);
    depthStates_[index] = device_->newDepthStencilState(desc);
    desc->release();
    return depthStates_[index];
}

void MetalContext::releaseTexture(MTL::Texture*& texture) {
    if (!texture) return;
    // A frame still in flight may sample it; the command buffer retains what
    // its encoders used, so dropping this reference is enough.
    texture->release();
    texture = nullptr;
}

}  // namespace rendering
}  // namespace wowee
