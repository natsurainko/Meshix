//
// Created by Natsurainko on 2026/3/7.
//

#ifndef MESHIX_LIGHTINGPASS_H
#define MESHIX_LIGHTINGPASS_H

#include <Vertix/Graphics/DescriptorView.h>
#include <Vertix/Rendering/Pipeline/RenderPass.h>

#include "Rendering/RenderContext.h"

class LightingPass : public Vertix::RenderPass {
public:
    explicit LightingPass(RenderContext* renderContext) : renderContext(renderContext) {}

    void Initialize(ID3D12Device10* device) override;
    void Execute(ID3D12GraphicsCommandList5* commandList) override;

    Vertix::DescriptorView<Vertix::RenderResourceUsage::PixelShaderResource> gNormalSRV;
    Vertix::DescriptorView<Vertix::RenderResourceUsage::PixelShaderResource> gAlbedoSRV;
    Vertix::DescriptorView<Vertix::RenderResourceUsage::PixelShaderResource> gORMSRV;
    Vertix::DescriptorView<Vertix::RenderResourceUsage::PixelShaderResource> gDepthSRV;
    Vertix::DescriptorView<Vertix::RenderResourceUsage::PixelShaderResource> shadowMaskSRV;
    Vertix::DescriptorView<Vertix::RenderResourceUsage::RenderTarget> currentFrameRTV;

    D3D12_GPU_VIRTUAL_ADDRESS frameConstants = {};
    D3D12_GPU_VIRTUAL_ADDRESS lightConstants = {};

private:
    struct TextureHandles {
        uint gNormalHandle;
        uint gAlbedoHandle;
        uint gORMHandle;
        uint gDepthHandle;
        uint ShadowMaskHandle;
    } handles = {};

    RenderContext* renderContext;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
};

#endif //MESHIX_LIGHTINGPASS_H
