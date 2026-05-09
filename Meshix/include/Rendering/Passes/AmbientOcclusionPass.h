//
// Created by Natsurainko on 2026/3/14.
//

#ifndef MESHIX_AMBIENTOCCLUSIONPASS_H
#define MESHIX_AMBIENTOCCLUSIONPASS_H

#include <Vertix/Rendering/RenderResourceView.h>
#include <Vertix/Rendering/Pipeline/RenderPass.h>

#include "Rendering/RenderContext.h"

class AmbientOcclusionPass : public Vertix::RenderPass<RenderContext> {
public:
    void Initialize(ID3D12Device10* device) override;
    void Execute(ID3D12GraphicsCommandList5* commandList) override;

    const Vertix::RenderResourceView<Vertix::ShaderResource>* gDepthSRV;
    const Vertix::RenderResourceView<Vertix::ShaderResource>* gNormalSRV;
    const Vertix::RenderResourceView<Vertix::RenderTarget>* gORMRTV;
private:
    ID3D12DescriptorHeap* descriptorHeap = nullptr;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
};

#endif //MESHIX_AMBIENTOCCLUSIONPASS_H
