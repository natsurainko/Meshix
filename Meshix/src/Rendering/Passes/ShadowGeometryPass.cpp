//
// Created by Natsurainko on 2026/1/29.
//

#include "Rendering/Passes/ShadowGeometryPass.h"

#include <ShadowGeometryPass_PS.h>
#include <ShadowGeometryPass_VS.h>

ShadowGeometryPass::~ShadowGeometryPass() {
    delete depthStencilView;
}

void ShadowGeometryPass::Initialize(
    Vertix::GraphicsDevice* device,
    RenderContext* context)
{
    RenderPass::Initialize(device, context);
    const auto &d3d12Device = device->GetD3D12Device();

    viewport = CD3DX12_VIEWPORT {0.0f, 0.0f, static_cast<float>(renderContext->ShadowMapSize), static_cast<float>(renderContext->ShadowMapSize)};
    scissorRect = CD3DX12_RECT {0,0, static_cast<int>(renderContext->ShadowMapSize), static_cast<int>(renderContext->ShadowMapSize)};

    {
        renderContext->dsvDescriptorHeap.AllocDescriptorHandle(dsvHandle);
        renderContext->srvDescriptorHeap.AllocDescriptorHandle(srvHandle, renderContext->shadowGeometrySrvGpuHandle);
    }

    {
        auto dsvResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS,renderContext->ShadowMapSize, renderContext->ShadowMapSize);
        const auto srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2DArray(DXGI_FORMAT_R32_FLOAT, CASCADE_NUM, 1);
        auto dsvDesc = D3D12_DEPTH_STENCIL_VIEW_DESC();

        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.MipSlice = 0;
        dsvDesc.Texture2DArray.FirstArraySlice = 0;
        dsvDesc.Texture2DArray.ArraySize = CASCADE_NUM;
        dsvResourceDesc.DepthOrArraySize = CASCADE_NUM;
        dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        depthStencilView = new Vertix::DepthStencilView(graphicsDevice, dsvResourceDesc, dsvHandle, &dsvDesc);
        depthStencilView->CreateShaderResourceView(&srvDesc, srvHandle);

        renderContext->shadowGeometryDsvBarrier = depthStencilView->CreateTransitionBarrier(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        srvBarrier = depthStencilView->CreateTransitionBarrier(D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

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
        ThrowIfFailed(d3d12Device->CreateRootSignature(0,
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
        ThrowIfFailed(d3d12Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
    }
}

void ShadowGeometryPass::Execute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> &commandList) {
    if (!renderContext->EnablePCSS) {
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH , 1.0f, 0, 0, nullptr);
        commandList->ResourceBarrier(1, &srvBarrier);
        return;
    }

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetGraphicsRootConstantBufferView(1, renderContext->cascadeShadowConstantsBuffer.GetGpuVirtualAddress());
    commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH , 1.0f, 0, 0, nullptr);
    commandList->SetPipelineState(pipelineState.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (UINT i = 0; i < renderContext->sceneObjects.size(); ++i) {
        const auto &sceneObject = renderContext->sceneObjects[i];
        commandList->SetGraphicsRootConstantBufferView(0, renderContext->objectConstantsBuffer.GetGpuVirtualAddressAt(i));

        for (const auto &mesh : sceneObject->SceneModel->Meshes) {
            if (mesh.Material.slot) {
                if (const auto material = renderContext->materialPool.GetAs<Vertix::Engine::DefaultPBRMaterial>(mesh.Material); material->alphaMode == 2) {
                    // BLEND materials should skip the shadow rendering phase.
                    continue;
                }
            }

            commandList->IASetVertexBuffers(0, 1, &mesh.VertexBuffer->d3d12VertexBufferView);
            commandList->IASetIndexBuffer(&mesh.IndexBuffer->d3d12IndexBufferView);
            commandList->DrawIndexedInstanced(mesh.IndexBuffer->indexCount, CASCADE_NUM, 0, 0, 0);
        }
    }

    commandList->ResourceBarrier(1, &srvBarrier);
    commandList->RSSetViewports(1, &renderContext->viewport);
    commandList->RSSetScissorRects(1, &renderContext->scissorRect);
}
