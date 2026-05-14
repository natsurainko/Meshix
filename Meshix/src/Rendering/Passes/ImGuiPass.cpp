//
// Created by Natsurainko on 2026/3/7.
//

#include "Rendering/Passes/ImGuiPass.h"

#include <imgui/backends/imgui_impl_dx12.h>
#include <imgui/backends/imgui_impl_win32.h>

Vertix::DescriptorHeap* imguiSrvDescriptorHeap = nullptr;

void ImGuiPass::Execute(ID3D12GraphicsCommandList5* commandList) {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    {
        if (guiContext.windowAboutVisible) ImGui::OpenPopup("About");

        const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Meshix");
            ImGui::Text("A lightweight modern real-time renderer developed based on the Vertix framework");
            ImGui::Separator();

            constexpr float buttonWidth = 120.0f;
            ImGui::TextLinkOpenURL("GitHub Repository", "https://github.com/natsurainko/Meshix");
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - buttonWidth);
            if (ImGui::Button("OK", ImVec2(buttonWidth, 0))) {
                guiContext.windowAboutVisible = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Scene")) {
                if (ImGui::MenuItem("New",  "Ctrl+N")) { }
                if (ImGui::MenuItem("Open", "Ctrl+O")) { }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) { exit(0); }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Render")) {
                if (ImGui::MenuItem("VSync", nullptr, &guiContext.enableVSync)) guiContext.OnVSyncChanged();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Window")) {
                ImGui::MenuItem("Performance Window", nullptr, &guiContext.windowPerformanceVisible);
                ImGui::MenuItem("Camera Window", nullptr, &guiContext.windowCameraVisible);
                ImGui::MenuItem("Scene Window", nullptr, &guiContext.windowSceneVisible);
                ImGui::MenuItem("Shader Window", nullptr, &guiContext.windowShaderVisible);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                ImGui::MenuItem("About", nullptr, &guiContext.windowAboutVisible);
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (guiContext.windowCameraVisible) {
            const auto camera = renderContext->GetPerspectiveCamera();
            ImGui::Begin("Camera", &guiContext.windowCameraVisible);
            ImGui::InputFloat3("Position", const_cast<float*>(reinterpret_cast<const float*>(&camera->GetPosition())), "%.3f", ImGuiInputTextFlags_ReadOnly);
            ImGui::InputFloat4("Orientation", const_cast<float*>(reinterpret_cast<const float*>(&camera->GetOrientation())), "%.3f", ImGuiInputTextFlags_ReadOnly);
            ImGui::End();
        }

        if (guiContext.windowPerformanceVisible) {
            ImGui::Begin("Performance", &guiContext.windowPerformanceVisible);
            ImGui::Text("%.0f FPS (%.2f ms/frame)", io->Framerate, 1000.0f / io->Framerate);
            ImGui::End();
        }

        if (guiContext.windowShaderVisible) {
            ImGui::Begin("Shader", &guiContext.windowShaderVisible);

            if (ImGui::CollapsingHeader("Lighting")) {
                ImGui::SeparatorText("Directional Light");
                ImGui::SliderFloat3("Direction", reinterpret_cast<float*>(&renderContext->LightConstants.LightDirection), 1.0f, -1.0f, "%.3f");
                ImGui::ColorEdit3("Light Color", reinterpret_cast<float*>(&renderContext->LightConstants.LightColor));
                ImGui::SliderFloat("Light Intensity", &renderContext->LightConstants.LightIntensity, 0.0f, 25.0f, "%.3f");
                ImGui::SliderFloat("Ambient Intensity", &renderContext->LightConstants.AmbientIntensity, 0.0f, 1.0f, "%.3f");
            }

            ImGui::Checkbox("##shadow_enabled", &guiContext.enableShadowEffect);
            ImGui::SameLine();
            if (!ImGui::CollapsingHeader("Shadow") && guiContext.enableShadowEffect) {
                ImGui::Text("Percentage-Closer Soft Shadow");
            }

            ImGui::Checkbox("##occlusion_enabled", &guiContext.enableOcclusionEffect);
            ImGui::SameLine();
            if (!ImGui::CollapsingHeader("Occlusion") && guiContext.enableOcclusionEffect) {
                ImGui::Text("Horizon-Based Ambient Occlusion");
            }

            ImGui::End();
        }
    }
    ImGui::Render();

    (*currentFrameRTV)->SetRenderTarget(commandList);
    commandList->SetDescriptorHeaps(1, imguiSrvDescriptorHeap->GetDescriptorHeap().GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}
