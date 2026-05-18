//
// Created by Natsurainko on 2026/1/27.
//

#include "Rendering/Passes/GeometryPass.h"

#include <GeometryPass_PS.h>
#include <GeometryPass_VS.h>
#include <d3d12/d3dx12_core.h>
#include <Vertix/Exceptions/HResultException.h>
#include <Vertix/Graphics/GraphicsDevice.h>

void GeometryPass::Initialize(ID3D12Device10* device) {
    {
        CD3DX12_STATIC_SAMPLER_DESC staticSampler(0);
        staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        staticSampler.Filter = D3D12_FILTER_ANISOTROPIC;
        staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.MaxAnisotropy = 4;
        staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_ROOT_PARAMETER rootParameters[4];
        rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[2].InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[3].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.NumParameters = 4;
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
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertix::Vertex, Normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertix::Vertex, TexCoord), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertix::Vertex, Tangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertix::Vertex, Bitangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDesc, _countof(inputElementDesc) };
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = SHADER_BYTECODE(SHADER_BYTECODE_GEOMETRY_PASS_VS);
        psoDesc.PS = SHADER_BYTECODE(SHADER_BYTECODE_GEOMETRY_PASS_PS);
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC2(D3D12_DEFAULT);
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = UINT_MAX;

        psoDesc.NumRenderTargets = 3;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        psoDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        psoDesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
    }
}

void GeometryPass::Execute(ID3D12GraphicsCommandList5* commandList) {
    constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetGraphicsRootConstantBufferView(0, frameConstantsAddress);
    commandList->SetGraphicsRootShaderResourceView(3, renderContext->materialPool->GetGpuVirtualAddress());

    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[3] = {
        gNormalRTV->cpuHandle,
        gAlbedoRTV->cpuHandle,
        gORMRTV->cpuHandle,
    };
    commandList->OMSetRenderTargets(3, rtvHandles, FALSE, &gDepthDSV->cpuHandle);

    gNormalRTV->Clear(commandList, clearColor);
    gAlbedoRTV->Clear(commandList, clearColor);
    gORMRTV->Clear(commandList, clearColor);
    gDepthDSV->ClearDepth(commandList, 1.0f);

    commandList->SetPipelineState(pipelineState.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (UINT i = 0; i < renderContext->sceneObjects.size(); ++i) {
        const auto &sceneObject = renderContext->sceneObjects[i];
        commandList->SetGraphicsRootConstantBufferView(1, renderContext->objectConstantsBuffer.GetGpuVirtualAddressAt(i));

        for (const auto &mesh : sceneObject->SceneModel->Meshes) {
            if (mesh.Material.slot) {
                if (const auto material = renderContext->materialPool->GetAs<Vertix::Engine::DefaultPBRMaterial>(mesh.Material); material->alphaMode == 2) {
                    // BLEND materials should skip the deferred rendering phase.
                    // TODO: Add a Forward Rendering Pass to render transparent material meshes.
                    continue;
                }
            }

            commandList->SetGraphicsRoot32BitConstant(2, mesh.Material.slot, 0);
            commandList->IASetVertexBuffers(0, 1, &mesh.VertexBuffer->d3d12VertexBufferView);
            commandList->IASetIndexBuffer(&mesh.IndexBuffer->d3d12IndexBufferView);
            commandList->DrawIndexedInstanced(mesh.IndexBuffer->indexCount, 1, 0, 0, 0);
        }
    }
}
