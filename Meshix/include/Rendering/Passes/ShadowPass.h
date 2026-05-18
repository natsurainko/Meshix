//
// Created by Natsurainko on 2026/1/29.
//

#ifndef MESHIX_DIRECTIONALSHADOWPASS_H
#define MESHIX_DIRECTIONALSHADOWPASS_H

#include <Vertix/Rendering/RenderResourceView.h>
#include <Vertix/Rendering/Pipeline/RenderPass.h>

#include "Rendering/RenderContext.h"

class ShadowPass : public Vertix::RenderPass {
public:
    explicit ShadowPass(RenderContext* renderContext) : renderContext(renderContext) {}

    void Initialize(ID3D12Device10 *device) override;
    void Execute(ID3D12GraphicsCommandList5* commandList) override;

    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::ShaderResource>* gDepthSRV = nullptr;
    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::ShaderResource>* gNormalSRV = nullptr;
    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::ShaderResource>* shadowDepthSRV = nullptr;
    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::RenderTarget>* shadowMaskRTV = nullptr;

    D3D12_GPU_VIRTUAL_ADDRESS frameConstantsAddress = {};
    D3D12_GPU_VIRTUAL_ADDRESS lightConstantsAddress = {};
    D3D12_GPU_VIRTUAL_ADDRESS cascadeShadowConstantsAddress = {};

private:
    struct TextureHandles {
        uint gNormalHandle;
        uint gDepthHandle;
        uint ShadowDepthHandle;
    } handles = {};

    RenderContext* renderContext;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
};

#endif //MESHIX_DIRECTIONALSHADOWPASS_H
