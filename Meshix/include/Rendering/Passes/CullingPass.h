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

    D3D12_GPU_VIRTUAL_ADDRESS frameConstants[2] = {};
    D3D12_GPU_VIRTUAL_ADDRESS cullingViewConstants[2] = {};

    D3D12_GPU_VIRTUAL_ADDRESS materialStructured = {};
    D3D12_GPU_VIRTUAL_ADDRESS objectStructured = {};
    D3D12_GPU_VIRTUAL_ADDRESS meshCullingStructured = {};

    Vertix::DescriptorView<Vertix::RenderResourceUsage::UnorderedAccess> indirectCommandsUAVs[CULLING_VIEW_NUM];
    Vertix::DescriptorView<Vertix::RenderResourceUsage::UnorderedAccess> indirectCountUAV;

    Vertix::RenderResource* indirectCountBuffer = nullptr;

private:
    RenderContext* renderContext;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> uavStagingHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE uavStagingHandle = {};
};

#endif //MESHIX_CULLINGPASS_H
