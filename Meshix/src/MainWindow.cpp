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
#include "Rendering/Passes/CullingPass.h"
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

        builder.Buffers.StructuredBuffer<MeshCullingConstants>("Structured.MeshCulling", 65535);
        builder.Buffers.StructuredBuffer<MeshIndirectCommand>("Structured.MeshIndirect", 65535);
        builder.Buffers.StructuredBuffer<ObjectConstants>("Structured.Object", 4096);
        builder.Buffers.StructuredBuffer<MaterialConstants>("Structured.Material", 4096);

        builder.Buffers.Add("Typed.VisibleCount", CD3DX12_RESOURCE_DESC::Buffer(4));

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

        builder.Passes.Add<CullingPass>([&](auto &pb) { pb
            .ReadStructuredBuffer("Structured.MeshCulling", &CullingPass::meshCullingStructured)
            .ReadStructuredBuffer("Structured.Object", &CullingPass::objectStructured)
            .ReadStructuredBuffer("Structured.Material", &CullingPass::materialStructured)
            .ReadConstantBuffer("Constants.Frame", &CullingPass::frameConstants)
            .Write("Structured.MeshIndirect", &CullingPass::indirectCommandsUAV, CD3DX12_UNORDERED_ACCESS_VIEW_DESC::StructuredBuffer(65535, sizeof(MeshIndirectCommand)))
            .Write("Structured.MeshIndirect", &CullingPass::indirectCommandsBuffer, Vertix::RenderResourceUsage::UnorderedAccess)
            .Write("Typed.VisibleCount", &CullingPass::visibleCountUAV, CD3DX12_UNORDERED_ACCESS_VIEW_DESC::TypedBuffer(DXGI_FORMAT_R32_UINT, 1))
            .Write("Typed.VisibleCount", &CullingPass::visibleCountBuffer, Vertix::RenderResourceUsage::UnorderedAccess);
        }, renderContext.get());

        builder.Passes.Add<GeometryPass>([&](auto &pb) { pb
            .ReadConstantBuffer("Constants.Frame", &GeometryPass::frameConstants)
            .ReadStructuredBuffer("Structured.Material", &GeometryPass::materialStructured)
            .ReadStructuredBuffer("Structured.Object", &GeometryPass::objectStructured)
            .Read("Structured.MeshIndirect", &GeometryPass::indirectCommandsBuffer, Vertix::RenderResourceUsage::IndirectArgumentRead)
            .Read("Typed.VisibleCount", &GeometryPass::visibleCountBuffer, Vertix::RenderResourceUsage::IndirectArgumentRead)
            .Write("GBuffer.Normal", &GeometryPass::gNormalRTV)
            .Write("GBuffer.Albedo", &GeometryPass::gAlbedoRTV)
            .Write("GBuffer.OcclusionRoughnessMetallic", &GeometryPass::gORMRTV)
            .Write("GBuffer.Depth", &GeometryPass::gDepthDSV, gDepthDSVDesc);
        }, renderContext.get());

        builder.Passes.Add<AmbientOcclusionPass>([&](auto &pb) { pb
            .ReadConstantBuffer("Constants.Frame", &AmbientOcclusionPass::frameConstants)
            .Read("GBuffer.Depth", &AmbientOcclusionPass::gDepthSRV, gDepthSRVDesc)
            .Read("GBuffer.Normal", &AmbientOcclusionPass::gNormalSRV)
            .Write("GBuffer.OcclusionRoughnessMetallic", &AmbientOcclusionPass::gORMRTV);
        }, renderContext.get());

        builder.Passes.Add<ShadowGeometryPass>([&] (auto &pb) { pb
            .ReadConstantBuffer("Constants.CascadeShadow", &ShadowGeometryPass::cascadeShadowConstants)
            .Write("Shadow.Depth", &ShadowGeometryPass::shadowDepthDSV, shadowDepthDSVDesc);
        }, renderContext.get());

        builder.Passes.Add<ShadowPass>([&] (auto &pb) { pb
            .ReadConstantBuffer("Constants.Frame", &ShadowPass::frameConstants)
            .ReadConstantBuffer("Constants.Light", &ShadowPass::lightConstants)
            .ReadConstantBuffer("Constants.CascadeShadow", &ShadowPass::cascadeShadowConstants)
            .Read("Shadow.Depth", &ShadowPass::shadowDepthSRV, shadowDepthSRVDesc)
            .Read("GBuffer.Normal", &ShadowPass::gNormalSRV)
            .Read("GBuffer.Depth", &ShadowPass::gDepthSRV, gDepthSRVDesc)
            .Write("Shadow.Mask", &ShadowPass::shadowMaskRTV);
        }, renderContext.get());

        builder.Passes.Add<LightingPass>([&](auto &pb) { pb
            .ReadConstantBuffer("Constants.Frame", &LightingPass::frameConstants)
            .ReadConstantBuffer("Constants.Light", &LightingPass::lightConstants)
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
    renderContext->frameConstantsBuffer         = renderPipeline->GetConstantBuffer<FrameConstants>("Constants.Frame");
    renderContext->lightConstantsBuffer         = renderPipeline->GetConstantBuffer<LightConstants>("Constants.Light");
    renderContext->cascadeShadowConstantsBuffer = renderPipeline->GetConstantBuffer<CascadeShadowConstants>("Constants.CascadeShadow");
    renderContext->materialStructuredBuffer    = renderPipeline->GetStructuredBuffer<MaterialConstants>("Structured.Material");
    renderContext->objectStructuredBuffer      = renderPipeline->GetStructuredBuffer<ObjectConstants>("Structured.Object");
    renderContext->meshCullingStructuredBuffer = renderPipeline->GetStructuredBuffer<MeshCullingConstants>("Structured.MeshCulling");
    renderContext->modelPool    = std::make_unique<Vertix::ModelPool>();
    renderContext->objectPool   = std::make_unique<Vertix::ResourcePool<Vertix::Engine::SceneObject3D, ObjectHandle>>(4096);
    renderContext->texturePool  = std::make_unique<Vertix::TexturePool>(renderContext->sharedDescriptorHeap->AllocateRange(2048));
    renderContext->materialPool = std::make_unique<Vertix::MaterialPool<MaterialConstants>>(renderContext->materialStructuredBuffer);
}

void MainWindow::OnInitialize() {
    BuildRenderPipeline();
    imGuiIO = &ImGui::GetIO();
    commandList = frameCommandList->GetD3D12GraphicsCommandList().Get();

    defaultPositionController.AttachObject(renderContext->GetPerspectiveCamera());
    defaultRotationController.AttachObject(renderContext->GetPerspectiveCamera());

    defaultPositionController.Speed *= 3.0;
    defaultRotationController.Sensitivity *= 1.5;
}

void MainWindow::OnRender(const double deltaTime) {
    if (GetWindowState() == Vertix::Minimized) return;

    renderContext->materialPool->FlushDirty(commandList);
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
