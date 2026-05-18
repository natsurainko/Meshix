//
// Created by Natsurainko on 2026/1/27.
//

#ifndef MESHIX_RENDER_CONTEXT_H
#define MESHIX_RENDER_CONTEXT_H

#include <memory>
#include <Vertix.Engine/Camera/PerspectiveCamera.h>
#include <Vertix.Engine/Effect/Shadow/CascadeShadowMapping.h>
#include <../../../Vertix/Vertix/include/Vertix/Rendering/Buffers/ConstantBuffer.hpp>
#include <Vertix/Graphics/Buffers/ConstantBufferPageArray.hpp>
#include <Vertix.Engine/Helpers/MathHelper.h>
#include <Vertix.Engine/Helpers/VectorHelper.h>
#include <Vertix.Engine/Pool/DefaultMaterialPool.hpp>
#include <Vertix.Engine/Scene/SceneObject3D.hpp>
#include <Vertix/Graphics/FrameCommandList.h>
#include <Vertix/Math/Vector2D.hpp>
#include <Vertix/Pool/ModelPool.hpp>
#include <Vertix/Pool/TexturePool.hpp>

#include "../shaders/structures.h"
#include "Gui/GuiContext.h"

#define SHADER_BYTECODE(T) CD3DX12_SHADER_BYTECODE(T, sizeof(T))

struct CascadeShadowConstants {
    Vertix::Engine::CascadeData CascadeDatas[CASCADE_NUM];
};

class RenderContext {
public:
    explicit RenderContext(
        Vertix::GraphicsDevice* graphicsDevice,
        Vertix::FrameCommandList* frameCommandList,
        Vertix::SwapChain* swapChain)
    : objectConstantsBuffer(graphicsDevice, 4096), graphicsDevice(graphicsDevice), swapChain(swapChain)
    {
        Vertix::ResourceUploadHeap resourceUploadHeap {};
        frameCommandList->BeginCommand(nullptr);
        fullScreenVertex = std::unique_ptr<Vertix::VertexBuffer>(Vertix::VertexBuffer::CreateFullScreenRect(graphicsDevice, frameCommandList, resourceUploadHeap));
        frameCommandList->EndCommand();
        frameCommandList->WaitForCommand();

        perspectiveCamera.SetPosition({ -8.80743f, 1.59221947f, -0.85825783f });
        perspectiveCamera.SetOrientation({ 0.0622985959f, -0.766231537f, 0.07516095f, 0.635105431f });

        frameConstants.NearFarProjScale.x = cameraNearPlane;
        frameConstants.NearFarProjScale.y = cameraFarPlane;
    }

    std::vector<std::shared_ptr<Vertix::Engine::SceneObject3D>> sceneObjects;
    std::unique_ptr<Vertix::VertexBuffer> fullScreenVertex;

    Vertix::ConstantBuffer<FrameConstants>* frameConstantsBuffer = nullptr;
    Vertix::ConstantBuffer<LightConstants>* lightConstantsBuffer = nullptr;
    Vertix::ConstantBuffer<CascadeShadowConstants>* cascadeShadowConstantsBuffer = nullptr;
    Vertix::ConstantBufferPageArray<ObjectConstants> objectConstantsBuffer;

    Vertix::DescriptorHeap* sharedDescriptorHeap = nullptr;
    std::unique_ptr<Vertix::TexturePool> texturePool;
    std::unique_ptr<Vertix::ModelPool>   modelPool;
    std::unique_ptr<Vertix::MaterialPool<Vertix::Engine::DefaultMaterialConstants>> materialPool;

    Vertix::Vector2D<UINT> windowSize;

    void UpdateFrameConstants() {
        perspectiveCamera.GetViewMatrix(frameConstants.View);
        frameConstants.ViewProjection = frameConstants.View * frameConstants.Projection;
        frameConstants.ViewProjection.Invert(frameConstants.ViewProjectionInverse);
        Vertix::Engine::FillVector4(frameConstants.CameraPosition, perspectiveCamera.GetPosition());
        frameConstantsBuffer->Fill(frameConstants);
    }

    void UpdateLightConstants() {
        lightConstants.LightDirection.Normalize(lightConstants.LightDirection);
        lightConstantsBuffer->Fill(lightConstants);
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
            lightConstants.LightDirection,
            static_cast<float>(ShadowMapSize)
        );
        cascadeShadowConstantsBuffer->Fill(cascadeShadowConstants);
    }

    void SetWindowSize(const Vertix::Vector2D<UINT> &size) {
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

    [[nodiscard]]
    Vertix::Engine::PerspectiveCamera* GetPerspectiveCamera() {
        return &perspectiveCamera;
    }

private:
    Vertix::GraphicsDevice* graphicsDevice = nullptr;
    Vertix::SwapChain*      swapChain      = nullptr;

    FrameConstants frameConstants{};
    ObjectConstants objectConstants{};
    CascadeShadowConstants cascadeShadowConstants{};
public:
    bool EnablePCSS = true;
    bool EnableHBAO = true;

    const uint32_t ShadowMapSize = 2048;
    const float cameraNearPlane = 0.1f;
    const float cameraFarPlane = 100.0f;

    const D3D12_RECT* scissorRect = nullptr;
    const D3D12_VIEWPORT* viewport = nullptr;

    LightConstants lightConstants {
        .LightDirection = float3 { 0.3f, -0.925f, -0.225f },
        .AmbientIntensity = 0.05f,
        .LightColor = float3 { 1.0f, 1.0f, 1.0f },
        .LightIntensity = 7.0f,
    };
    Vertix::Engine::PerspectiveCamera perspectiveCamera{4.0f / 3.0f, Vertix::Engine::DegreesToRadians(60), cameraNearPlane, cameraFarPlane};
};

#endif //MESHIX_RENDER_CONTEXT_H
