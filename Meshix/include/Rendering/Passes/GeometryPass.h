//
// Created by Natsurainko on 2026/1/27.
//

#ifndef MESHIX_GEOMETRYPASS_H
#define MESHIX_GEOMETRYPASS_H

#include <Vertix/Rendering/RenderResourceView.h>
#include <Vertix/Rendering/Pipeline/RenderPass.h>

#include "Rendering/RenderContext.h"

class GeometryPass : public Vertix::RenderPass {
public:
    explicit GeometryPass(RenderContext* renderContext) : renderContext(renderContext) {}

    void Initialize(ID3D12Device10* device) override;
    void Execute(ID3D12GraphicsCommandList5* commandList) override;

    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::RenderTarget>* gNormalRTV = nullptr;
    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::RenderTarget>* gAlbedoRTV = nullptr;
    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::RenderTarget>* gORMRTV = nullptr;
    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::DepthStencil>* gDepthDSV = nullptr;

    D3D12_GPU_VIRTUAL_ADDRESS frameConstantsAddress = {};

private:
    RenderContext* renderContext;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
};

#endif //MESHIX_GEOMETRYPASS_H
