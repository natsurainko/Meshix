# Meshix
A lightweight modern D3D12 Real-Time renderer developed based on the *[Vertix Framework](https://github.com/natsurainko/Vertix)*  

### Screenshots
<img width="1332" height="850" alt="5bcf4ba6996bc8c9203821667f8a08bd" src="https://github.com/user-attachments/assets/0d678fb9-b685-4e09-a675-a152d9c71497" />

## Project Structural
``` text
Meshix/
├── src/              # Source code
├── include/          # Public headers
├── shaders/          # HLSL shader files
├── CMakeLists.txt    # Build configuration
└── README.md         # This file
```

## Shader Effects
- **Deferred Rendering**: Multi-pass rendering with geometry and lighting passes for efficient handling of multiple light sources.
- **Bindless Texturing**: Using texture handle for bindless texture access and improved performance.
- **Cascaded Shadow Mapping**: Efficient shadow mapping across multiple view frustum levels.
- **PCSS Soft Shadows**: Percentage-Closer Soft Shadows for realistic shadow penumbra.
- **PBR Texture-Based Lighting**: Physically-Based Rendering using texture-based material properties.
- **HBAO (Horizon-Based Ambient Occlusion)**: Use HBAO to improve the quality of shaded ambient light.

## Build Requirements
- C++ 20
- CMake 4.0
- MSVC, Windows 10 SDK (or matching SDK for your VS)
- DXCompiler Latest Version
- Submodule: Vertix Framework

## Contributing
- Issues and PRs are welcome. Please provide a clear description of changes and, when applicable, build instructions for reproducing.
