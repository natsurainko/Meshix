//
// Created by Natsurainko on 2026/1/27.
//

#include "Rendering/RenderPipeline.h"

#include <d3dcompiler.h>
#include <d3d12/d3dx12_barriers.h>
#include <Vertix/Graphics/FrameCommandList.h>
#include <Vertix/Graphics/SwapChain.h>
#include <Vertix/Windowing/GameWindow.h>

#include "Rendering/Passes/AmbientOcclusionPass.h"
#include "Rendering/Passes/GeometryPass.h"
#include "Rendering/Passes/ImGuiPass.h"
#include "Rendering/Passes/LightingPass.h"
#include "Rendering/Passes/ShadowGeometryPass.h"
#include "Rendering/Passes/ShadowPass.h"

RenderPipeline::RenderPipeline(
    Vertix::GraphicsDevice* graphicsDevice,
    Vertix::FrameCommandList* commandList,
    Vertix::GameWindow* gameWindow) : Vertix::RenderPipeline<RenderContext>(graphicsDevice, commandList->GetD3D12GraphicsCommandList()) {
    swapChain = gameWindow->GetSwapChain();
    window = gameWindow;
    frameCommandList = commandList;

    renderContext = new RenderContext(graphicsDevice);
    renderContext->SetWindowSize(window->GetWindowSize());

    // release after command list executed
    Vertix::ResourceUploadHeap resourceUploadHeap{};
    frameCommandList->BeginCommand(nullptr);
    renderContext->fullScreenVertex = std::unique_ptr<Vertix::VertexBuffer>(
    Vertix::VertexBuffer::CreateFullScreenRect(graphicsDevice, frameCommandList, resourceUploadHeap));
    frameCommandList->EndCommand();
    frameCommandList->WaitForCommand();

    CreateAddPass<GeometryPass>();
    CreateAddPass<ShadowGeometryPass>();
    CreateAddPass<ShadowPass>();
    CreateAddPass<AmbientOcclusionPass>();
    CreateAddPass<LightingPass>(swapChain);
    CreateAddPass<ImGuiPass>(window);
}

void RenderPipeline::Execute() {
    if (window->GetWindowState() == Vertix::Minimized) return;

    const auto rtvResource = swapChain->GetCurrentFrameRenderTargetResource().Get();

    const auto toRTBarrier = CD3DX12_RESOURCE_BARRIER::Transition(rtvResource, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    const auto toPresBarrier = CD3DX12_RESOURCE_BARRIER::Transition(rtvResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    commandList->RSSetViewports(1, &renderContext->viewport);
    commandList->RSSetScissorRects(1, &renderContext->scissorRect);
    commandList->ResourceBarrier(1, &toRTBarrier);
    Vertix::RenderPipeline<RenderContext>::Execute();
    commandList->ResourceBarrier(1, &toPresBarrier);
}

void RenderPipeline::Resize(const Vertix::Vector2D<unsigned> &size) {
    frameCommandList->WaitForCommand();
    swapChain->Resize(size);
    renderContext->SetWindowSize(size);

    for (const auto &pass : renderPasses) {
        pass->Resize(size);
    }
}
