//
// Created by Natsurainko on 2026/1/27.
//

#ifndef MESHIX_GEOMETRYPASS_H
#define MESHIX_GEOMETRYPASS_H

#include <Vertix/Rendering/RenderResourceView.h>
#include <Vertix/Rendering/Pipeline/RenderPass.h>

#include "Rendering/RenderContext.h"

class GeometryPass : public Vertix::RenderPass<RenderContext> {
public:
    void Initialize(ID3D12Device10* device) override;
    void Execute(ID3D12GraphicsCommandList5* commandList) override;

    const Vertix::RenderResourceView<Vertix::RenderTarget>* gNormalRTV;
    const Vertix::RenderResourceView<Vertix::RenderTarget>* gAlbedoRTV;
    const Vertix::RenderResourceView<Vertix::RenderTarget>* gORMRTV;
    const Vertix::RenderResourceView<Vertix::DepthStencil>* gDepthDSV;

private:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
};

#endif //MESHIX_GEOMETRYPASS_H
