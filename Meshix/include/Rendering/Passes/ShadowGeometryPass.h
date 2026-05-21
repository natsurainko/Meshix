//
// Created by Natsurainko on 2026/1/29.
//

#ifndef MESHIX_LIGHTVIEWDEPTHPASS_H
#define MESHIX_LIGHTVIEWDEPTHPASS_H

#include <Vertix/Graphics/DescriptorView.h>
#include <Vertix/Rendering/Pipeline/RenderPass.h>

#include "Rendering/RenderContext.h"

class ShadowGeometryPass : public Vertix::RenderPass {
public:
    explicit ShadowGeometryPass(RenderContext* renderContext) : renderContext(renderContext) {}

    void Initialize(ID3D12Device10* device) override;
    void Execute(ID3D12GraphicsCommandList5* commandList) override;

    Vertix::DescriptorView<Vertix::RenderResourceUsage::DepthWrite> shadowDepthDSV;

    D3D12_GPU_VIRTUAL_ADDRESS cascadeShadowConstants = {};

private:
    RenderContext* renderContext;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

    CD3DX12_VIEWPORT viewport{};
    CD3DX12_RECT scissorRect{};
};

#endif //MESHIX_LIGHTVIEWDEPTHPASS_H
