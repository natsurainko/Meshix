//
// Created by Natsurainko on 2026/1/27.
//

#ifndef MESHIX_RENDER_CONTEXT_H
#define MESHIX_RENDER_CONTEXT_H

#include <memory>
#include <Vertix.Engine/Camera/PerspectiveCamera.h>
#include <Vertix/Graphics/DescriptorHeap.h>
#include <Vertix/Graphics/SwapChain.h>
#include <Vertix/Rendering/Buffers/ConstantBuffer.hpp>
#include <Vertix.Engine/Helpers/MathHelper.h>
#include <Vertix.Engine/Helpers/VectorHelper.h>
#include <Vertix.Engine/Scene/SceneObject3D.hpp>
#include <Vertix/Graphics/FrameCommandList.h>
#include <Vertix/Math/Vector2D.hpp>
#include <Vertix/Pool/ModelPool.hpp>
#include <Vertix/Pool/TexturePool.hpp>

#include "../shaders/structures.h"
#include "Vertix/Pool/MaterialPool.hpp"
#include "Vertix/Rendering/Buffers/StructuredBuffer.hpp"

#define SHADER_BYTECODE(T) CD3DX12_SHADER_BYTECODE(T, sizeof(T))

struct ObjectTag {};
using ObjectHandle = Vertix::ResourceHandle<ObjectTag>;

class RenderContext {
    struct MeshCullingConstantsRange {
        uint32_t startIndex;
        uint32_t count;
    };

public:
    explicit RenderContext(
        Vertix::GraphicsDevice* graphicsDevice,
        Vertix::FrameCommandList* frameCommandList,
        Vertix::SwapChain* swapChain) : graphicsDevice(graphicsDevice), swapChain(swapChain)
    {
        Vertix::ResourceUploadHeap resourceUploadHeap {};
        frameCommandList->BeginCommand(nullptr);
        fullScreenVertex = std::unique_ptr<Vertix::VertexBuffer>(Vertix::VertexBuffer::CreateFullScreenRect(graphicsDevice, frameCommandList, resourceUploadHeap));
        frameCommandList->EndCommand();
        frameCommandList->WaitForCommand();

        graphicsDevice->CreateCommandQueue(copyCommandQueue, { .Type = D3D12_COMMAND_LIST_TYPE_COPY, .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE });
        graphicsDevice->CreateCommandQueue(computeCommandQueue, { .Type = D3D12_COMMAND_LIST_TYPE_COMPUTE, .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE });
        sharedDirectCommandList = std::make_unique<Vertix::GraphicsCommandList>(graphicsDevice->GetD3D12Device(), frameCommandList->GetD3D12CommandQueue(), D3D12_COMMAND_LIST_TYPE_DIRECT);

        perspectiveCamera.SetPosition({ 31.548f, 7.229f, 46.704f });
        perspectiveCamera.SetOrientation({ -0.047f, -0.579f, -0.033f, 0.813f });

        frameConstants.NearFarProjScale.x = cameraNearPlane;
        frameConstants.NearFarProjScale.y = cameraFarPlane;

        for (uint32_t i = 0; i < CULLING_VIEW_NUM; ++i) {
            cullingViewConstants.CullingViewDatas[i].MaxCommandCount = 65535;
            if (i) cullingViewConstants.CullingViewDatas[i].CullSkipMask = 1 << 4;
        }

        lightConstants = {
            .LightDirection   = { 0.3f, -0.925f, -0.225f },
            .AmbientIntensity = 0.2f,
            .LightColor       = { 1.0f, 1.0f, 1.0f },
            .LightIntensity   = 10.0f,
        };

        currentCullingConstantsIndex = 0;
    }

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> copyCommandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> computeCommandQueue;
    std::unique_ptr<Vertix::GraphicsCommandList> sharedDirectCommandList;

    std::unique_ptr<Vertix::VertexBuffer> fullScreenVertex;

    // Created on UploadHeap, double(=SwapChain FrameCount) buffering is required.
    Vertix::ConstantBuffer<FrameConstants>* frameConstantsBuffer[2] = {};
    Vertix::ConstantBuffer<LightConstants>* lightConstantsBuffer[2] = {};
    Vertix::ConstantBuffer<CullingViewConstants>* cullingViewConstantsBuffer[2] = {};
    Vertix::ConstantBuffer<CascadeShadowConstants>* cascadeShadowConstantsBuffer[2] = {};

    Vertix::StructuredBuffer<MaterialConstants>* materialStructuredBuffer = nullptr;
    Vertix::StructuredBuffer<ObjectConstants>* objectStructuredBuffer = nullptr;
    Vertix::StructuredBuffer<MeshCullingConstants>* meshCullingStructuredBuffer = nullptr;

    Vertix::DescriptorHeap* sharedDescriptorHeap = nullptr;
    std::unique_ptr<Vertix::TexturePool> texturePool;
    std::unique_ptr<Vertix::ModelPool>   modelPool;
    std::unique_ptr<Vertix::MaterialPool<MaterialConstants>> materialPool;
    std::unique_ptr<Vertix::ResourcePool<Vertix::Engine::SceneObject3D, ObjectHandle>> objectPool;

    Vertix::Vector2D<UINT> windowSize;

    void PrepareMeshIndirectBufferHandles(Vertix::DescriptorView<Vertix::RenderResourceUsage::UnorderedAccess> handles[CULLING_VIEW_NUM]) noexcept {
        for (uint32_t i = 0; i < CULLING_VIEW_NUM; ++i) {
            cullingViewConstants.CullingViewDatas[i].IndirectCommandBufferHandle = handles[i].slot;
        }
    }

    void OnFrameUpdate() {
        const auto frameIndex = GetCurrentFrameIndex();

        perspectiveCamera.GetViewMatrix(frameConstants.View);
        frameConstants.ViewProjection = frameConstants.View * frameConstants.Projection;
        frameConstants.ViewProjection.Invert(frameConstants.ViewProjectionInverse);
        Vertix::Engine::FillVector4(frameConstants.CameraPosition, perspectiveCamera.GetPosition());
        frameConstantsBuffer[frameIndex]->Fill(frameConstants);

        lightConstants.LightDirection.Normalize(lightConstants.LightDirection);
        lightConstantsBuffer[frameIndex]->Fill(lightConstants);

        Vertix::Engine::SetupCascades<CASCADE_NUM>(
            cascadeShadowConstants.CascadeDatas,
            frameConstants.ViewProjection,
            cameraNearPlane,
            cameraFarPlane,
            lightConstants.LightDirection,
            static_cast<float>(ShadowMapSize)
        );
        cascadeShadowConstantsBuffer[frameIndex]->Fill(cascadeShadowConstants);

        cullingViewConstants.MeshCount = currentCullingConstantsIndex;
        Vertix::Engine::ExtractFrustumPlanes(
            frameConstants.ViewProjection,
            cullingViewConstants.CullingViewDatas[0].FrustumPlanes
        );
        for (uint i = 0; i < CASCADE_NUM; ++i) {
            Vertix::Engine::ExtractFrustumPlanes(
                cascadeShadowConstants.CascadeDatas[i].LightViewProjection,
                cullingViewConstants.CullingViewDatas[1 + i].FrustumPlanes
            );
        }
        cullingViewConstantsBuffer[frameIndex]->Fill(cullingViewConstants);
    }

    void AddSceneObject(std::unique_ptr<Vertix::Engine::SceneObject3D> sceneObject) {
        const Vertix::Engine::SceneObject3D* object = sceneObject.get();
        const ObjectHandle handle = objectPool->Allocate(std::move(sceneObject));

        std::vector<MeshCullingConstants> meshCullingConstants;
        for (const auto &mesh : object->SceneModel->Meshes) {
            if (!mesh.IndexBuffer || !mesh.VertexBuffer) continue;
            meshCullingConstants.emplace_back(MeshCullingConstants {
                .ObjectHandle = handle.slot,
                .MaterialHandle = mesh.Material.slot,
                .IndexCount = mesh.IndexBuffer->indexCount,
                .VertexCount = mesh.VertexBuffer->vertexCount,
                .IndexBufferAddress = mesh.IndexBuffer->d3d12Resource->GetGPUVirtualAddress(),
                .VertexBufferAddress = mesh.VertexBuffer->d3d12Resource->GetGPUVirtualAddress(),
                .LocalBounds = BoundingBox {
                    .Center = mesh.BoundingBox.Center,
                    .Extents = mesh.BoundingBox.Extents,
                }
            });
        }

        const ObjectConstants objectConstants = {
            .World = object->GetWorldMatrix(),
            .WorldInverseTranspose = object->GetWorldInverseTranspose(),
        };

        sharedDirectCommandList->BeginCommand(nullptr);
        {
            const auto copyCommandList = sharedDirectCommandList->GetD3D12GraphicsCommandList().Get();
            objectStructuredBuffer->Fill(copyCommandList, handle.slot - 1, objectConstants);
            if (!meshCullingConstants.empty()) {
                meshCullingStructuredBuffer->FillRange(copyCommandList, currentCullingConstantsIndex, meshCullingConstants);
            }
        }
        sharedDirectCommandList->EndCommand();
        sharedDirectCommandList->WaitForCommand();

        auto count = static_cast<uint32_t>(meshCullingConstants.size());
        meshCullingConstantsRange.emplace_back(currentCullingConstantsIndex, count);
        currentCullingConstantsIndex += count;
    }

    void SetWindowSize(const Vertix::Vector2D<UINT> &size) noexcept {
        windowSize = size;

        frameConstants.FrameResolution.x = static_cast<float>(windowSize.X);
        frameConstants.FrameResolution.y = static_cast<float>(windowSize.Y);
        frameConstants.FrameResolutionInverse.x = 1.f / static_cast<float>(windowSize.X);
        frameConstants.FrameResolutionInverse.y = 1.f / static_cast<float>(windowSize.Y);

        perspectiveCamera.SetAspect(static_cast<float>(windowSize.X) / static_cast<float>(windowSize.Y));
        perspectiveCamera.GetProjectionMatrix(frameConstants.Projection);

        frameConstants.NearFarProjScale.z = frameConstants.Projection._11;
        frameConstants.NearFarProjScale.w = frameConstants.Projection._22;
    }

    [[nodiscard]] Vertix::Engine::PerspectiveCamera* GetPerspectiveCamera() noexcept { return &perspectiveCamera; }
    [[nodiscard]] uint32_t GetCurrentFrameIndex() const noexcept { return swapChain->GetCurrentFrameIndex(); }
    [[nodiscard]] uint32_t GetMeshCount() const noexcept { return currentCullingConstantsIndex; }
    [[nodiscard]] LightConstants& RefLightConstants() noexcept { return lightConstants; }

    const uint32_t ShadowMapSize = 2048;
    const float cameraNearPlane = 0.1f;
    const float cameraFarPlane = 100.0f;

    const D3D12_RECT* scissorRect = nullptr;
    const D3D12_VIEWPORT* viewport = nullptr;

private:
    Vertix::GraphicsDevice* graphicsDevice = nullptr;
    Vertix::SwapChain*      swapChain = nullptr;

    FrameConstants         frameConstants = {};
    LightConstants         lightConstants = {};
    CullingViewConstants   cullingViewConstants = {};
    CascadeShadowConstants cascadeShadowConstants = {};

    uint32_t currentCullingConstantsIndex;
    std::vector<MeshCullingConstantsRange> meshCullingConstantsRange;

    Vertix::Engine::PerspectiveCamera perspectiveCamera = {
        4.0f / 3.0f,
        Vertix::Engine::DegreesToRadians(60),
        cameraNearPlane,
        cameraFarPlane
    };
};

#endif //MESHIX_RENDER_CONTEXT_H
