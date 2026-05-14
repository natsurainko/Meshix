//
// Created by Natsurainko on 2026/5/14.
//

#ifndef MESHIX_GUICONTEXT_H
#define MESHIX_GUICONTEXT_H

#include <Vertix/Graphics/SwapChain.h>
#include <imgui/backends/imgui_impl_dx12.h>
#include <imgui/imgui_internal.h>

class GuiContext {
public:
    GuiContext(Vertix::SwapChain* swapChain) : swapChain(swapChain) {}

    bool windowAboutVisible = false;
    bool windowPerformanceVisible = true;
    bool windowCameraVisible = false;
    bool windowSceneVisible = false;
    bool windowShaderVisible = false;

    bool enableVSync = true;

    bool enableShadowEffect = true;
    bool enableOcclusionEffect = true;

private:
    Vertix::SwapChain* swapChain = nullptr;

    friend class ImGuiPass;
    void OnVSyncChanged() const {
        if (enableVSync != swapChain->GetEnableVsync()) {
            swapChain->SetEnableVSync(enableVSync);
        }
    }

    static void RegisterSettingsHandler(GuiContext* ctx) {
        ImGuiSettingsHandler handler;
        handler.TypeName = "WindowVisibility";
        handler.TypeHash = ImHashStr("WindowVisibility");

        handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler* h, void*, const char* line) {
            auto* ctx = static_cast<GuiContext *>(h->UserData);
            int v;
            if (sscanf_s(line, "Performance=%d", &v) == 1) ctx->windowPerformanceVisible = v;
            if (sscanf_s(line, "Camera=%d",      &v) == 1) ctx->windowCameraVisible      = v;
            if (sscanf_s(line, "Scene=%d",       &v) == 1) ctx->windowSceneVisible       = v;
            if (sscanf_s(line, "Shader=%d",      &v) == 1) ctx->windowShaderVisible      = v;
        };

        handler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* h, ImGuiTextBuffer* buf) {
            const auto* ctx = static_cast<GuiContext *>(h->UserData);
            buf->appendf("[WindowVisibility][Data]\n");
            buf->appendf("Performance=%d\n", ctx->windowPerformanceVisible);
            buf->appendf("Camera=%d\n",      ctx->windowCameraVisible);
            buf->appendf("Scene=%d\n",       ctx->windowSceneVisible);
            buf->appendf("Shader=%d\n",      ctx->windowShaderVisible);
        };

        handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char*) -> void* { return reinterpret_cast<void *>(true); };
        handler.UserData = ctx;

        ImGui::AddSettingsHandler(&handler);
    }
};

#endif //MESHIX_GUICONTEXT_H
