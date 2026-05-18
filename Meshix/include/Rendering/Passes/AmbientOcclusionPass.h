//
// Created by Natsurainko on 2026/3/14.
//

#ifndef MESHIX_AMBIENTOCCLUSIONPASS_H
#define MESHIX_AMBIENTOCCLUSIONPASS_H

#include <Vertix/Rendering/RenderResourceView.h>
#include <Vertix/Rendering/Pipeline/RenderPass.h>

#include "Rendering/RenderContext.h"

class AmbientOcclusionPass : public Vertix::RenderPass {
public:
    explicit AmbientOcclusionPass(RenderContext* renderContext) : renderContext(renderContext) {}

    void Initialize(ID3D12Device10* device) override;
    void Execute(ID3D12GraphicsCommandList5* commandList) override;

    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::ShaderResource>* gDepthSRV = nullptr;
    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::ShaderResource>* gNormalSRV = nullptr;
    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::RenderTarget>* gORMRTV = nullptr;

    D3D12_GPU_VIRTUAL_ADDRESS frameConstantsAddress = {};

private:
    struct TextureHandles {
        uint32_t gDepthHandle;
        uint32_t gNormalHandle;
    } handles = {};

    RenderContext* renderContext;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
};

#endif //MESHIX_AMBIENTOCCLUSIONPASS_H
