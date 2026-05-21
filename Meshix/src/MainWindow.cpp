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

    Vertix::RenderPipelineBuilder builder { graphicsDevice, frameCommandList };
    {
        // Configure SwapChain
        builder.SwapChain.ptr = swapChain;
        builder.SwapChain.swapChainResourceName = "SwapChainBackBuffer";

        builder.Descriptors.reservedSharedDescriptorCount = 2048 + 1;
        builder.Descriptors.onDescriptorSetCreated = [&](const Vertix::DescriptorHeapSet* heapSet) {
            renderContext->sharedDescriptorHeap = (*heapSet)[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
        };

        constexpr auto depthClearValue = D3D12_CLEAR_VALUE { .Format = DXGI_FORMAT_D32_FLOAT, .DepthStencil = { .Depth = 1.0f, .Stencil = 0 } };

        builder.Textures.Add("GBuffer.Normal", CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R16G16B16A16_FLOAT, VERTIX_VECTOR2D_EXPAND(windowSize)), D3D12_CLEAR_VALUE { .Format = DXGI_FORMAT_R16G16B16A16_FLOAT, .Color = { 0.0f, 0.0f, 0.0f, 0.0f } });
        builder.Textures.Add("GBuffer.Albedo", CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, VERTIX_VECTOR2D_EXPAND(windowSize)), D3D12_CLEAR_VALUE { .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, .Color = { 0.0f, 0.0f, 0.0f, 0.0f } });
        builder.Textures.Add("GBuffer.OcclusionRoughnessMetallic", CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM, VERTIX_VECTOR2D_EXPAND(windowSize)), D3D12_CLEAR_VALUE { .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .Color = { 0.0f, 0.0f, 0.0f, 0.0f } });
        builder.Textures.Add("GBuffer.Depth", CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32_TYPELESS, VERTIX_VECTOR2D_EXPAND(windowSize)), depthClearValue);

        builder.Textures.Add("Shadow.Depth", CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32_TYPELESS, renderContext->ShadowMapSize, renderContext->ShadowMapSize, CASCADE_NUM), depthClearValue, false);
        builder.Textures.Add("Shadow.Mask", CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R16_FLOAT, VERTIX_VECTOR2D_EXPAND(windowSize)), D3D12_CLEAR_VALUE { .Format = DXGI_FORMAT_R16_FLOAT, .Color = { 1.0f } });

        builder.Buffers.ConstantBuffer<FrameConstants>("Constants.Frame");
        builder.Buffers.ConstantBuffer<LightConstants>("Constants.Light");
        builder.Buffers.ConstantBuffer<CascadeShadowConstants>("Constants.CascadeShadow");
        builder.Buffers.StructuredBuffer<ObjectConstants>("Structured.Objects", 2048);

        D3D12_DEPTH_STENCIL_VIEW_DESC gDepthDSVDesc {};
        gDepthDSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
        gDepthDSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        gDepthDSVDesc.Texture2D.MipSlice = 0;

        D3D12_DEPTH_STENCIL_VIEW_DESC shadowDepthDSVDesc {};
        shadowDepthDSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
        shadowDepthDSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        shadowDepthDSVDesc.Texture2DArray.ArraySize = CASCADE_NUM;

        const auto gDepthSRVDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT);
        const auto shadowDepthSRVDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2DArray(DXGI_FORMAT_R32_FLOAT, CASCADE_NUM, 1);

        builder.Passes.Add<GeometryPass>([&](auto &pb) { pb
            .Read("Constants.Frame", &GeometryPass::frameConstants, Vertix::RenderResourceUsage::ConstantBuffer)
            .Write("GBuffer.Normal", &GeometryPass::gNormalRTV)
            .Write("GBuffer.Albedo", &GeometryPass::gAlbedoRTV)
            .Write("GBuffer.OcclusionRoughnessMetallic", &GeometryPass::gORMRTV)
            .Write("GBuffer.Depth", &GeometryPass::gDepthDSV, gDepthDSVDesc);
        }, renderContext.get());

        builder.Passes.Add<AmbientOcclusionPass>([&](auto &pb) { pb
            .Read("Constants.Frame", &AmbientOcclusionPass::frameConstants, Vertix::RenderResourceUsage::ConstantBuffer)
            .Read("GBuffer.Depth", &AmbientOcclusionPass::gDepthSRV, gDepthSRVDesc)
            .Read("GBuffer.Normal", &AmbientOcclusionPass::gNormalSRV)
            .Write("GBuffer.OcclusionRoughnessMetallic", &AmbientOcclusionPass::gORMRTV);
        }, renderContext.get());

        builder.Passes.Add<ShadowGeometryPass>([&] (auto &pb) { pb
            .Read("Constants.CascadeShadow", &ShadowGeometryPass::cascadeShadowConstants, Vertix::RenderResourceUsage::ConstantBuffer)
            .Write("Shadow.Depth", &ShadowGeometryPass::shadowDepthDSV, shadowDepthDSVDesc);
        }, renderContext.get());

        builder.Passes.Add<ShadowPass>([&] (auto &pb) { pb
            .Read("Constants.Frame", &ShadowPass::frameConstants, Vertix::RenderResourceUsage::ConstantBuffer)
            .Read("Constants.Light", &ShadowPass::lightConstants, Vertix::RenderResourceUsage::ConstantBuffer)
            .Read("Constants.CascadeShadow", &ShadowPass::cascadeShadowConstants, Vertix::RenderResourceUsage::ConstantBuffer)
            .Read("Shadow.Depth", &ShadowPass::shadowDepthSRV, shadowDepthSRVDesc)
            .Read("GBuffer.Normal", &ShadowPass::gNormalSRV)
            .Read("GBuffer.Depth", &ShadowPass::gDepthSRV, gDepthSRVDesc)
            .Write("Shadow.Mask", &ShadowPass::shadowMaskRTV);
        }, renderContext.get());

        builder.Passes.Add<LightingPass>([&](auto &pb) { pb
            .Read("Constants.Frame", &LightingPass::frameConstants, Vertix::RenderResourceUsage::ConstantBuffer)
            .Read("Constants.Light", &LightingPass::lightConstants, Vertix::RenderResourceUsage::ConstantBuffer)
            .Read("GBuffer.Normal", &LightingPass::gNormalSRV)
            .Read("GBuffer.Albedo", &LightingPass::gAlbedoSRV)
            .Read("GBuffer.OcclusionRoughnessMetallic", &LightingPass::gORMSRV)
            .Read("GBuffer.Depth", &LightingPass::gDepthSRV, gDepthSRVDesc)
            .Read("Shadow.Mask", &LightingPass::shadowMaskSRV)
            .Write("SwapChainBackBuffer", &LightingPass::currentFrameRTV);
        }, renderContext.get());

        builder.Passes.Add<ImGuiPass>([](auto &pb) { pb
            .Write("SwapChainBackBuffer", &ImGuiPass::currentFrameRTV)
            .template DependsAfter<LightingPass>();
        }, this, renderContext.get());
    }
    renderPipeline = builder.Build();
    renderContext->viewport     = renderPipeline->GetD3D12Viewport();
    renderContext->scissorRect  = renderPipeline->GetD3D12ScissorRect();
    renderContext->texturePool  = std::make_unique<Vertix::TexturePool>(renderContext->sharedDescriptorHeap->AllocateRange(2048));
    renderContext->modelPool    = std::make_unique<Vertix::ModelPool>();
    renderContext->materialPool = std::make_unique<Vertix::MaterialPool<Vertix::Engine::DefaultMaterialConstants>>(graphicsDevice, 2048);
    renderContext->frameConstantsBuffer         = renderPipeline->GetConstantBuffer<FrameConstants>("Constants.Frame");
    renderContext->lightConstantsBuffer         = renderPipeline->GetConstantBuffer<LightConstants>("Constants.Light");
    renderContext->cascadeShadowConstantsBuffer = renderPipeline->GetConstantBuffer<CascadeShadowConstants>("Constants.CascadeShadow");
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
    const auto directory = std::filesystem::path(R"(F:\glTF-Sample-Assets\Models\Sponza\glTF)");
    const auto fileName = "Sponza.gltf";

    const std::function modelMaterialLoadCallback = [
        materialPool    = renderContext->materialPool.get(),
        texturePool     = renderContext->texturePool.get(),
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
                    [destPtr, materialPool, materialHandle] (const Vertix::TextureHandle &textureHandle) {
                        *destPtr = textureHandle;
                        materialPool->MarkDirty(materialHandle);
                    }
                );
            });

            context->MaterialHandles.emplace_back(materialHandle);
        }
        textureAsyncLoader.ExecuteAsync(dispatcherQueue);
    };

    Vertix::Engine::ModelAsyncLoader modelAsyncLoader {renderContext->modelPool.get(), graphicsDevice, copyCommandQueue, computeCommandQueue, modelMaterialLoadCallback};
    Vertix::Engine::ModelLoadOptions options{};
    options.AssimpPostProcessSteps |=
        aiProcess_OptimizeGraph |
        aiProcess_RemoveRedundantMaterials;

    modelAsyncLoader.LoadModelAsync((directory / fileName).string(), options, [
        modelPool = renderContext->modelPool.get(),
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
    renderContext->materialPool->FlushDirty();
    renderContext->OnFrameUpdate();
    renderPipeline->Execute();
}

void MainWindow::OnUpdate(const double deltaTime) {
    if (!GetFocusingState()) return;

    if (!imGuiIO || !imGuiIO->WantCaptureMouse) mouseControllerInput.Update(deltaTime);
    if (!imGuiIO || !imGuiIO->WantCaptureKeyboard) keyboardControllerInput.Update(deltaTime);
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
