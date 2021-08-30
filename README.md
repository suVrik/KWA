# KURWA

This is a game engine I created in 4 months to improve my game engine programming skills.

[![Video](https://i.imgur.com/sWgBvlF_d.webp?maxwidth=760&fidelity=grand)](https://www.youtube.com/watch?v=7YzHbVlsqNA)

_Video version of the text below_

## Features

### Abstract Low-Level Rendering API
Simple yet efficient rendering API was designed to abstract away from modern graphics APIs. It makes extensive use of custom GPU memory allocators, CPU and GPU parallelism. Unlike in Vulkan or DirectX 12, there's no need to think about synchronization.

### Frame Graph
Frame graph makes it very simple to manage render passes and attachments. It automatically sets memory barriers, image layout transitions, allocates memory for attachments, makes use of memory aliasing for non-overlapping attachments. There’s even no such thing as a command buffer! All you do is submit draw calls in parallel in any order you want, and the frame graph takes care of the rest. Very simple and still efficient.

### Frustum Culling
Acceleration structures such as quadtree and octree are extensively used to cull geometry, lights, shadows, environment maps, and particle effects.

### Concurrency
All subsystems run in parallel on any number of threads. Many subsystems spawn parallel subtasks that make even greater use of all available processing power.

### Texture Streaming
Texture streaming allows loading textures from disk and present them on the screen within the same frame without framerate drops. A custom texture format stores MIP levels from smallest to largest to aid texture streaming.

### Particle Effects
Particle effects simulation makes extensive use of SIMD and SOA memory layout. Different particle systems are simulated in parallel.

### Image Based Lighting
Parallax-correcting weight-blended local environment maps are baked right in the engine at runtime (it’s not a requirement to bake them at runtime though, they can easily be loaded from disk as well).

### Shadow Mapping
Percentage-Closer Soft Shadows and translucent shadow maps make great use of multithreading and frustum culling: each cubemap side is rendered in parallel and performs its own frustum culling query.

### Post-Processing
Frame graph architecture allows to easily add new post-processing techniques such as bloom (inspired by COD: MW bloom), tonemapping (Uncharted 2) or FXAA.

### Data Driven
The engine supports many resource types: textures, models, animations, shaders, materials, particle effects, prefabs, and levels. All of them can be edited and added into the engine without writing any code. Every resource is loaded in parallel.

## Build

1) Install vcpkg as described [here](https://vcpkg.io/en/getting-started.html);
2) Download and install Vulkan from [here](https://vulkan.lunarg.com/);
3) Install thirdparties like this:
```
vcpkg install sdl2
vcpkg install vulkan:x64-windows
```
4) Clone and generate the MSVC solution like this:
```
git clone https://github.com/suVrik/KURWA.git
cd KURWA
mkdir _build
cd _build
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ..
```
