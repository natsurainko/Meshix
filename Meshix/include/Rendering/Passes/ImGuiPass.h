//
// Created by Natsurainko on 2026/3/7.
//

#ifndef MESHIX_IMGUIPASS_H
#define MESHIX_IMGUIPASS_H

#include <imgui/imgui.h>
#include <Vertix/Rendering/RenderPass.hpp>

#include "Rendering/RenderContext.h"

static Vertix::DescriptorHeap* imguiSrvDescriptorHeap = nullptr;

class ImGuiPass : public Vertix::RenderPass<RenderContext> {
public:
    explicit ImGuiPass(const Vertix::GameWindow* window);
    ~ImGuiPass() override;

    void Initialize(
        Vertix::GraphicsDevice* device,
        RenderContext* context) override;

    void Execute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> &commandList) override;
private:
    const Vertix::GameWindow *window;
    Vertix::SwapChain* swapChain;
    ImGuiIO* io = nullptr;
};

#endif //MESHIX_IMGUIPASS_H
