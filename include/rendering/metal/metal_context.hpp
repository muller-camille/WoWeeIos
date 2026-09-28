#pragma once

#include <cstdint>
#include <dispatch/dispatch.h>
#include <string>
#include <unordered_map>
#include <vector>

struct SDL_Window;

namespace MTL {
class Device;
class CommandQueue;
class Library;
class CommandBuffer;
class RenderPassDescriptor;
class Texture;
class Buffer;
class SamplerState;
class DepthStencilState;
}  // namespace MTL
namespace CA {
class MetalLayer;
class MetalDrawable;
}  // namespace CA
namespace NS {
class AutoreleasePool;
}  // namespace NS

namespace wowee {
namespace rendering {

/// The device, the queue and the layer the Metal renderer draws into, and the
/// pacing of its frames. What VkContext is to the Vulkan build, and the start
/// of its replacement (docs/plan-metal.md, M1).
///
/// A frame is beginFrame(), encoders on commandBuffer() - the first render
/// pass through renderPass(), which clears the drawable - and endFrame(), which
/// presents it. Two frames are in flight at most; beginFrame waits for the
/// older one to finish on the GPU before it reuses anything of its.
class MetalContext {
public:
    MetalContext() = default;
    ~MetalContext();

    MetalContext(const MetalContext&) = delete;
    MetalContext& operator=(const MetalContext&) = delete;

    [[nodiscard]] bool initialize(SDL_Window* window);
    void shutdown();

    [[nodiscard]] MTL::Device* getDevice() const { return device_; }
    [[nodiscard]] MTL::CommandQueue* getQueue() const { return queue_; }
    /// default.metallib, the shaders tools/metal/convert_shaders.py translated.
    /// Null if the app was built without it; nothing in M1 draws with it yet.
    [[nodiscard]] MTL::Library* getLibrary() const { return library_; }

    /// False when there is nothing to draw into this frame: presentation is
    /// paused, or no drawable came back in time. Nothing is to be encoded then,
    /// and endFrame is not to be called.
    [[nodiscard]] bool beginFrame();
    [[nodiscard]] MTL::CommandBuffer* commandBuffer() const { return commandBuffer_; }
    /// The pass onto the drawable, cleared to black. Valid between beginFrame
    /// and endFrame.
    [[nodiscard]] MTL::RenderPassDescriptor* renderPass() const { return renderPass_; }
    void endFrame();

    /// iOS takes the right to use the GPU away from an app in the background,
    /// and kills one that submits work anyway. The same switch the MoltenVK
    /// build has: nothing is released, frames are skipped until it is resumed.
    void pausePresentation();
    void resumePresentation();
    [[nodiscard]] bool isPresentationPaused() const { return presentationPaused_; }

    /// An RGBA8 picture as a texture: the interface draws it as the ImTextureID
    /// ImGui's Metal backend expects, which is the MTL::Texture itself; the
    /// renderers sample it. Private, filled by a blit on the queue and, with
    /// mipmaps, its chain generated there too, so it is ready for any command
    /// buffer committed after this returns. Retained; give it back with
    /// releaseTexture.
    [[nodiscard]] MTL::Texture* uploadTexture(const uint8_t* rgba, uint32_t width, uint32_t height,
                                              bool mipmaps = false);
    void releaseTexture(MTL::Texture*& texture);

    /// An interface texture that lives as long as this context, as those from
    /// VkContext::uploadImGuiTexture do: not the caller's to release.
    [[nodiscard]] MTL::Texture* uploadInterfaceTexture(const uint8_t* rgba, uint32_t width,
                                                       uint32_t height);

    /// A shared buffer, filled from data when it is given. Retained.
    [[nodiscard]] MTL::Buffer* newBuffer(const void* data, size_t bytes);

    /// What a Vulkan sampler asked for, as far as the renderers ask for it.
    enum class Filter : uint8_t { Nearest, Linear };
    enum class Address : uint8_t { Repeat, ClampToEdge };
    /// Made once per combination and kept for the context's life; not the
    /// caller's to release. Linear samplers filter between mip levels too.
    [[nodiscard]] MTL::SamplerState* sampler(Filter filter, Address address);
    /// Compares against the reference, for sampler2DShadow.
    [[nodiscard]] MTL::SamplerState* shadowSampler();
    /// Depth test less-than, writing or not. Kept for the context's life.
    [[nodiscard]] MTL::DepthStencilState* depthState(bool test, bool write);

    /// Stand-ins for a binding a pass does not use but the shader declares:
    /// 1x1 white, a 1x1 depth texture at 1.0, a 1x1x1 volume.
    [[nodiscard]] MTL::Texture* whiteTexture() const { return white_; }
    [[nodiscard]] MTL::Texture* neutralDepthTexture() const { return neutralDepth_; }
    [[nodiscard]] MTL::Texture* neutralVolumeTexture() const { return neutralVolume_; }

    /// Counts beginFrame calls. With two frames in flight, anything the CPU
    /// writes per frame and the GPU reads is safe in a ring of three indexed
    /// by this.
    [[nodiscard]] uint64_t frameNumber() const { return frameNumber_; }
    static constexpr uint32_t kRingSize = 3;

    /// Write the next frame endFrame presents to a PNG at path, and wait for
    /// it. How the Mac sees what the device shows (WOWEE_SCREENSHOT). Only
    /// possible when WOWEE_SCREENSHOT was set at initialize: the drawables are
    /// otherwise framebuffer-only and cannot be read back.
    void captureNextFrame(const std::string& path) { capturePath_ = path; }

    [[nodiscard]] uint32_t drawableWidth() const { return drawableWidth_; }
    [[nodiscard]] uint32_t drawableHeight() const { return drawableHeight_; }

private:
    /// The layer's drawables follow the window's size in pixels, read each
    /// frame rather than told: SDL is the only one who knows it changed.
    void refreshDrawableSize();

    static constexpr int kFramesInFlight = 2;

    SDL_Window* window_ = nullptr;
    void* view_ = nullptr;  // SDL_MetalView
    MTL::Device* device_ = nullptr;
    MTL::CommandQueue* queue_ = nullptr;
    MTL::Library* library_ = nullptr;
    CA::MetalLayer* layer_ = nullptr;

    dispatch_semaphore_t frameSlots_ = nullptr;
    NS::AutoreleasePool* framePool_ = nullptr;
    CA::MetalDrawable* drawable_ = nullptr;
    MTL::CommandBuffer* commandBuffer_ = nullptr;
    MTL::RenderPassDescriptor* renderPass_ = nullptr;

    uint32_t drawableWidth_ = 0;
    uint32_t drawableHeight_ = 0;
    bool presentationPaused_ = false;
    bool readableDrawables_ = false;
    uint64_t frameNumber_ = 0;
    std::unordered_map<uint16_t, MTL::SamplerState*> samplers_;
    MTL::SamplerState* shadowSampler_ = nullptr;
    MTL::DepthStencilState* depthStates_[4] = {};
    MTL::Texture* white_ = nullptr;
    MTL::Texture* neutralDepth_ = nullptr;
    MTL::Texture* neutralVolume_ = nullptr;
    std::vector<MTL::Texture*> interfaceTextures_;
    std::string capturePath_;
};

}  // namespace rendering
}  // namespace wowee
