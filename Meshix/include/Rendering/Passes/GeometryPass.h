//
// Created by Natsurainko on 2026/1/27.
//

#ifndef MESHIX_GEOMETRYPASS_H
#define MESHIX_GEOMETRYPASS_H

#include <Vertix/Graphics/DescriptorView.h>
#include <Vertix/Rendering/Pipeline/RenderPass.h>

#include "Rendering/RenderContext.h"

class GeometryPass : public Vertix::RenderPass {
public:
    explicit GeometryPass(RenderContext* renderContext) : renderContext(renderContext) {}

    void Initialize(ID3D12Device10* device) override;
    void Execute(ID3D12GraphicsCommandList5* commandList) override;

    Vertix::DescriptorView<Vertix::RenderResourceUsage::RenderTarget> gNormalRTV;
    Vertix::DescriptorView<Vertix::RenderResourceUsage::RenderTarget> gAlbedoRTV;
    Vertix::DescriptorView<Vertix::RenderResourceUsage::RenderTarget> gORMRTV;
    Vertix::DescriptorView<Vertix::RenderResourceUsage::DepthWrite>   gDepthDSV;

    D3D12_GPU_VIRTUAL_ADDRESS frameConstants[2] = {};

    D3D12_GPU_VIRTUAL_ADDRESS objectStructured = {};
    D3D12_GPU_VIRTUAL_ADDRESS materialStructured = {};

    Vertix::RenderResource* indirectCommandsBuffers[1] = {};
    Vertix::RenderResource* indirectCountBuffer;

private:
    RenderContext* renderContext;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[3] = {
        gNormalRTV.cpuHandle,
        gAlbedoRTV.cpuHandle,
        gORMRTV.cpuHandle,
    };

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> commandSignature;
};

#endif //MESHIX_GEOMETRYPASS_H
