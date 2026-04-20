# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Temerald** is a DirectX 12 real-time renderer / game featuring an infinite procedural forest with a leaf actor driving forward motion through a deferred rendering pipeline. The executable target is `forest_runner`.

## Tech Stack

- **API:** DirectX 12 with DXGI (`IDXGISwapChain3`, `FLIP_DISCARD`, 2 back buffers)
- **Math:** DirectXMath (SIMD, XMFLOAT4X4 / XM* types throughout)
- **Windowing:** Native Win32 (`WinMain`, `PeekMessage` pump)
- **Build:** CMake + Visual Studio 2022 generator, x64 only, out-of-source in `/build`
- **Shaders:** DXC, HLSL Shader Model 6.x, compiled to `/shaders/compiled` at app startup with timestamp-based caching; hot-reload on F5
- **Language:** C++

## Build Commands

```bash
# Configure (from repo root)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# Build
cmake --build build --config Debug
cmake --build build --config Release

# Run
./build/Debug/forest_runner.exe
```

## Repository Layout (planned)

```
/src
  app/       WinMain, application lifecycle
  engine/    top-level engine init and loop
  render/    Device, swap chain, G-buffer, deferred passes
  scene/     Scene, SceneObject, MeshRegistry, MaterialRegistry
  world/     ChunkManager, Chunk, procedural generation
  util/      log.h (LOG_INFO/WARN/ERROR), Clock, HR() HRESULT helper
/include     public headers mirroring /src
/shaders     .hlsl source; compiled output in /shaders/compiled
/assets      placeholder/, paths/
/external    DirectXMath (header-only), DirectX-Headers, dxc bin
/build       CMake out-of-source (git-ignored)
```

## Architecture

### Render Pipeline (deferred)

Two-pass deferred pipeline per frame:

1. **Geometry pass** — `gbuffer.hlsl` writes to four G-buffer targets:
   - RT0 `R8G8B8A8_UNORM`: albedo + AO
   - RT1 `R10G10B10A2_UNORM`: packed world-space normal + flags
   - RT2 `R8G8B8A8_UNORM`: metallic + roughness + material ID
   - DSV `D32_FLOAT`: depth (used for position reconstruction)
2. **Lighting pass** — `lighting.hlsl` fullscreen triangle reads G-buffer SRVs, outputs to back buffer

Locked vertex layout: `Position(3) | Normal(3) | Tangent(4) | UV(2)`

Frame synchronization: `FRAMES_IN_FLIGHT = 2` `FrameContext`s, each with its own command allocator and fence value. `BeginFrame` waits on the fence for the slot being reused.

### Instancing

Per-frame upload buffer holds `StructuredBuffer<InstanceData>` (`float4x4 world; uint32_t materialId; uint32_t pad[3]`). The renderer buckets `SceneObject`s by `(mesh, material)` and issues one `DrawIndexedInstanced` per bucket (max 4096 instances per draw, splits if exceeded). Geometry-pass VS reads `world` from this buffer via `SV_InstanceID + baseInstance`.

### World Streaming

`ChunkManager` maintains a hash map of `Chunk` structs (32×32m XZ, keyed by `ChunkCoord`). Each frame it computes the chunk under the leaf, requests a 3×3 ring, and evicts chunks outside `R+1` rings. Generation uses a deterministic per-chunk seed (`hash(coord)`). Chunk generation is on the main thread (marked `// TODO: jobify`).

Each chunk scatters: ~80 trees, ~25 rocks, ~150 debris quads via stratified jitter; a ~3m corridor around the camera path is kept clear.

### Leaf Actor & Camera

The `Leaf` actor owns the path parameter `t` and drives both camera and streaming. The `GuidedPath` uses a Catmull-Rom spline over ~12 hand-coded control points; arc-length is precomputed for stable speed. The camera reads `leaf.position + tangent` and applies a fixed offset (+0.6m up) with a lookahead point for aim direction.

Debug toggles: `F1` detaches camera from leaf; `Space` pauses leaf/streaming; `WASD` nudges travel speed; `F5` hot-reloads shaders.

### Resource Handles

`MeshHandle` and `MaterialHandle` are opaque indices into `MeshRegistry` and `MaterialRegistry` respectively. These registries own GPU vertex/index buffers, PSOs, root signatures, and parameter blocks.

## Descriptor Heap Strategy

One CBV/SRV/UAV shader-visible heap with ~4096 slots. Separate RTV and DSV heaps. G-buffer SRVs live in the shader-visible heap; the lighting pass binds them via a descriptor table.

## Debug / Profiling

- D3D12 debug layer + GPU-based validation enabled in Debug builds
- PIX/RenderDoc markers (`PIXBeginEvent`/`SetMarker`) wrap each render pass
- GPU timestamp queries (`D3D12_QUERY_TYPE_TIMESTAMP`) readback one frame late to avoid stalls
- Window title updated once/second: `forest_runner | FPS | CPU ms | GPU ms | chunks N | objs N`
- `HR(expr)` macro throws on failed `HRESULT` with file/line info
