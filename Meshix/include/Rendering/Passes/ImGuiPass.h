//
// Created by Natsurainko on 2026/3/7.
//

#ifndef MESHIX_IMGUIPASS_H
#define MESHIX_IMGUIPASS_H

#include <imgui/imgui.h>
#include <Vertix/Rendering/RenderResourceView.h>
#include <Vertix/Rendering/Pipeline/RenderPass.h>
#include <Vertix/Windowing/GameWindow.h>

#include "Rendering/RenderContext.h"

class ImGuiPass : public Vertix::RenderPass {
public:
    explicit ImGuiPass(
        const Vertix::GameWindow* window,
        RenderContext* renderContext)
    : renderContext(renderContext), window(window), guiContext(window->GetSwapChain()) {}

    ~ImGuiPass() override;

    void Initialize(ID3D12Device10* device) override;
    void Execute(ID3D12GraphicsCommandList5* commandList) override;

    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::RenderTarget>* currentFrameRTV = nullptr;

private:
    ImGuiIO* io = nullptr;
    RenderContext* renderContext;
    const Vertix::GameWindow* window;
    GuiContext guiContext;
};

#endif //MESHIX_IMGUIPASS_H
