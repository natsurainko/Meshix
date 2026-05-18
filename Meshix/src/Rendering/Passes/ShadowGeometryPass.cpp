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
        CD3DX12_ROOT_PARAMETER rootParameters[2];
        rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.NumParameters = 2;
        rootSignatureDesc.pParameters = rootParameters;
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers = nullptr;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

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
}

void ShadowGeometryPass::Execute(ID3D12GraphicsCommandList5* commandList) {
    shadowDepthDSV->ClearDepth(commandList);
    if (!renderContext->EnablePCSS) return;

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    {
        commandList->SetGraphicsRootSignature(rootSignature.Get());
        commandList->SetGraphicsRootConstantBufferView(1, renderContext->cascadeShadowConstantsBuffer->GetGPUVirtualAddress());

        shadowDepthDSV->SetRenderTarget(commandList);
        commandList->SetPipelineState(pipelineState.Get());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for (UINT i = 0; i < renderContext->sceneObjects.size(); ++i) {
            const auto &sceneObject = renderContext->sceneObjects[i];
            commandList->SetGraphicsRootConstantBufferView(0, renderContext->objectConstantsBuffer.GetGpuVirtualAddressAt(i));

            for (const auto &mesh : sceneObject->SceneModel->Meshes) {
                if (mesh.Material.slot) {
                    if (const auto material = renderContext->materialPool->GetAs<Vertix::Engine::DefaultPBRMaterial>(mesh.Material); material->alphaMode == 2) {
                        // BLEND materials should skip the shadow rendering phase.
                        continue;
                    }
                }

                commandList->IASetVertexBuffers(0, 1, &mesh.VertexBuffer->d3d12VertexBufferView);
                commandList->IASetIndexBuffer(&mesh.IndexBuffer->d3d12IndexBufferView);
                commandList->DrawIndexedInstanced(mesh.IndexBuffer->indexCount, CASCADE_NUM, 0, 0, 0);
            }
        }
    }
    commandList->RSSetViewports(1, renderContext->viewport);
    commandList->RSSetScissorRects(1, renderContext->scissorRect);
}
