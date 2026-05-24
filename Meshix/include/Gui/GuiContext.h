//
// Created by Natsurainko on 2026/5/14.
//

#ifndef MESHIX_GUICONTEXT_H
#define MESHIX_GUICONTEXT_H

#include <filesystem>
#include <Vertix/Graphics/SwapChain.h>
#include <imgui/backends/imgui_impl_dx12.h>
#include <imgui/imgui_internal.h>

#include "Rendering/RenderContext.h"
#include "Vertix.Engine/Content/ModelLoader.h"
#include "Vertix.Engine/Content/TextureLoader.h"

class GuiContext {
public:
    GuiContext(
        Vertix::GameWindow* window,
        RenderContext* renderContext)
    : window(window), swapChain(window->GetSwapChain()), renderContext(renderContext) {}

    bool windowAboutVisible = false;
    bool windowPerformanceVisible = true;
    bool windowCameraVisible = false;
    bool windowSceneVisible = false;
    bool windowShaderVisible = false;

    bool enableVSync = true;

    bool enableShadowEffect = true;
    bool enableOcclusionEffect = true;

private:
    Vertix::GameWindow* window = nullptr;
    Vertix::SwapChain* swapChain = nullptr;
    RenderContext* renderContext = nullptr;

    friend class ImGuiPass;
    void OnVSyncChanged() const {
        if (enableVSync != swapChain->GetEnableVsync()) {
            swapChain->SetEnableVSync(enableVSync);
        }
    }

    void OnModelImportClicked() const {
        wchar_t filePathChars[MAX_PATH] = {};

        OPENFILENAMEW openFileName = {};
        openFileName.lStructSize  = sizeof(openFileName);
        openFileName.hwndOwner    = window->GetWindowHandle();
        openFileName.lpstrFile    = filePathChars;
        openFileName.nMaxFile     = MAX_PATH;
        openFileName.lpstrFilter  =
            L"glTF Model\0*.gltf\0"
            L"glTF Binary Model\0*.glb\0";
        openFileName.nFilterIndex = 1;
        openFileName.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
        if (!GetOpenFileNameW(&openFileName)) return;

        const auto filePath = std::filesystem::path(filePathChars);
        const auto directory = filePath.parent_path();

        const auto graphicsDevice = window->GetGraphicsDevice();
        const auto dispatcherQueue = window->GetDispatcherQueue();

        const std::function modelMaterialLoadCallback = [
            materialPool    = renderContext->materialPool.get(),
            texturePool     = renderContext->texturePool.get(),
            graphicsDevice  = graphicsDevice,
            copyQueue       = renderContext->copyCommandQueue,
            computeQueue    = renderContext->computeCommandQueue,
            dispatcherQueue = dispatcherQueue,
            directory
        ] (Vertix::Engine::ModelMaterialLoadCallbackContext* context) -> void {
            Vertix::Engine::TextureAsyncLoader textureAsyncLoader = {
                texturePool,
                graphicsDevice,
                copyQueue,
                computeQueue
            };

            for (const auto &[aiMaterial, name] : context->Materials) {
                const auto materialHandle = materialPool->Allocate(std::make_unique<Vertix::Engine::DefaultPBRMaterial>());
                const auto material = materialPool->GetAs<Vertix::Engine::DefaultPBRMaterial>(materialHandle);

                material->ReadPropertiesFromAssimp(aiMaterial);
                material->ReadTexturesFromAssimp(aiMaterial, [
                    materialPool,
                    materialHandle,
                    &textureAsyncLoader,
                    directory
                ] (const aiString &path, const aiTextureType textureType, Vertix::TextureHandle* destPtr) -> void {
                    textureAsyncLoader.LoadTextureAsync(directory / std::string(path.C_Str()),
                        nullptr, Vertix::Engine::DefaultPBRMaterial::GetWicLoaderFlags(textureType) | DirectX::WIC_LOADER_MIP_RESERVE,
                        [
                            destPtr,
                            materialPool,
                            materialHandle
                        ] (const Vertix::TextureHandle &textureHandle) {
                            *destPtr = textureHandle;
                            materialPool->MarkDirty(materialHandle);
                        }
                    );
                });

                context->MaterialHandles.emplace_back(materialHandle);
            }
            textureAsyncLoader.ExecuteAsync(dispatcherQueue);
        };

        Vertix::Engine::ModelLoadOptions options = {};
        options.AssimpPostProcessSteps |=
            // aiProcess_OptimizeGraph |
            aiProcess_RemoveRedundantMaterials;

        Vertix::Engine::ModelAsyncLoader modelAsyncLoader = {
            renderContext->modelPool.get(),
            graphicsDevice,
            renderContext->copyCommandQueue,
            renderContext->computeCommandQueue,
            modelMaterialLoadCallback
        };

        modelAsyncLoader.LoadModelAsync(filePath.string(), options, [
            renderContext = renderContext
        ] (const Vertix::ModelHandle handle) -> void {
            const auto model = renderContext->modelPool->Get(handle);
            auto sceneObject = std::make_unique<Vertix::Engine::SceneObject3D>();
            sceneObject->SceneModel = model;
            sceneObject->SetScale(model->Transformation.Scale);
            sceneObject->SetPosition(model->Transformation.Position);
            sceneObject->SetOrientation(model->Transformation.Orientation);
            renderContext->AddSceneObject(std::move(sceneObject));
        });

        modelAsyncLoader.ExecuteAsync(dispatcherQueue);
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
