//
// Created by Natsurainko on 2026/1/29.
//

#ifndef MESHIX_DIRECTIONALSHADOWPASS_H
#define MESHIX_DIRECTIONALSHADOWPASS_H

#include <Vertix/Rendering/RenderPass.hpp>
#include <Vertix/Rendering/RenderTargetView.h>

#include "Rendering/RenderContext.h"

class ShadowPass : public Vertix::RenderPass<RenderContext> {
public:
    ~ShadowPass() override;

    void Initialize(
        Vertix::GraphicsDevice *device,
        RenderContext *context) override;

    void Execute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> &commandList) override;
    void Resize(const Vertix::Vector2D<unsigned> &size) override;

private:
    Vertix::RenderTargetView* renderTargetView = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle{};

    D3D12_RESOURCE_BARRIER srvBarrier{};
    D3D12_RESOURCE_BARRIER srvToDsvBarrier{};
    D3D12_RESOURCE_BARRIER dsvToSrvBarrier{};

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

    float ShadowMapTexel[2] = {};
};

#endif //MESHIX_DIRECTIONALSHADOWPASS_H
