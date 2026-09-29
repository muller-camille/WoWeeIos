# Metal Renderer Plan

**Decision, 2026-09-28:** the renderer is rewritten on Metal, natively, and this fork keeps
iPhone and iPad as its only platforms. Vulkan, MoltenVK, and the Linux, Windows, macOS and
Android builds go at the end of the port (M6), not the start.

**Status:** M0 and M1 done, M2 under way. On 2026-09-28 the Mac build compiled all 80
translated shaders into `default.metallib` with no error or warning, the library in the app holds
exactly the 80 functions the manifest lists, and the MoltenVK build still played on the iPad.

With `WOWEE_METAL` (on by default for iOS) the iPad draws, through Metal:

- the login screen, realm list and character list (M1): `MetalContext` in place of `VkContext`,
  ImGui on `imgui_impl_metal`, the login background as an `MTL::Texture`, no `Renderer`;
- the character preview on the character creation screen (M2): `CharacterRenderer` and
  `CharacterPreview` have a Metal path beside the Vulkan one. The draw loop - culling, geosets,
  material and blend choice - is one template both backends run (`drawInstances`), each with a
  sink for its own calls. `VkTexture` holds an `MTL::Texture` in the Metal build, so every
  `VkTexture*` handle keeps working. Pipelines come from `buildMetalPipeline` and their binding
  indices from `MetalBindings`, which reads the manifest and fails at pipeline creation when an
  entry is missing. The preview's multisampled colour and depth are memoryless.

Every interface texture now goes through `uploadInterfaceTexture`, which uploads to whichever
context the window has: the icons, cursors and raid marks of `ui_texture_load` and FrameXML's art
in `widget_renderer`. Those are only drawn in the world, so on the device they wait for M3 to be
seen; both backends compile them. The loading screen and the world map's layers have
their Metal textures too.

From M4, done and seen on the iPad (2026-09-29): the minimap (its 3x3 tile composite, then the
disc over the world) and the world map (its tile and explored-overlay composite, the zone
highlights and the player and corpse markers). Each composite is a pass of its own recorded before
the world's, from a draw list both backends share.

From M3, done and seen on the iPad (2026-09-29): the M2 effects - smoke, particles, ribbons and
glow sprites, whose vertices both backends write through the same functions - and the spell
visuals, which are M2 instances and only needed their system created. The sky's sun and moons, procedural stars,
clouds and lens flare are ported too, through one SkySystem::renderImpl both backends draw with;
in zones whose sky is a model, as Teldrassil's, the model supplies them and these stay unused, as
on Vulkan. The selection circle, the underwater and ghost tints (OverlaySystem, which records into
the world pass's encoder while one is set) and the quest markers are ported as well.

Entering the world is refused in the Metal build until M3. Left for M2: `M2Renderer`, BC
textures where the GPU has them, and checking the character list's preview and the equipment on
the device. `WOWEE_SCREENSHOT=<file.png>` works in the Metal build and writes under the config
root.

**Target:** iOS 16 or later on a Metal 3 GPU (A13 or later): iPhone 11 onward, iPad Air 4 onward,
every M-series iPad. The test device is an iPad Air (4th generation, A14, 4 GB).

Measured 2026-09-28 against the tree at `cd8df6f`.

---

## 1. What is being accepted

The trade was weighed before the decision, and is written down so it is not re-argued at every
milestone.

| Given up | Why it is acceptable |
|---|---|
| Merging upstream renderer changes | The game, network, interface and asset code still merge. Every upstream change under `src/rendering/` becomes a port by hand |
| Linux, Windows, macOS and Android in this fork | The fork exists for iOS. Upstream keeps them |
| A playable build on this branch between M1 and M3 | `master` keeps the MoltenVK build, which plays, until the Metal branch reaches parity |

What it buys, and what it does not:

- **CPU.** No translation layer between the renderer and the GPU. MoltenVK's per-draw cost goes.
- **No shader hitches.** Shaders are compiled into `default.metallib` at build time instead of
  being translated from SPIR-V the first time a pipeline is made.
- **Apple's GPU features** directly: MetalFX, memoryless attachments, tile memory, heaps.
- **Not, by itself, more frames on the GPU.** The shaders are the same shaders. On the iPad Air the
  GPU was the limit in Goldshire, so the frame-rate gains come from M5 (MetalFX, memoryless
  targets), not from the port.

---

## 2. What there is to port

| Measure | Value |
|---|---|
| Renderer source (`src/rendering`, `include/rendering`) | 78,522 lines |
| Files naming a Vulkan type | 117 (104 rendering, 28 interface, 8 core, 1 pipeline, with overlap) |
| Vulkan call sites / distinct functions | 956 / 104 |
| Pipelines created | ~150 |
| Descriptor set layouts | 24 |
| Explicit barriers | 61 |
| Render passes begun | 17 `vkCmdBeginRenderPass`, 6 `vkCmdBeginRendering` |
| Push constant uploads | 46 |
| Compute dispatches | 11 |
| Shaders | 92 SPIR-V modules from 90 GLSL sources, 13 of them compute |

The interface's 71 uses of `VkDescriptorSet` are ImGui texture handles, not descriptor sets in
any real sense; they become `ImTextureID` holding an `MTL::Texture*`.

Vulkan features the port has to answer, and where they are:

| Feature | Uses | Where | Metal answer |
|---|---|---|---|
| Indexed indirect draw | 1 | `grass_renderer.cpp` | `drawIndexedPrimitives` with an indirect buffer |
| Timestamp query pool | 1 | `vk_context.cpp` | `MTL::CounterSampleBuffer`, or GPU start/end times on the command buffer |
| Timeline semaphore | 1 | `vk_context.cpp` | `MTL::SharedEvent` |
| Depth/stencil resolve | 11 | `vk_context.cpp` | `MTLStoreActionMultisampleResolve` on the depth attachment |
| Secondary command buffers | 1 | `renderer.cpp` | Parallel render encoders, or recorded inline |
| Transfer queue | 6 | `vk_context.cpp` | One queue; a blit encoder on it |
| Acceleration structures | 1 | `rt_scene.hpp` (ray tracing) | Not ported |

---

## 3. Decisions, so they are not re-litigated

1. **metal-cpp, not Objective-C++.** Apple's C++ bindings for Metal keep the renderer in `.cpp`
   files and in this codebase's idiom. Objective-C++ (`.mm`) is for glue only: the SDL view, MetalFX
   if its C++ header lags, and what is already there (`ios_platform.mm`, `miniaudio_impl.mm`).
   metal-cpp is vendored under `extern/metal-cpp` at M1, pinned to the SDK the Xcode on the build
   Mac carries.
2. **GLSL stays the source.** `assets/shaders/*.glsl` → `glslc` → tracked `.spv` →
   `tools/metal/convert_shaders.py` (SPIRV-Cross) → tracked `assets/shaders/metal/*.metal` →
   `xcrun metal` → `default.metallib`. An upstream shader fix still merges, and building the app
   needs only Xcode. `convert_shaders.py --check` says when the MSL is stale.
3. **Discrete bindings, from a manifest.** Metal has no descriptor sets. SPIRV-Cross gives each
   buffer, texture and sampler of a stage its own index, and `assets/shaders/metal/manifest.json`
   records, per function, which index every Vulkan `(set, binding)` became. The renderer keeps its
   descriptor-set-shaped code - a set is made once and bound per draw - and binding a set becomes
   the `setVertex*`/`setFragment*`/`set*` calls the manifest lists for that pipeline's stages.
   Argument buffers were tried and are not used: SPIRV-Cross numbers their members in SPIR-V id
   order, so a set shared by two stages would not be laid out the same in both. They can come back
   in M5 for the draw-heavy passes, with an explicit layout.
4. **Push constants are `setVertexBytes` / `setFragmentBytes` / `setBytes`** at the index the
   manifest gives. They are all well under Metal's 4 KB.
5. **Vertex buffers are bound from index 30 downward**, clear of the shaders' own buffers: the
   most any shader uses is 5 (`grass_vert`). The Vulkan vertex input descriptions translate one to
   one into an `MTL::VertexDescriptor`.
6. **Clip space.** Vertex shaders flip `gl_Position.y` (SPIRV-Cross `--flip-vert-y`), because
   Vulkan's clip space has +y down and Metal's +y up, with the framebuffer origin top left in both.
   The flip reverses winding: every pipeline that culls swaps its front face.
7. **Memory is unified.** Buffers the CPU writes are `StorageModeShared` and written in place;
   there are no staging buffers for them. Textures and render targets are `StorageModePrivate`,
   textures filled by a blit from a shared upload buffer. VMA goes. Resources come from `MTL::Heap`s
   where their lifetime allows, with tracked hazards, so the 61 barriers are deleted rather than
   translated.
8. **Frames in flight: two**, paced by a semaphore signalled from each command buffer's completion
   handler. That replaces the fences and the timeline semaphore.
9. **Presentation:** SDL makes the view (`SDL_Metal_CreateView`), the renderer takes its
   `CAMetalLayer`, draws into `nextDrawable()` and presents with the command buffer. Backgrounding
   stops encoding, as the MoltenVK build's `pausePresentation` does now.
10. **ImGui through its own Metal backend** (`extern/imgui/backends/imgui_impl_metal.mm`), which the
    ImGui checkout already carries.
11. **Not ported:** ray traced lighting, the AMD FidelityFX paths, FSR 2/3 (MetalFX replaces them),
    the screen recorder, the second map window, the world editor's water. `convert_shaders.py`
    leaves their shaders out by name, with the reason. Volumetric fog and sun shafts are translated
    but come last, since iOS starts with both off.
12. **Other platforms are removed at M6, not now.** Deleting them first would turn every upstream
    merge during the port into a conflict, for nothing the port needs.

---

## 4. Vulkan to Metal

| Vulkan | Metal |
|---|---|
| `VkInstance`, `VkPhysicalDevice`, `VkDevice` | `MTL::Device` (`MTL::CreateSystemDefaultDevice`) |
| `VkQueue` (graphics, transfer) | One `MTL::CommandQueue` |
| `VkCommandBuffer` | `MTL::CommandBuffer`, with render / compute / blit encoders |
| `VkRenderPass` + `VkFramebuffer`, dynamic rendering | `MTL::RenderPassDescriptor` per pass; load and store actions per attachment |
| `VkPipeline` (graphics) | `MTL::RenderPipelineState` + `MTL::DepthStencilState`; cull mode, winding, depth bias and viewport are encoder state |
| `VkPipeline` (compute) | `MTL::ComputePipelineState` |
| `VkPipelineCache` | Not needed for a precompiled library; `MTL::BinaryArchive` in M5 if pipeline creation still shows |
| `VkDescriptorSetLayout`, `VkDescriptorSet` | A small CPU-side binding set, bound through the manifest |
| `VkBuffer` + VMA | `MTL::Buffer` (shared, or private from a heap) |
| `VkImage` + `VkImageView` | `MTL::Texture`, and `newTextureView` where a view differs |
| `VkSampler` | `MTL::SamplerState` |
| Barriers, image layouts | Nothing: hazard tracking |
| `VkSemaphore`, `VkFence` | Completion handlers, `MTL::SharedEvent` |
| `VkSwapchainKHR` | `CA::MetalLayer`, `CA::MetalDrawable` |
| `vkCmdPushConstants` | `set*Bytes` |
| `vkCmdCopyBufferToImage`, `vkCmdBlitImage` | Blit encoder `copyFromBuffer`, `generateMipmaps`, or a draw where blit filtering is needed |
| Transient attachment + lazily allocated memory | `StorageModeMemoryless` |

---

## 5. Milestones

Each ends where the iPad shows the result. Later ones depend on earlier ones.

| | Ends when | Main work |
|---|---|---|
| **M0. Toolchain** | The Xcode build contains a `default.metallib` holding all 80 functions, the app still runs on MoltenVK | Shader translation (done), the library build step (written), `--check` run whenever a shader changes |
| **M1. Device and interface** | The login screen, realm list and character list draw through Metal on the iPad; the world does not draw yet | metal-cpp vendored; `MetalContext` in place of `VkContext` (device, queue, layer, frame pacing); ImGui on `imgui_impl_metal`; the Paper UI and FrameXML widget drawing; UI textures as `MTL::Texture` |
| **M2. Models and characters** | Character creation and selection show the 3D preview | Texture upload (BLP; BC where the GPU has it, the RGBA8 fallback where not); `CharacterRenderer`; `M2Renderer` static and skinned; the pipeline and binding helpers every later renderer uses |
| **M3. World** | A character can walk out of Goldshire and play | Terrain, WMO, water, sky (skybox, celestial, starfield, clouds, lens flare), weather, particles and ribbons, footprints, quest markers, selection circle, loading screen |
| **M4. Parity** | Everything the MoltenVK build draws, the Metal build draws. The branch merges to `master` | Shadow maps, Hi-Z and GPU culling, post-processing and FXAA, minimap, world map, grass, volumetric fog, sun shafts |
| **M5. Apple features** | Measured frame rate and memory beat the MoltenVK build's on the iPad Air | MetalFX spatial, then temporal with per-object motion vectors; memoryless depth and MSAA; argument buffers for material-heavy passes; Instruments passes |
| **M6. One platform** | Nothing Vulkan left in the tree | Remove Vulkan, MoltenVK, vk-bootstrap, VMA, the SPIR-V runtime path, the desktop and Android builds, their CI jobs and packaging, and the docs that describe them |

Estimates, for one developer full time with the device at hand: M1 two to three weeks, M2 and M3
together four to eight, M4 two to four, M5 two to four. Three to five months in all. The calendar
depends mostly on how fast a build can be run on the device.

---

## 6. How one renderer is ported

The same steps for each of `terrain_renderer`, `wmo_renderer`, `m2_renderer`, and the rest:

1. Its header loses every Vulkan type. Members become `MTL::` objects; `render(VkCommandBuffer, ...)`
   becomes `render(MTL::RenderCommandEncoder*, ...)`.
2. Pipelines: each `VkGraphicsPipelineCreateInfo` becomes a render pipeline descriptor naming the
   two functions from `default.metallib`, the vertex descriptor translated from the vertex input
   state, and the blend and attachment formats. Cull mode, winding (swapped, see 3.6), depth bias
   and depth state move to the encoder or a depth-stencil state object.
3. Descriptor sets become binding sets; `vkUpdateDescriptorSets` becomes setting their entries,
   and `vkCmdBindDescriptorSets` becomes binding them through the manifest for the pipeline's
   functions.
4. Buffers: VMA allocations become `MTL::Buffer`s. A buffer the CPU fills is shared and written
   directly; its staging copy is deleted.
5. Barriers are deleted. Where one existed to order a compute write before a draw, the encoder
   boundary already does.
6. The renderer's own logic - culling, instancing, animation, streaming - is not touched. A port
   that changes behaviour is two changes and is split.

---

## 7. How the work is done

- **Only a Mac builds and runs this.** The cloud sessions cannot compile Metal or reach the device.
  They can write code, translate shaders and check the manifest, as they did for M0, but every
  milestone is built and run by the session on the Mac, with the iPad attached.
- **The branch is the Metal work.** `master` stays on the MoltenVK build, which plays, until M4
  merges.
- **A shader change** is made in the GLSL, compiled to `.spv` as now, then
  `tools/metal/convert_shaders.py` regenerates the MSL and the manifest. Both are committed.
- **A milestone is not done until the iPad shows it.** "It compiles" is not a milestone.

---

## 8. Risks

| Risk | Answer |
|---|---|
| SPIRV-Cross output that Metal's compiler refuses | Found at M0: the build compiles all 80 functions before the renderer uses any |
| A binding index the renderer and shader disagree on | The manifest is the only source of both; a missing entry is an error at pipeline creation, not a black draw |
| Winding and clip space mistakes | One rule (3.6) applied in one helper that builds every pipeline |
| Residency and hazards once heaps and argument buffers arrive | Tracked hazards everywhere until M5, where each untracked resource is added deliberately |
| Upstream renderer changes during the port | Recorded as they land; ported in M4 if they are features, dropped if they are desktop-only |
| The memory ceiling (about 2.9 GB on the test iPad) | Private textures, memoryless targets and heaps in M5; the MoltenVK build's budgets carry over until then |
