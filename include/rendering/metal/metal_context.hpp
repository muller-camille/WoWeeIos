#pragma once

#include <atomic>
#include <mutex>
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
    /// format and bytesPerPixel say what the pixels are when they are not
    /// RGBA8: the terrain's alpha masks are one byte a texel (R8Unorm).
    [[nodiscard]] MTL::Texture* uploadTexture(const uint8_t* rgba, uint32_t width, uint32_t height,
                                              bool mipmaps = false, uint32_t format = 70,
                                              uint32_t bytesPerPixel = 4);
    void releaseTexture(MTL::Texture*& texture);

    /// A block-compressed texture from its levels, largest first, as a BLP
    /// keeps them. format is an MTL::PixelFormat (BC1, BC2 or BC3); blockBytes
    /// is 8 for BC1 and 16 for the others. Null when a level is short or the
    /// device refuses the format. Retained; give it back with releaseTexture.
    [[nodiscard]] MTL::Texture* uploadCompressedTexture(uint32_t format, uint32_t width,
                                                        uint32_t height, uint32_t blockBytes,
                                                        const std::vector<std::vector<uint8_t>>& levels);
    /// Whether the GPU samples BC1-BC3. Where it does not, a BLP is decoded.
    [[nodiscard]] bool supportsBC() const { return supportsBC_; }

    /// An interface texture that lives as long as this context, as those from
    /// VkContext::uploadImGuiTexture do: not the caller's to release.
    [[nodiscard]] MTL::Texture* uploadInterfaceTexture(const uint8_t* rgba, uint32_t width,
                                                       uint32_t height);

    /// A shared buffer, filled from data when it is given. Retained.
    [[nodiscard]] MTL::Buffer* newBuffer(const void* data, size_t bytes);
    /// A private buffer, for the GPU alone: fill it with uploadToBuffer.
    /// Retained.
    [[nodiscard]] MTL::Buffer* newPrivateBuffer(size_t bytes);
    /// bytes of data into target at offset, through a shared staging buffer
    /// and a blit on the queue, as uploadTexture fills a texture: a command
    /// buffer committed after this sees it written, and those committed
    /// before - frames still in flight - read what was there. False when
    /// there is nothing to write or it does not fit.
    bool uploadToBuffer(MTL::Buffer* target, const void* data, size_t bytes, size_t offset = 0);

    /// What a Vulkan sampler asked for, as far as the renderers ask for it.
    enum class Filter : uint8_t { Nearest, Linear };
    enum class Address : uint8_t { Repeat, ClampToEdge };
    /// Made once per combination and kept for the context's life; not the
    /// caller's to release. Linear samplers filter between mip levels too.
    [[nodiscard]] MTL::SamplerState* sampler(Filter filter, Address address);
    /// Compares against the reference, for sampler2DShadow.
    [[nodiscard]] MTL::SamplerState* shadowSampler();
    /// Depth test less-than - or less-or-equal, as the M2 pipelines ask for -
    /// writing or not. Kept for the context's life.
    [[nodiscard]] MTL::DepthStencilState* depthState(bool test, bool write, bool lessEqual = false);

    /// Stand-ins for a binding a pass does not use but the shader declares:
    /// 1x1 white, a 1x1 depth texture at 1.0, a 1x1x1 volume.
    [[nodiscard]] MTL::Texture* whiteTexture() const { return white_; }
    /// 1x1 black, for a colour a shader weighs by its own brightness - the
    /// water's reflection - where black means none.
    [[nodiscard]] MTL::Texture* blackTexture() const { return black_; }
    [[nodiscard]] MTL::Texture* neutralDepthTexture() const { return neutralDepth_; }
    /// The world's shadow map, which the renderer sets once it has one;
    /// until then, and for anything drawn without it, the neutral depth.
    void setShadowMap(MTL::Texture* map) { shadowMap_ = map; }
    [[nodiscard]] MTL::Texture* shadowMap() const {
        return shadowMap_ ? shadowMap_ : neutralDepth_;
    }
    [[nodiscard]] MTL::Texture* neutralVolumeTexture() const { return neutralVolume_; }
    /// The air the volumetric fog has integrated this frame, which the
    /// renderer sets while it is on; otherwise, and for anything drawn
    /// without it, the neutral volume - which the per-frame data's
    /// volumetricParams.x also tells the shaders not to read.
    void setFogVolume(MTL::Texture* volume) { fogVolume_ = volume; }
    [[nodiscard]] MTL::Texture* fogVolume() const {
        return fogVolume_ ? fogVolume_ : neutralVolume_;
    }

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

    /// The GPU's time on the last frame that finished, and how long the last
    /// beginFrame waited for a frame slot - the CPU waiting on the GPU. For
    /// WOWEE_FRAME_PROFILE.
    [[nodiscard]] double lastGpuMs() const { return lastGpuMs_.load(std::memory_order_relaxed); }
    [[nodiscard]] double lastSlotWaitMs() const { return lastSlotWaitMs_; }
    /// WOWEE_FRAME_PROFILE: commits what the frame has recorded so far as a
    /// command buffer of its own, timed under segmentName, and carries on in
    /// a new one - so the GPU's time can be told apart pass by pass. Only
    /// between encoders. takeGpuSegmentReport averages them over frames.
    void splitCommandBuffer(const char* segmentName);
    [[nodiscard]] std::string takeGpuSegmentReport(int frames);

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
    bool supportsBC_ = false;
    uint64_t frameNumber_ = 0;
    std::unordered_map<uint16_t, MTL::SamplerState*> samplers_;
    MTL::SamplerState* shadowSampler_ = nullptr;
    MTL::DepthStencilState* depthStates_[8] = {};
    MTL::Texture* white_ = nullptr;
    MTL::Texture* black_ = nullptr;
    MTL::Texture* neutralDepth_ = nullptr;
    MTL::Texture* neutralVolume_ = nullptr;
    MTL::Texture* shadowMap_ = nullptr;  // the renderer's; not owned
    MTL::Texture* fogVolume_ = nullptr;  // the renderer's; not owned
    std::atomic<double> lastGpuMs_{0.0};
    void endGpuSegment(const char* name);
    static constexpr int kMaxGpuSegments = 12;
    struct GpuSegments {
        std::mutex mutex;
        const char* names[kMaxGpuSegments] = {};
        double ms[kMaxGpuSegments] = {};
        double end[kMaxGpuSegments] = {};  // this frame's, until its last is in
        double start = 0.0;
        int count = 0;
    } gpuSegments_;
    int gpuSegmentCount_ = 0;
    std::atomic<double> frameGpuStartShared_{0.0};
    double lastSlotWaitMs_ = 0.0;
    std::vector<MTL::Texture*> interfaceTextures_;
    std::string capturePath_;
};

}  // namespace rendering
}  // namespace wowee
