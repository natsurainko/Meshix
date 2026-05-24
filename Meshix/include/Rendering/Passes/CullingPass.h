//
// Created by Natsurainko on 2026/5/23.
//

#ifndef MESHIX_CULLINGPASS_H
#define MESHIX_CULLINGPASS_H

#include <Vertix/Graphics/DescriptorView.h>
#include <Vertix/Rendering/Pipeline/RenderPass.h>

#include "Rendering/RenderContext.h"

class CullingPass : public Vertix::RenderPass {
public:
    explicit CullingPass(RenderContext* renderContext) : renderContext(renderContext) {}

    void Initialize(ID3D12Device10* device) override;
    void Execute(ID3D12GraphicsCommandList5* commandList) override;

    D3D12_GPU_VIRTUAL_ADDRESS frameConstants = {};
    D3D12_GPU_VIRTUAL_ADDRESS materialStructured = {};
    D3D12_GPU_VIRTUAL_ADDRESS meshCullingStructured = {};
    D3D12_GPU_VIRTUAL_ADDRESS objectStructured = {};

    Vertix::DescriptorView<Vertix::RenderResourceUsage::UnorderedAccess> indirectCommandsUAV;
    Vertix::DescriptorView<Vertix::RenderResourceUsage::UnorderedAccess> visibleCountUAV;

    Vertix::RenderResource* indirectCommandsBuffer = nullptr;
    Vertix::RenderResource* visibleCountBuffer = nullptr;

private:
    RenderContext* renderContext;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> uavStagingHeap;

    D3D12_CPU_DESCRIPTOR_HANDLE uavStagingHandle = {};
};

#endif //MESHIX_CULLINGPASS_H
