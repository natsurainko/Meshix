//
// Created by Natsurainko on 2026/3/14.
//

#ifndef MESHIX_AMBIENTOCCLUSIONPASS_H
#define MESHIX_AMBIENTOCCLUSIONPASS_H

#include <Vertix/Rendering/RenderPass.hpp>
#include <Vertix/Rendering/RenderTargetView.h>

#include "Rendering/RenderContext.h"

class AmbientOcclusionPass : public Vertix::RenderPass<RenderContext> {
public:
    ~AmbientOcclusionPass() override;

    void Initialize(
        Vertix::GraphicsDevice* device,
        RenderContext* context) override;

    void Execute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> &commandList) override;
    void Resize(const Vertix::Vector2D<unsigned> &size) override;

private:
    Vertix::RenderTargetView* renderTargetView = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle{};

    D3D12_RESOURCE_BARRIER srvBarrier{};

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
};

#endif //MESHIX_AMBIENTOCCLUSIONPASS_H
