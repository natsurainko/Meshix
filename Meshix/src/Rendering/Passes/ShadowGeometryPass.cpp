//
// Created by Natsurainko on 2026/1/29.
//

#include "Rendering/Passes/ShadowGeometryPass.h"

#include <ShadowGeometryPass_PS.h>
#include <ShadowGeometryPass_VS.h>

void ShadowGeometryPass::Initialize(ID3D12Device10* device) {
    viewport = CD3DX12_VIEWPORT {0.0f, 0.0f, static_cast<float>(renderContext->ShadowMapSize), static_cast<float>(renderContext->ShadowMapSize)};
    scissorRect = CD3DX12_RECT {0,0, static_cast<int>(renderContext->ShadowMapSize), static_cast<int>(renderContext->ShadowMapSize)};

    {
        CD3DX12_STATIC_SAMPLER_DESC pointSampler(0);

        CD3DX12_ROOT_PARAMETER rootParameters[6];
        rootParameters[0].InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[1].InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[3].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[4].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[5].InitAsConstants(1, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.NumParameters = 6;
        rootSignatureDesc.pParameters = rootParameters;
        rootSignatureDesc.NumStaticSamplers = 1;
        rootSignatureDesc.pStaticSamplers = &pointSampler;
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
        psoDesc.VS = SHADER_BYTECODE(SHADER_BYTECODE_SHADOW_GEOMETRY_PASS_VS);
        psoDesc.PS = SHADER_BYTECODE(SHADER_BYTECODE_SHADOW_GEOMETRY_PASS_PS);
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.DepthClipEnable = false;
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC2(D3D12_DEFAULT);
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.NumRenderTargets = 0;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = UINT_MAX;
        ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
    }

    {
        D3D12_INDIRECT_ARGUMENT_DESC argDescs[5] = {};
        argDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        argDescs[0].Constant.RootParameterIndex = 0;
        argDescs[0].Constant.DestOffsetIn32BitValues = 0;
        argDescs[0].Constant.Num32BitValuesToSet = 1;

        argDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        argDescs[1].Constant.RootParameterIndex = 1;
        argDescs[1].Constant.DestOffsetIn32BitValues = 0;
        argDescs[1].Constant.Num32BitValuesToSet = 1;

        argDescs[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
        argDescs[2].VertexBuffer.Slot = 0;

        argDescs[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
        argDescs[4].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

        D3D12_COMMAND_SIGNATURE_DESC desc = {};
        desc.pArgumentDescs   = argDescs;
        desc.NumArgumentDescs = 5;
        desc.ByteStride       = sizeof(MeshIndirectCommand);
        desc.NodeMask         = 0;

        ThrowIfFailed(device->CreateCommandSignature(&desc, rootSignature.Get(), IID_PPV_ARGS(&commandSignature)));
    }
}

void ShadowGeometryPass::Execute(ID3D12GraphicsCommandList5* commandList) {
    shadowDepthDSV.ClearDepth(commandList);

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    {
        commandList->SetGraphicsRootSignature(rootSignature.Get());
        commandList->SetGraphicsRootConstantBufferView(2, cascadeShadowConstants[renderContext->GetCurrentFrameIndex()]);
        commandList->SetGraphicsRootShaderResourceView(3, objectStructured);
        commandList->SetGraphicsRootShaderResourceView(4, materialStructured);

        shadowDepthDSV.SetRenderTarget(commandList);
        commandList->SetPipelineState(pipelineState.Get());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        for (uint32_t i = 0; i < CASCADE_NUM; ++i) {
            commandList->SetGraphicsRoot32BitConstant(5, i, 0);
            commandList->ExecuteIndirect(
                commandSignature.Get(),
                65535,
                indirectCommandsBuffers[i]->GetResource(),
                0,
                indirectCountBuffer->GetResource(),
                4 * (i + 1)
            );
        }
    }
    commandList->RSSetViewports(1, renderContext->viewport);
    commandList->RSSetScissorRects(1, renderContext->scissorRect);
}
