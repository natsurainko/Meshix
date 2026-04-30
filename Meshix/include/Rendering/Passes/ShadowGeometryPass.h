//
// Created by Natsurainko on 2026/1/29.
//

#ifndef MESHIX_LIGHTVIEWDEPTHPASS_H
#define MESHIX_LIGHTVIEWDEPTHPASS_H

#include <Vertix/Rendering/DepthStencilView.h>
#include <Vertix/Rendering/RenderPass.hpp>

#include "Rendering/RenderContext.h"

class ShadowGeometryPass : public Vertix::RenderPass<RenderContext> {
public:
    ~ShadowGeometryPass() override;

    void Initialize(
        Vertix::GraphicsDevice *device,
        RenderContext *context) override;

    void Execute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> &commandList) override;

private:
    Vertix::DepthStencilView* depthStencilView = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{};
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle{};

    D3D12_RESOURCE_BARRIER srvBarrier{};

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

    CD3DX12_VIEWPORT viewport{};
    CD3DX12_RECT scissorRect{};
};

#endif //MESHIX_LIGHTVIEWDEPTHPASS_H
