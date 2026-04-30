//
// Created by Natsurainko on 2026/1/27.
//

#ifndef MESHIX_GEOMETRYPASS_H
#define MESHIX_GEOMETRYPASS_H

#include <array>
#include <Vertix/Rendering/DepthStencilView.h>
#include <Vertix/Rendering/RenderPass.hpp>
#include <Vertix/Rendering/RenderTargetView.h>

#include "Rendering/RenderContext.h"

constexpr std::array renderTargetFormats = {
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R16_FLOAT,
};

class GeometryPass : public Vertix::RenderPass<RenderContext> {
public:
    ~GeometryPass() override;

    void Initialize(
        Vertix::GraphicsDevice* device,
        RenderContext* context) override;

    void Execute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> &commandList) override;
    void Resize(const Vertix::Vector2D<unsigned> &size) override;

private:
    Vertix::DepthStencilView* depthStencilView = nullptr;
    Vertix::RenderTargetView* renderTargetViews[renderTargetFormats.size()] = {};

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{};
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[renderTargetFormats.size()] = {};
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandles[renderTargetFormats.size()] = {};

    D3D12_RESOURCE_BARRIER srvBarriers[renderTargetFormats.size()] = {};

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
};

#endif //MESHIX_GEOMETRYPASS_H
