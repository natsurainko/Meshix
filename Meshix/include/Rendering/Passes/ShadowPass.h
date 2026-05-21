//
// Created by Natsurainko on 2026/1/29.
//

#ifndef MESHIX_DIRECTIONALSHADOWPASS_H
#define MESHIX_DIRECTIONALSHADOWPASS_H

#include <Vertix/Graphics/DescriptorView.h>
#include <Vertix/Rendering/Pipeline/RenderPass.h>

#include "Rendering/RenderContext.h"

class ShadowPass : public Vertix::RenderPass {
public:
    explicit ShadowPass(RenderContext* renderContext) : renderContext(renderContext) {}

    void Initialize(ID3D12Device10 *device) override;
    void Execute(ID3D12GraphicsCommandList5* commandList) override;

    Vertix::DescriptorView<Vertix::RenderResourceUsage::PixelShaderResource> gDepthSRV;
    Vertix::DescriptorView<Vertix::RenderResourceUsage::PixelShaderResource> gNormalSRV;
    Vertix::DescriptorView<Vertix::RenderResourceUsage::PixelShaderResource> shadowDepthSRV;
    Vertix::DescriptorView<Vertix::RenderResourceUsage::RenderTarget> shadowMaskRTV;

    D3D12_GPU_VIRTUAL_ADDRESS frameConstants = {};
    D3D12_GPU_VIRTUAL_ADDRESS lightConstants = {};
    D3D12_GPU_VIRTUAL_ADDRESS cascadeShadowConstants = {};

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
