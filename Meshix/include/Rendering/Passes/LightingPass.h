//
// Created by Natsurainko on 2026/3/7.
//

#ifndef MESHIX_LIGHTINGPASS_H
#define MESHIX_LIGHTINGPASS_H

#include <Vertix/Rendering/RenderResourceView.h>
#include <Vertix/Rendering/Pipeline/RenderPass.h>

#include "Rendering/RenderContext.h"

class LightingPass : public Vertix::RenderPass {
public:
    explicit LightingPass(RenderContext* renderContext) : renderContext(renderContext) {}

    void Initialize(ID3D12Device10* device) override;
    void Execute(ID3D12GraphicsCommandList5* commandList) override;

    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::ShaderResource>* gNormalSRV = nullptr;
    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::ShaderResource>* gAlbedoSRV = nullptr;
    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::ShaderResource>* gORMSRV = nullptr;
    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::ShaderResource>* gDepthSRV = nullptr;
    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::ShaderResource>* shadowMaskSRV = nullptr;
    const Vertix::RenderResourceView<Vertix::RenderResourceViewType::RenderTarget>* currentFrameRTV = nullptr;

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
