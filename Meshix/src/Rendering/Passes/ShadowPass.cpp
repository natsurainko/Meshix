//
// Created by Natsurainko on 2026/1/29.
//

#include "Rendering/Passes/ShadowPass.h"

#include <ShadowPass_PS.h>
#include <ShadowPass_VS.h>

ShadowPass::~ShadowPass() {
    delete renderTargetView;
}

void ShadowPass::Initialize(
    Vertix::GraphicsDevice* device,
    RenderContext *context)
{
    RenderPass::Initialize(device, context);
    const auto &d3d12Device = device->GetD3D12Device();

    ShadowMapTexel[0] = 1.0f / static_cast<float>(renderContext->ShadowMapSize);
    ShadowMapTexel[1] = 1.0f / static_cast<float>(renderContext->ShadowMapSize);

    {
        renderContext->rtvDescriptorHeap.AllocDescriptorHandle(rtvHandle);
        renderContext->srvDescriptorHeap.AllocDescriptorHandle(srvHandle, renderContext->shadowSrvGpuHandle);
    }

    {
        auto rtvResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16_FLOAT,context->windowSize.X, context->windowSize.Y);
        constexpr auto clearValue = D3D12_CLEAR_VALUE{ .Color = { 1.0f, 1.0f, 1.0f, 1.0f } };
        const auto srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(DXGI_FORMAT_R16_FLOAT, 1);
        rtvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        renderTargetView = new Vertix::RenderTargetView(graphicsDevice, rtvResourceDesc, rtvHandle, nullptr, clearValue);
        renderTargetView->CreateShaderResourceView(&srvDesc, srvHandle);

        renderContext->shadowRtvBarrier = renderTargetView->CreateTransitionBarrier(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        srvBarrier = renderTargetView->CreateTransitionBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    {
        CD3DX12_DESCRIPTOR_RANGE srvRanges[2];
        srvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);
        srvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);

        CD3DX12_STATIC_SAMPLER_DESC pointSampler(0);
        pointSampler.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        pointSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pointSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pointSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        /*CD3DX12_STATIC_SAMPLER_DESC comparePointSampler(1);
        comparePointSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        comparePointSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
        comparePointSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        comparePointSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        comparePointSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        CD3DX12_STATIC_SAMPLER_DESC samplers[] = { pointSampler, comparePointSampler };*/

        CD3DX12_ROOT_PARAMETER rootParameters[5];
        rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[2].InitAsDescriptorTable(1, &srvRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[3].InitAsDescriptorTable(1, &srvRanges[1], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[4].InitAsConstants(2, 2, 0, D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.NumParameters = 5;
        rootSignatureDesc.pParameters = rootParameters;
        rootSignatureDesc.NumStaticSamplers = 1;
        rootSignatureDesc.pStaticSamplers = &pointSampler; // samplers;
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
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertix::Vertex, TexCoord), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = { inputElementDesc, _countof(inputElementDesc) };
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = SHADER_BYTECODE(SHADER_BYTECODE_SHADOW_PASS_VS);
        psoDesc.PS = SHADER_BYTECODE(SHADER_BYTECODE_SHADOW_PASS_PS);
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC2(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R16_FLOAT;
        psoDesc.NumRenderTargets = 1;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = UINT_MAX;
        ThrowIfFailed(d3d12Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
    }
}

void ShadowPass::Execute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> &commandList) {
    constexpr float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    if (!renderContext->EnablePCSS) {
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        commandList->ResourceBarrier(1, &srvBarrier);
        return;
    }

    commandList->SetDescriptorHeaps(1, renderContext->srvDescriptorHeap.GetDescriptorHeap().GetAddressOf());
    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetGraphicsRootConstantBufferView(0, renderContext->lightConstantsBuffer.GetGpuVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, renderContext->cascadeShadowConstantsBuffer.GetGpuVirtualAddress());
    commandList->SetGraphicsRoot32BitConstants(4, 2, &ShadowMapTexel, 0);
    commandList->SetGraphicsRootDescriptorTable(2, renderContext->geometrySrvGpuHandles[0]);
    commandList->SetGraphicsRootDescriptorTable(3, renderContext->shadowGeometrySrvGpuHandle);

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->SetPipelineState(pipelineState.Get());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    commandList->IASetVertexBuffers(0, 1, &renderContext->fullScreenVertex->d3d12VertexBufferView);
    commandList->DrawInstanced(renderContext->fullScreenVertex->vertexCount, 1, 0, 0);

    commandList->ResourceBarrier(1, &srvBarrier);
}

void ShadowPass::Resize(const Vertix::Vector2D<unsigned> &size) {
    const auto srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(DXGI_FORMAT_R16_FLOAT);

    renderTargetView->Resize(size);
    renderTargetView->CreateShaderResourceView(&srvDesc, srvHandle);

    renderContext->shadowRtvBarrier = renderTargetView->CreateTransitionBarrier(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    srvBarrier = renderTargetView->CreateTransitionBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
