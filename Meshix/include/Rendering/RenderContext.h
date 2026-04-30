//
// Created by Natsurainko on 2026/1/27.
//

#ifndef MESHIX_RENDER_CONTEXT_H
#define MESHIX_RENDER_CONTEXT_H

#include <memory>
#include <d3d12/d3dx12_core.h>
#include <Vertix.Engine/Camera/PerspectiveCamera.h>
#include <Vertix/Graphics/DescriptorHeap.h>
#include <Vertix.Engine/Effect/Shadow/CascadeShadowMapping.h>
#include <Vertix/Graphics/Buffers/ConstantBuffer.hpp>
#include <Vertix/Graphics/Buffers/ConstantBufferPageArray.hpp>
#include <Vertix.Engine/Helpers/MathHelper.h>
#include <Vertix.Engine/Helpers/VectorHelper.h>
#include <Vertix.Engine/Pool/DefaultMaterialPool.hpp>
#include <Vertix.Engine/Scene/SceneObject3D.hpp>
#include <Vertix/Math/Vector2D.hpp>
#include <Vertix/Pool/ModelPool.hpp>
#include <Vertix/Pool/TexturePool.hpp>

#include "../shaders/structures.h"

#define SHADER_BYTECODE(T) CD3DX12_SHADER_BYTECODE(T, sizeof(T))

struct CascadeShadowConstants {
    Vertix::Engine::CascadeData CascadeDatas[CASCADE_NUM];
};

class RenderContext {
public:
    explicit RenderContext(const Vertix::GraphicsDevice* graphicsDevice) :
        frameConstantsBuffer(graphicsDevice),
        lightConstantsBuffer(graphicsDevice),
        cascadeShadowConstantsBuffer(graphicsDevice),
        objectConstantsBuffer(graphicsDevice, 4096),
        rtvDescriptorHeap(graphicsDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV),
        dsvDescriptorHeap(graphicsDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV),
        srvDescriptorHeap(graphicsDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32, true),
        texturePool(graphicsDevice),
        materialPool(graphicsDevice),
        graphicsDevice(graphicsDevice)
    {
        perspectiveCamera.SetPosition({ -8.80743f, 1.59221947f, -0.85825783f });
        perspectiveCamera.SetOrientation({ 0.0622985959f, -0.766231537f, 0.07516095f, 0.635105431f });
    }

    std::vector<std::shared_ptr<Vertix::Engine::SceneObject3D>> sceneObjects;
    std::unique_ptr<Vertix::VertexBuffer> fullScreenVertex = nullptr;

    Vertix::ConstantBuffer<FrameConstants> frameConstantsBuffer;
    Vertix::ConstantBuffer<LightConstants> lightConstantsBuffer;
    Vertix::ConstantBuffer<CascadeShadowConstants> cascadeShadowConstantsBuffer;
    Vertix::ConstantBufferPageArray<ObjectConstants> objectConstantsBuffer;

    D3D12_GPU_DESCRIPTOR_HANDLE geometrySrvGpuHandles[4] = {};
    D3D12_GPU_DESCRIPTOR_HANDLE shadowGeometrySrvGpuHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE shadowSrvGpuHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE ambientOcclusionSrvGpuHandle{};

    D3D12_RESOURCE_BARRIER geometryRtvBarriers[4] = {};
    D3D12_RESOURCE_BARRIER shadowGeometryDsvBarrier{};
    D3D12_RESOURCE_BARRIER shadowRtvBarrier{};
    D3D12_RESOURCE_BARRIER ambientOcclusionRtvBarrier{};

    Vertix::DescriptorHeap rtvDescriptorHeap;
    Vertix::DescriptorHeap dsvDescriptorHeap;
    Vertix::DescriptorHeap srvDescriptorHeap;

    Vertix::TexturePool<> texturePool;
    Vertix::Engine::DefaultMaterialPool<> materialPool;
    Vertix::ModelPool modelPool;

    Vertix::Vector2D<UINT> windowSize;
    CD3DX12_VIEWPORT viewport{0.f,0.f, 0.f, 0.f};
    CD3DX12_RECT scissorRect{};

    void UpdateFrameConstants() {
        perspectiveCamera.GetViewMatrix(frameConstants.View);
        frameConstants.ViewProjection = frameConstants.View * frameConstants.Projection;
        Vertix::Engine::FillVector4(frameConstants.CameraPosition, perspectiveCamera.GetPosition());
        frameConstantsBuffer.Fill(frameConstants);
    }

    void UpdateLightConstants() {
        LightConstants.LightDirection.Normalize(LightConstants.LightDirection);
        lightConstantsBuffer.Fill(LightConstants);
    }

    void UpdateObjectConstants() {
        for (UINT i = 0; i < sceneObjects.size(); i++) {
            const auto &sceneObject = sceneObjects[i];
            objectConstants.World = sceneObject->GetWorldMatrix();
            objectConstants.WorldInverseTranspose = sceneObject->GetWorldInverseTranspose();
            objectConstantsBuffer.FillAt(i, objectConstants);
        }
    }

    void UpdateCascadeShadowConstants() {
        Vertix::Engine::SetupCascades<CASCADE_NUM>(
            cascadeShadowConstants.CascadeDatas,
            frameConstants.ViewProjection,
            cameraNearPlane,
            cameraFarPlane,
            LightConstants.LightDirection,
            static_cast<float>(ShadowMapSize)
        );
        cascadeShadowConstantsBuffer.Fill(cascadeShadowConstants);
    }

    void SetWindowSize(const Vertix::Vector2D<UINT> &size) {
        windowSize = size;
        viewport.Width = static_cast<float>(windowSize.X);
        viewport.Height = static_cast<float>(windowSize.Y);
        scissorRect.right = static_cast<LONG>(windowSize.X);
        scissorRect.bottom = static_cast<LONG>(windowSize.Y);

        frameConstants.FrameResolution.x = static_cast<float>(windowSize.X);
        frameConstants.FrameResolution.y = static_cast<float>(windowSize.Y);
        frameConstants.FrameResolutionInverse.x = 1.f / static_cast<float>(windowSize.X);
        frameConstants.FrameResolutionInverse.y = 1.f / static_cast<float>(windowSize.Y);

        perspectiveCamera.SetAspect(static_cast<float>(windowSize.X) / static_cast<float>(windowSize.Y));
        perspectiveCamera.GetProjectionMatrix(frameConstants.Projection);
    }

    [[nodiscard]] Vertix::Engine::PerspectiveCamera* GetPerspectiveCamera() {
        return &perspectiveCamera;
    }

    bool EnablePCSS = true;
    bool EnableHBAO = true;

    const uint32_t ShadowMapSize = 2048;

    LightConstants LightConstants {
        .LightDirection = float3 { 0.3f, -0.925f, -0.225f },
        .AmbientIntensity = 0.05f,
        .LightColor = float3 { 1.0f, 1.0f, 1.0f },
        .LightIntensity = 7.0f,
    };
private:
    const float cameraNearPlane = 0.1f;
    const float cameraFarPlane = 100.0f;
    Vertix::Engine::PerspectiveCamera perspectiveCamera{4.0f / 3.0f, Vertix::Engine::DegreesToRadians(60), cameraNearPlane, cameraFarPlane};

    const Vertix::GraphicsDevice* graphicsDevice = nullptr;

    FrameConstants frameConstants{};
    ObjectConstants objectConstants{};
    CascadeShadowConstants cascadeShadowConstants{};
};

#endif //MESHIX_RENDER_CONTEXT_H
