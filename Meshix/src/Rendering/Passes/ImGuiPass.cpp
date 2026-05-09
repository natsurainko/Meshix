//
// Created by Natsurainko on 2026/3/7.
//

#include "Rendering/Passes/ImGuiPass.h"

#include <imgui/backends/imgui_impl_dx12.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <Vertix/Graphics/SwapChain.h>

Vertix::DescriptorHeap* imguiSrvDescriptorHeap = nullptr;

void ImGuiPass::Execute(ID3D12GraphicsCommandList5* commandList) {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    {
        static bool _enableVSync = true;
        bool enableVSync = _enableVSync;

        ImGui::Begin("Meshix");
        {
            ImGui::Text("Use WASD, Left Shift, and Space to move the camera.");
            ImGui::TextLinkOpenURL("GitHub Repository", "https://github.com/natsurainko/Vertix");

            ImGui::Checkbox("Enable VSync", &enableVSync);
            ImGui::SameLine();
            ImGui::Text("%.0f FPS (%.2f ms/frame)", io->Framerate, 1000.0f / io->Framerate);

            if (ImGui::CollapsingHeader("Scene Information", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("%llu SceneObjects, %u Models, %u Materials, %u Textures",
                    renderContext->sceneObjects.size(),
                    renderContext->modelPool.GetCount(),
                    renderContext->materialPool.GetCount(),
                    renderContext->texturePool.GetCount());

                const auto camera = renderContext->GetPerspectiveCamera();
                ImGui::SeparatorText("Camera");
                ImGui::InputFloat3("Position", const_cast<float*>(reinterpret_cast<const float*>(&camera->GetPosition())), "%.3f", ImGuiInputTextFlags_ReadOnly);
                ImGui::InputFloat4("Orientation", const_cast<float*>(reinterpret_cast<const float*>(&camera->GetOrientation())), "%.3f", ImGuiInputTextFlags_ReadOnly);
                ImGui::SeparatorText("Directional Light");
                ImGui::SliderFloat3("Direction", reinterpret_cast<float*>(&renderContext->LightConstants.LightDirection), 1.0f, -1.0f, "%.3f");
                ImGui::ColorEdit3("Light Color", reinterpret_cast<float*>(&renderContext->LightConstants.LightColor));
                ImGui::SliderFloat("Light Intensity", &renderContext->LightConstants.LightIntensity, 0.0f, 25.0f, "%.3f");
                ImGui::SliderFloat("Ambient Intensity", &renderContext->LightConstants.AmbientIntensity, 0.0f, 1.0f, "%.3f");
            }

            if (ImGui::CollapsingHeader("Shader Effect", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Enable PCSS Shadow (Percentage-Closer Soft Shadows)", &renderContext->EnablePCSS);
                ImGui::Checkbox("Enable HBAO (Horizon-Based Ambient Occlusion)", &renderContext->EnableHBAO);
            }
        }
        ImGui::End();

        if (enableVSync != _enableVSync) {
            swapChain->SetEnableVSync(enableVSync);
            _enableVSync = enableVSync;
        }
    }
    ImGui::Render();

    (*currentFrameRTV)->SetRenderTarget(commandList);
    commandList->SetDescriptorHeaps(1, imguiSrvDescriptorHeap->GetDescriptorHeap().GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}
