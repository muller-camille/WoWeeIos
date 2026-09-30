# Metal Renderer Plan

**Decision, 2026-09-28:** the renderer is rewritten on Metal, natively, and this fork keeps
iPhone and iPad as its only platforms. Vulkan, MoltenVK, and the Linux, Windows, macOS and
Android builds go at the end of the port (M6), not the start.

**Status (2026-09-29):** M0 to M3 done, M4 under way. The Metal build plays on the iPad: the
world, its effects, shadows, water with refraction and reflection, the minimap and world map, all
drawn through Metal and upscaled by MetalFX, at about 27 fps at the Shadowglen pond on the A14.
What is left is in **Next steps** just below. On 2026-09-28 the Mac build compiled all 80
translated shaders into `default.metallib` with no error or warning, the library in the app holds
exactly the 80 functions the manifest lists, and the MoltenVK build still played on the iPad.

Later on 2026-09-29 a cloud session wrote the terrain's cheaper alpha masks, FSR 1 where MetalFX
is missing, FXAA, MSAA, and every effect M4 still lacked (listed under **Next steps**, 1). None of
it has been built by Xcode or seen on the iPad yet. What was checked: the regenerated MSL and
manifest (`convert_shaders.py --check`), the Linux build and the alpha-map test, and every
source with a Metal path parsed against metal-cpp by `tools/metal/syntax_check/check.sh`.

## Next steps (handoff, 2026-09-29, evening)

In order. What each one needs is noted, because the device work needs the Mac.

   *Checked on the Mac and the iPad, 2026-09-29 late:* it all builds (after `cmake -S . -B
   build-ios` for the new sources) and plays. At the Shadowglen pond: a frame every ~36.5 ms
   (~27 fps), the opaque world ~21.5 ms from ~23 with the one-tap alpha masks, the picture
   right. MetalFX still copies to the drawable here (the log says so). `WOWEE_METAL_FSR1=1`:
   "upscaled ... by FSR 1", the picture as with MetalFX. `WOWEE_METAL_MSAA=4`: "drawn with 4x
   MSAA", ~41 ms a frame, 1760 MB footprint. Still to look at by playing: the effects, grass,
   fog and sun shafts, FXAA, chunk seams up close, and a tree's shadow edge.

1. **Build and look at what the cloud session wrote.** First the build itself: the Metal compiler
   takes the regenerated `terrain.frag.metal`, and Xcode the new sources. Then each on the iPad:
   - *Terrain alpha masks.* The seam blur near a chunk's edge is baked into each map as it is
     uploaded (`pipeline::featherAlphaEdges`, tested by `test_adt_alpha`), and `terrain.frag`
     takes one bilinear tap per layer instead of five. Both backends. Look at chunk edges up close
     for seams, and take `WOWEE_FRAME_PROFILE` at the Shadowglen pond against the ~37 ms frame.
   - *FSR 1* (`MetalPostProcess::encodeUpscale`, `fsr_easu` as `renderFSRUpscale` draws it)
     where `MTLFX::SpatialScalerDescriptor::supportsDevice` says no. `WOWEE_METAL_FSR1=1` takes it
     on the A14. The menu's sharpening drives it.
   - *FXAA* (`MetalPostProcess::encodeFxaa`), from the menu's switch or while drunk. It runs at
     the world's size, ahead of the upscaler: MetalFX wants anti-aliased input.
   - *MSAA*, from the menu's choice as the settings file holds it at launch
     (`metalSampleCount`), or `WOWEE_METAL_MSAA=4`. Every world pipeline is built for it, so a
     new choice applies at the next launch; the menu says so on Metal. The water's split resolves
     into `mtlSceneColor_`/`mtlSceneDepth_` instead of copying. Look at the water (its refraction
     reads the resolved depth) and at foliage, where alpha-to-coverage now has samples to use.
   - *Effects:* swim ripples, bubbles and midges (swim, and stand by water plants); mount dust
     (ride); the charge trail (a warrior's Charge); the level-up column and the loot sparkle over
     lootable corpses (M2 models that only needed creating). Lightning is ported but
     `Renderer::update` turns it off whenever there is a game handler, on both backends.
   - *Grass* (the Graphics page's "Grass (experimental)"): cull as a compute pass, indexed
     indirect draw. Its 76 MB of buffers exist only while it is on.
   - *Volumetric fog* ("Light shafts and mist") and *sun shafts* ("Sun shafts"). The fog's two
     compute passes run after the shadow map; the world's shaders take the volume from
     `MetalContext::fogVolume()`. The shafts march the finished picture itself, not a
     quarter-size copy (a Metal blit does not scale).
   `WOWEE_METAL_SKIP` takes `grass`, `fog` and `sunshafts` to tell their cost apart.
   - *The first of M5,* written the same evening, also unseen: the shadow filter takes four
     taps instead of nine on the terrain, buildings, doodads and characters (edges one texel
     sharper - look at a tree's shadow on the ground); the world's depth and MSAA samples live in
     tile memory on frames the water does not split (compare `WOWEE_MEMORY_REPORT` somewhere dry
     with MSAA on); MetalFX writes straight onto the drawable where the drawable allows it (the
     log says once which way it went; the other is the copy as before).
2. **Black foliage (open bug).** The nearest tree's canopy draws solid black at night in
   Tirisfal (Tusa's spot) while distant canopies are fine. Ruled out: shadows (black with
   `WOWEE_METAL_SKIP=shadow`), the cutout (the leaf shapes are cut; only their colour is black),
   and alpha-to-coverage (now on at one sample, as Vulkan has it - correct, not the cause).
   Suspects: the leaf texture's high mips as Metal uploads them (`uploadBLPMetal`, BC where the
   GPU has it), or a foliage-only term in `m2.frag.glsl` (fringe fix at `textureLod(..., 4.0)`,
   mip-alpha boost, canopy AO). Compare against the MoltenVK build (`-DWOWEE_METAL=OFF` in a
   separate build directory) at the same spot to know whether it is Metal's at all.
3. **Undead player model:** no hair (`Player geosets: 0 1 102 ...` - the style scalp lookup
   answered the bald cap; check `CharHairGeosets` for Scourge male, or whether the style chosen
   is bald) and a pale slab on the back (geoset 1501, which every in-world character gets).
4. **Spell missiles do not fly (both backends, found 2026-09-29).** A Wrath bolt appears on the
   target instead of travelling to it: `SpellVisualSystem` has precast, cast and impact, and
   uses a visual's MissileModel only as a fallback effect in place. Wanted: on the cast going
   off, the missile M2 at the caster's hand, moved to the target at Spell.dbc's speed (the
   `playPhysicalProjectile` start/end/elapsed machinery does the moving for arrows already),
   and the impact kit played on arrival rather than at once.
5. **Tree roots swayed in the wind (both backends, fixed 2026-09-29):** "root" is a foliage
   token, so KalidarRoots01/02 bent like saplings; the plural is now a hard tree part
   (`m2_model_classifier.cpp`), with `test_m2_classifier_foliage` extended - to be run on Linux.
   **Swimmers doubled in the water (both backends, still open, left for later on 2026-09-29):**
   the refraction offset sampled the swimmer's own head and arms above the surface.
   `water.frag.glsl` now falls back to the unshifted sample when the shifted one is nearer than
   the water by more than 0.1 yard. A screenshot in the pond looked single, but swimming in
   play the body still shows twice. Making the water opaque as well was tried and dropped:
   the pond went solid dark blue. Still to look at: the blend of the refraction over the real
   scene (the part under the surface seen both straight and rippled), and the swimmer's
   own depth near the surface. The Metal
   `water.frag.metal` was edited by hand (the Mac has no glslc or spirv-cross): a session with
   the shader tools must rebuild `water.frag.spv` and rerun `tools/metal/convert_shaders.py`,
   then check on the iPad that the pond looks the same.
   **A dry strip across Shadowglen's pond (both backends, fixed 2026-09-29):** the pond steps
   down 1.4 yards inside one chunk, and that layer's unused corners are zero in MH2O. Seeing
   them, the renderer threw the whole layer's heights away and laid it flat at the lower level,
   under the pond bed. `liquidCornerHeights` (`water_surface_grid.hpp`) now replaces only the
   bad corners; `test_water_surface_grid` has two cases for it, to be run on Linux.
6. Close M4: fps and memory at Goldshire and a capital, then the branch merges. The Linux
   client `master`'s CI builds compiles and links (a GCC `-Wchanges-meaning` error in
   `CharacterRenderer`'s Vulkan sink was fixed on the way). Of its 217 tests on 2026-09-29, in a
   Debug build, 214 passed. `shared_rules` and `settings_schema_consistency` fail only in Debug,
   where the `#ifndef NDEBUG` FSR 3 tuning entries split the Upscaling category; CI builds
   Release. `sweep_guard` failed on two findings, both fixed on the Mac and to be rerun on
   Linux: `MetalContext::frameGpuStart_` is gone, and `Celestial` and `Clouds` each keep their
   Metal releases in a `releaseMetal()` that `shutdown()` calls.

Not in M4, as decided or found: ray traced lighting is not ported (3.11). The Hi-Z pyramid is
never created on either backend, and the M2 GPU cull stays Vulkan's - the Metal M2 path culls on
the CPU. Porting the cull alone would not help: without Hi-Z it makes the same frustum and
distance tests the CPU path does, and around them the CPU writes every instance's input and
scatters the results back, two frames late - more CPU work, and doodads that appear two frames
after they turn into view. It is worth having only with occlusion, which is Hi-Z, which was left
off on Vulkan for its false culls. A menu MSAA change that applies without a relaunch was
weighed too: every world renderer builds its pipelines inside its initializeMetal among its other
resources, so it is about twenty classes to make rebuildable. Both wait for the device.

A cloud session can do the shader half of a change: `apt install glslc spirv-cross` gives the
tools, and on 2026-09-29 Ubuntu's spirv-cross reproduced every tracked MSL file exactly. Note
that building the Linux tree recompiles `.spv` files whose GLSL looks newer, in place: check
`git status` for `.spv` changes you did not make before committing.

Device testing, from the Mac with the iPad unlocked (see the memory notes for the commands):
`WOWEE_AUTO_ENTER=1` (with `WOWEE_AUTO_CHARACTER=<name>`) enters the world, 
`WOWEE_WORLD_SCREENSHOT=<png>` takes a picture after `WOWEE_WORLD_SCREENSHOT_DELAY` seconds and
the game keeps running, `WOWEE_WORLD_SCREENSHOT_LUA` runs interface Lua before it,
`WOWEE_FRAME_PROFILE=1` logs the frame per step and per GPU segment every 120 frames,
`WOWEE_METAL_SKIP_CYCLE="none;terrain;..."` compares skipped passes in one session,
`WOWEE_MEMORY_REPORT=1` logs the footprint every two seconds.

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

From M4, shadows (2026-09-29): one depth map drawn each frame before the world pass, with the
terrain, buildings, doodads (foliage alpha-tested and bent by the wind, through the same
M2Renderer::renderShadowImpl as Vulkan) and characters (skinned) as casters, and every surface
sampling it. The sampler, bias, culling and compare match the Vulkan pipeline. A ring of maps,
one per MetalContext slot, as Vulkan has one per frame in flight. The map's side follows
extShadowQuality (1024 on a phone). The black foliage once put down to shadows is not theirs -
see Next steps.

Water refraction and the shoreline fade (2026-09-29, seen on the iPad): where there is water the
world pass stops before it, the drawable's colour and the depth so far are copied out, and the
pass is taken up again with its attachments loaded, as Vulkan's scene continuation pass does.
The world's depth is stored rather than memoryless for it, and drawables are readable. The
reflection draws the sky, terrain and buildings from the mirrored camera into a 512 target;
the Detail page's "Water reflections" (Metal only) turns it off. With MSAA the split resolves
colour and depth into the copies' textures instead, and the reflection is multisampled too.

Performance, iPad Air 4 (A14), Shadowglen pond, 2026-09-29: about 8 fps with the world at full
resolution; about 25 fps (a frame every ~40 ms, GPU-bound) once it is drawn at 0.67 and
upscaled by MetalFX's spatial scaler, which the menu's upscaling and render-quality choices now
drive on Metal (FSR 1 where the device has no MetalFX, since the evening of 2026-09-29). Measured with WOWEE_FRAME_PROFILE,
which splits the frame into command buffers and times each by how much later it ends than the
one before, and WOWEE_METAL_SKIP_CYCLE, which steps through sets of skipped passes in one
session - launches compared against each other are too noisy, each facing somewhere else. The
"GPU ms" total overstates the frame when the GPU overlaps frames; the frame period is the
report's interval over its 120 frames. Shadows move the total but not the frame rate. What the
frame is spent on is the opaque world pass, about 28 ms of the 40: the sky and the terrain about
6 ms each. The sky is now drawn after the terrain and buildings with the viewport's depth range
pinned to the far plane, so covered pixels are never shaded: the opaque pass came down to ~23 ms
and the frame to ~37 ms (~27 fps). The terrain was next: its alpha masks now take one tap each
(Next steps, 1), not yet measured.

Also from M4, done and seen on the iPad (2026-09-29): the minimap (its 3x3 tile composite, then the
disc over the world) and the world map (its tile and explored-overlay composite, the zone
highlights and the player and corpse markers). Each composite is a pass of its own recorded before
the world's, from a draw list both backends share.

From M3, done and seen on the iPad (2026-09-29): the M2 effects - smoke, particles, ribbons and
glow sprites, whose vertices both backends write through the same functions - and the spell
visuals, which are M2 instances and only needed their system created. The sky's sun and moons, procedural stars,
clouds and lens flare are ported too, through one SkySystem::renderImpl both backends draw with;
in zones whose sky is a model, as Teldrassil's, the model supplies them and these stay unused, as
on Vulkan. The selection circle, the underwater and ghost tints (OverlaySystem, which records into
the world pass's encoder while one is set) and the quest markers are ported as well. So are the
weather (rain, snow, storms) and the footprints, the latter seen on the iPad (a dwarf's trail in
the Dun Morogh snow). The frame's last Vulkan-only effects - lightning, swim ripples, mount dust
and the charge trail - were ported on the evening of 2026-09-29, with grass, volumetric fog and
sun shafts, and wait to be seen (Next steps, 1).

`WOWEE_SCREENSHOT=<file.png>` works in the Metal build and writes under the config root.

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
| **M5. Apple features** | Measured frame rate and memory beat the MoltenVK build's on the iPad Air | MetalFX spatial (done), then temporal with per-object motion vectors; memoryless depth and MSAA (done where the water does not split the pass); argument buffers for material-heavy passes; Instruments passes |
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
  milestone is built and run by the session on the Mac, with the iPad attached. What they can
  also do is parse the Metal build's C++: `tools/metal/syntax_check/check.sh` runs clang over it
  with metal-cpp and stand-ins for Apple's system headers, which catches what does not compile
  before the Mac does - not what does not work.
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
