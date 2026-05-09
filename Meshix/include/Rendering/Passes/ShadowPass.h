//
// Created by Natsurainko on 2026/1/29.
//

#ifndef MESHIX_DIRECTIONALSHADOWPASS_H
#define MESHIX_DIRECTIONALSHADOWPASS_H

#include <Vertix/Rendering/RenderResourceView.h>
#include <Vertix/Rendering/Pipeline/RenderPass.h>

#include "Rendering/RenderContext.h"

class ShadowPass : public Vertix::RenderPass<RenderContext> {
public:
    void Initialize(ID3D12Device10 *device) override;
    void Execute(ID3D12GraphicsCommandList5* commandList) override;

    const Vertix::RenderResourceView<Vertix::ShaderResource>* gDepthSRV;
    const Vertix::RenderResourceView<Vertix::ShaderResource>* gNormalSRV;
    const Vertix::RenderResourceView<Vertix::ShaderResource>* shadowDepthSRV;
    const Vertix::RenderResourceView<Vertix::RenderTarget>* shadowMaskRTV;
private:
    ID3D12DescriptorHeap* descriptorHeap = nullptr;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

    float ShadowMapTexel[2] = {};
};

#endif //MESHIX_DIRECTIONALSHADOWPASS_H
