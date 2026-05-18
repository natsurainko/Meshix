//
// Created by Natsurainko on 2026/3/7.
//

#include "Rendering/Passes/LightingPass.h"

#include <LightingPass_PS.h>
#include <LightingPass_VS.h>

void LightingPass::Initialize(ID3D12Device10* device) {
    {
        CD3DX12_STATIC_SAMPLER_DESC staticSampler(0);

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[2].InitAsConstants(5, 2, 0, D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.NumParameters = 3;
        rootSignatureDesc.pParameters = rootParameters;
        rootSignatureDesc.NumStaticSamplers = 1;
        rootSignatureDesc.pStaticSamplers = &staticSampler;
        rootSignatureDesc.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

        Microsoft::WRL::ComPtr<ID3DBlob> signature;
        Microsoft::WRL::ComPtr<ID3DBlob> error;

        ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        ThrowIfFailed(device->CreateRootSignature(0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature)));
    }

    {
        constexpr D3D12_INPUT_ELEMENT_DESC inputElementDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertix::Vertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertix::Vertex, TexCoord), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDesc, _countof(inputElementDesc) };
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = SHADER_BYTECODE(SHADER_BYTECODE_LIGHTING_PASS_VS);
        psoDesc.PS = SHADER_BYTECODE(SHADER_BYTECODE_LIGHTING_PASS_PS);
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC2(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.NumRenderTargets = 1;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = UINT_MAX;
        ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
    }

    handles.gNormalHandle = gNormalSRV->slot;
    handles.gAlbedoHandle = gAlbedoSRV->slot;
    handles.gORMHandle = gORMSRV->slot;
    handles.gDepthHandle = gDepthSRV->slot;
    handles.ShadowMaskHandle = shadowMaskSRV->slot;
}

void LightingPass::Execute(ID3D12GraphicsCommandList5* commandList) {
    constexpr float clearColor[] = { 0.3921f, 0.5843f, 0.9294f, 1.0f };

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetGraphicsRootConstantBufferView(0, frameConstantsAddress);
    commandList->SetGraphicsRootConstantBufferView(1, lightConstantsAddress);
    commandList->SetGraphicsRoot32BitConstants(2, 5, &handles, 0);

    currentFrameRTV->SetRenderTarget(commandList);
    currentFrameRTV->Clear(commandList, clearColor);

    commandList->SetPipelineState(pipelineState.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    commandList->IASetVertexBuffers(0, 1, &renderContext->fullScreenVertex->d3d12VertexBufferView);
    commandList->DrawInstanced(renderContext->fullScreenVertex->vertexCount, 1, 0, 0);
}
