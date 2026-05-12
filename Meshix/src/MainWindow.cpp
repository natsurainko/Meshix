//
// Created by Natsurainko on 2026/4/30.
//

#include "MainWindow.h"

#include <filesystem>
#include <Vertix/Rendering/Pipeline/RenderPipelineBuilder.h>
#include <Vertix.Engine/Content/ModelLoader.h>
#include <Vertix.Engine/Content/TextureLoader.h>
#include <Vertix.Engine/Primitive/DefaultPBRMaterial.h>

#include "Rendering/Passes/AmbientOcclusionPass.h"
#include "Rendering/Passes/GeometryPass.h"
#include "Rendering/Passes/ImGuiPass.h"
#include "Rendering/Passes/LightingPass.h"
#include "Rendering/Passes/ShadowGeometryPass.h"
#include "Rendering/Passes/ShadowPass.h"

void MainWindow::BuildRenderPipeline() {
    const auto windowSize = GetWindowSize();

    renderContext = std::make_unique<RenderContext>(graphicsDevice, frameCommandList);
    renderContext->SetWindowSize(windowSize);

    Vertix::RenderPipelineBuilder renderPipelineBuilder { graphicsDevice, frameCommandList, renderContext.get() };
    {
        // Configure SwapChain
        renderPipelineBuilder.SwapChain.swapChainPtr = swapChain;
        renderPipelineBuilder.SwapChain.frameRTVDesc = D3D12_RENDER_TARGET_VIEW_DESC {
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
        };

        constexpr auto depthClearValue = D3D12_CLEAR_VALUE { .Format = DXGI_FORMAT_D32_FLOAT, .DepthStencil = { .Depth = 1.0f, .Stencil = 0 } };

        renderPipelineBuilder.Textures.Add<Vertix::DrawColorSampleAccessor>("GBuffer.Normal", CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R16G16B16A16_FLOAT, VERTIX_VECTOR2D_EXPAND(windowSize)));
        renderPipelineBuilder.Textures.Add<Vertix::DrawColorSampleAccessor>("GBuffer.Albedo", CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, VERTIX_VECTOR2D_EXPAND(windowSize)));
        renderPipelineBuilder.Textures.Add<Vertix::DrawColorSampleAccessor>("GBuffer.OcclusionRoughnessMetallic", CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM, VERTIX_VECTOR2D_EXPAND(windowSize)));
        renderPipelineBuilder.Textures.Add<Vertix::DrawDepthSampleAccessor>("GBuffer.Depth", CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32_TYPELESS, VERTIX_VECTOR2D_EXPAND(windowSize)), true, &depthClearValue);

        renderPipelineBuilder.Textures.Add<Vertix::DrawDepthSampleAccessor>("Shadow.Depth", CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32_TYPELESS, renderContext->ShadowMapSize, renderContext->ShadowMapSize, CASCADE_NUM), false, &depthClearValue);
        renderPipelineBuilder.Textures.Add<Vertix::DrawColorSampleAccessor>("Shadow.Mask", CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R16_FLOAT, VERTIX_VECTOR2D_EXPAND(windowSize)));

        D3D12_DEPTH_STENCIL_VIEW_DESC gDepthDSVDesc {};
        gDepthDSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
        gDepthDSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        gDepthDSVDesc.Texture2D.MipSlice = 0;

        D3D12_DEPTH_STENCIL_VIEW_DESC shadowDepthDSVDesc {};
        shadowDepthDSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
        shadowDepthDSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        shadowDepthDSVDesc.Texture2DArray.ArraySize = CASCADE_NUM;
        shadowDepthDSVDesc.Texture2DArray.FirstArraySlice = 0;
        shadowDepthDSVDesc.Texture2DArray.MipSlice = 0;

        renderPipelineBuilder.Views.AddExplicit<Vertix::DepthStencil>("GBuffer.Depth.DSV", "GBuffer.Depth", gDepthDSVDesc);
        renderPipelineBuilder.Views.AddExplicit<Vertix::ShaderResource>("GBuffer.Depth.SRV", "GBuffer.Depth",
            CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT));

        renderPipelineBuilder.Views.AddExplicit<Vertix::DepthStencil>("Shadow.Depth.DSV", "Shadow.Depth", shadowDepthDSVDesc);
        renderPipelineBuilder.Views.AddExplicit<Vertix::ShaderResource>("Shadow.Depth.SRV", "Shadow.Depth",
            CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2DArray(DXGI_FORMAT_R32_FLOAT, CASCADE_NUM, 1));

        renderPipelineBuilder.Passes.Add<GeometryPass>([](auto &builder) { builder
            .DeclareWrite("GBuffer.Normal", &GeometryPass::gNormalRTV)
            .DeclareWrite("GBuffer.Albedo", &GeometryPass::gAlbedoRTV)
            .DeclareWrite("GBuffer.OcclusionRoughnessMetallic", &GeometryPass::gORMRTV)
            .DeclareWriteExplicit("GBuffer.Depth.DSV", &GeometryPass::gDepthDSV, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        });

        renderPipelineBuilder.Passes.Add<AmbientOcclusionPass>([](auto &builder) { builder
            .DeclareReadExplicit("GBuffer.Depth.SRV", &AmbientOcclusionPass::gDepthSRV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            .DeclareRead("GBuffer.Normal", &AmbientOcclusionPass::gNormalSRV)
            .DeclareWrite("GBuffer.OcclusionRoughnessMetallic", &AmbientOcclusionPass::gORMRTV);
        });

        renderPipelineBuilder.Passes.Add<ShadowGeometryPass>([] (auto &builder) { builder
            .DeclareWriteExplicit("Shadow.Depth.DSV", &ShadowGeometryPass::shadowDepthDSV, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        });

        renderPipelineBuilder.Passes.Add<ShadowPass>([] (auto &builder) { builder
            .DeclareReadExplicit("Shadow.Depth.SRV", &ShadowPass::shadowDepthSRV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            .DeclareRead("GBuffer.Normal", &ShadowPass::gNormalSRV)
            .DeclareReadExplicit("GBuffer.Depth.SRV", &ShadowPass::gDepthSRV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            .DeclareWrite("Shadow.Mask", &ShadowPass::shadowMaskRTV);
        });

        renderPipelineBuilder.Passes.Add<LightingPass>([](auto &builder) { builder
            .DeclareRead("GBuffer.Normal", &LightingPass::gNormalSRV)
            .DeclareRead("GBuffer.Albedo", &LightingPass::gAlbedoSRV)
            .DeclareRead("GBuffer.OcclusionRoughnessMetallic", &LightingPass::gORMSRV)
            .DeclareReadExplicit("GBuffer.Depth.SRV", &LightingPass::gDepthSRV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            .DeclareRead("Shadow.Mask", &LightingPass::shadowMaskSRV)
            .DeclareSwapChainWrite(&LightingPass::currentFrameRTV);
        });

        renderPipelineBuilder.Passes.Add<ImGuiPass>([](auto &builder) { builder
            .DeclareSwapChainWrite(&ImGuiPass::currentFrameRTV);
        }, this);
    }
    renderPipeline = renderPipelineBuilder.Build();
    renderContext->viewport    = renderPipeline->GetD3D12Viewport();
    renderContext->scissorRect = renderPipeline->GetD3D12ScissorRect();
}

void MainWindow::OnInitialize() {
    BuildRenderPipeline();
    imGuiIO = &ImGui::GetIO();

    defaultPositionController.AttachObject(renderContext->GetPerspectiveCamera());
    defaultRotationController.AttachObject(renderContext->GetPerspectiveCamera());

    defaultPositionController.Speed *= 3.0;
    defaultRotationController.Sensitivity *= 1.5;

    graphicsDevice->CreateCommandQueue(copyCommandQueue, { .Type = D3D12_COMMAND_LIST_TYPE_COPY, .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE });
    graphicsDevice->CreateCommandQueue(computeCommandQueue, { .Type = D3D12_COMMAND_LIST_TYPE_COMPUTE, .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE });

    // https://github.com/qian-o/GLTF-Assets/tree/main/Bistro
    const auto directory = std::filesystem::path("F:\\GLTF-Assets-main\\Bistro");
    const auto fileName = "BistroExterior.gltf";

    const std::function modelMaterialLoadCallback = [
        materialPool    = &renderContext->materialPool,
        texturePool     = &renderContext->texturePool,
        graphicsDevice  = graphicsDevice,
        copyQueue       = copyCommandQueue,
        computeQueue    = computeCommandQueue,
        dispatcherQueue = &dispatcherQueue,
        directory
    ] (Vertix::Engine::ModelMaterialLoadCallbackContext* context) -> void {
        Vertix::Engine::TextureAsyncLoader textureAsyncLoader {texturePool, graphicsDevice, copyQueue, computeQueue};
        for (const auto &[aiMaterial, name] : context->Materials) {
            const auto materialHandle = materialPool->Allocate(std::make_unique<Vertix::Engine::DefaultPBRMaterial>());
            const auto material = materialPool->GetAs<Vertix::Engine::DefaultPBRMaterial>(materialHandle);

            material->ReadPropertiesFromAssimp(aiMaterial);
            material->ReadTexturesFromAssimp(aiMaterial, [materialPool, materialHandle, &textureAsyncLoader, directory] (const aiString &path, const aiTextureType textureType, Vertix::TextureHandle* destPtr) -> void {
                textureAsyncLoader.LoadTextureAsync(directory / std::string(path.C_Str()),
                    nullptr, Vertix::Engine::DefaultPBRMaterial::GetWicLoaderFlags(textureType) | DirectX::WIC_LOADER_MIP_RESERVE,
                    [destPtr, materialPool, materialHandle] (const Vertix::TextureHandle textureHandle) {
                        *destPtr = textureHandle;
                        materialPool->MarkDirty(materialHandle);
                    }
                );
            });

            context->MaterialHandles.emplace_back(materialHandle);
        }
        textureAsyncLoader.ExecuteAsync(dispatcherQueue);
    };

    Vertix::Engine::ModelAsyncLoader modelAsyncLoader {&renderContext->modelPool, graphicsDevice, copyCommandQueue, computeCommandQueue/*, modelMaterialLoadCallback*/};
    Vertix::Engine::ModelLoadOptions options{};
    options.AssimpPostProcessSteps |=
        aiProcess_OptimizeGraph |
        aiProcess_RemoveRedundantMaterials;

    modelAsyncLoader.LoadModelAsync((directory / fileName).string(), options, [
        modelPool = &renderContext->modelPool,
        sceneObjects = &renderContext->sceneObjects
    ] (const Vertix::ModelHandle handle) -> void {
        auto* model = modelPool->Get(handle);
        auto* sceneObject = sceneObjects->emplace_back(std::make_unique<Vertix::Engine::SceneObject3D>()).get();

        sceneObject->SceneModel = model;
        sceneObject->SetScale(model->Transformation.Scale);
        sceneObject->SetPosition(model->Transformation.Position);
        sceneObject->SetOrientation(model->Transformation.Orientation);
    });

    modelAsyncLoader.ExecuteAsync(&dispatcherQueue);
}

void MainWindow::OnRender(const double deltaTime) {
    if (GetWindowState() == Vertix::Minimized) return;

    dispatcherQueue.FlushQueue();
    renderContext->materialPool.FlushDirty();
    renderPipeline->Execute();
}

void MainWindow::OnUpdate(const double deltaTime) {
    if (!GetFocusingState()) return;

    if (!imGuiIO || !imGuiIO->WantCaptureMouse) mouseControllerInput.Update(deltaTime);
    if (!imGuiIO || !imGuiIO->WantCaptureKeyboard) keyboardControllerInput.Update(deltaTime);

    renderContext->UpdateFrameConstants();
    renderContext->UpdateLightConstants();
    renderContext->UpdateCascadeShadowConstants();
    renderContext->UpdateObjectConstants();
}

void MainWindow::OnResized(const Vertix::Vector2D<unsigned> &size) {
    renderContext->SetWindowSize(size);
    renderPipeline->Resize(size);
}

void MainWindow::OnFocusLost() {
    if (mouseControllerInput.EnableRotating) {
        ShowCursor(true);
        mouseControllerInput.EnableRotating = false;
    }
}
