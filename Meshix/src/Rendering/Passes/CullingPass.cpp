//
// Created by Natsurainko on 2026/5/23.
//

#include "Rendering/Passes/CullingPass.h"

#include <CullingPass_CS.h>

#include "d3dx12_barriers.h"

void CullingPass::Initialize(ID3D12Device10* device) {
    {
        CD3DX12_DESCRIPTOR_RANGE ranges[1];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

        CD3DX12_ROOT_PARAMETER rootParameters[6];
        rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);

        rootParameters[2].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[3].InitAsShaderResourceView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[4].InitAsShaderResourceView(2, 0, D3D12_SHADER_VISIBILITY_ALL);

        rootParameters[5].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.NumParameters = 6;
        rootSignatureDesc.pParameters = rootParameters;
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers = nullptr;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

        Microsoft::WRL::ComPtr<ID3DBlob> signature;
        Microsoft::WRL::ComPtr<ID3DBlob> error;

        ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        ThrowIfFailed(device->CreateRootSignature(0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature)));
    }

    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.CS = SHADER_BYTECODE(SHADER_BYTECODE_CULLING_PASS_CS);

        ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC stagingHeapDesc = {};
        stagingHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        stagingHeapDesc.NumDescriptors = 1;
        stagingHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&stagingHeapDesc, IID_PPV_ARGS(&uavStagingHeap)));

        const auto desc = CD3DX12_UNORDERED_ACCESS_VIEW_DESC::TypedBuffer(DXGI_FORMAT_R32_UINT, CULLING_VIEW_NUM);
        uavStagingHandle = uavStagingHeap->GetCPUDescriptorHandleForHeapStart();
        device->CreateUnorderedAccessView(indirectCountBuffer->GetResource(), nullptr, &desc, uavStagingHandle);
    }

    renderContext->PrepareMeshIndirectBufferHandles(indirectCommandsUAVs);
}

void CullingPass::Execute(ID3D12GraphicsCommandList5* commandList) {
    constexpr UINT clearValues[4] = { 0, 0, 0, 0 };
    commandList->ClearUnorderedAccessViewUint(
        indirectCountUAV.gpuHandle,
        uavStagingHandle,
        indirectCountBuffer->GetResource(),
        clearValues,
        0, nullptr
    );

    const uint32_t meshCount = renderContext->GetMeshCount();
    if (!meshCount) return;

    commandList->SetPipelineState(pipelineState.Get());
    commandList->SetComputeRootSignature(rootSignature.Get());

    commandList->SetComputeRootConstantBufferView(0, frameConstants[renderContext->GetCurrentFrameIndex()]);
    commandList->SetComputeRootConstantBufferView(1, cullingViewConstants[renderContext->GetCurrentFrameIndex()]);

    commandList->SetComputeRootShaderResourceView(2, materialStructured);
    commandList->SetComputeRootShaderResourceView(3, objectStructured);
    commandList->SetComputeRootShaderResourceView(4, meshCullingStructured);

    indirectCountUAV.SetComputeRootDescriptorTable(commandList, 5);

    commandList->Dispatch((meshCount + 63) / 64, 1, 1);
}
