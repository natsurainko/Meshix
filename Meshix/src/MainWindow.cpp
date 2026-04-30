//
// Created by Natsurainko on 2026/4/30.
//

#include "MainWindow.h"

#include <filesystem>
#include <Vertix.Engine/Content/ModelLoader.h>
#include <Vertix.Engine/Content/TextureLoader.h>
#include <Vertix.Engine/Primitive/DefaultPBRMaterial.h>

void MainWindow::OnInitialize() {
    renderPipeline = new RenderPipeline(graphicsDevice, frameCommandList, this);
    renderContext = renderPipeline->GetRenderContext();
    imGuiIO = &ImGui::GetIO();

    defaultPositionController.AttachObject(renderContext->GetPerspectiveCamera());
    defaultRotationController.AttachObject(renderContext->GetPerspectiveCamera());

    defaultPositionController.Speed *= 3.0;
    defaultRotationController.Sensitivity *= 1.5;

    graphicsDevice->CreateCommandQueue(copyCommandQueue, {
        .Type = D3D12_COMMAND_LIST_TYPE_COPY,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
    });
    graphicsDevice->CreateCommandQueue(computeCommandQueue, {
        .Type = D3D12_COMMAND_LIST_TYPE_COMPUTE,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
    });

    // https://github.com/qian-o/GLTF-Assets/tree/main/Bistro
    const auto directory = std::filesystem::path("F:\\GLTF-Assets-main\\Bistro");
    const auto fileName = "BistroExterior.gltf";

    const std::function modelMaterialLoadCallback = [
        materialPool    = &renderContext->materialPool,
        texturePool     = &renderContext->texturePool,
        graphicsDevice  = graphicsDevice,
        copyQueue       = copyCommandQueue,
        computeQueue    = computeCommandQueue,
        dispatcherQueue = &dispatcherQueue,
        directory
    ] (Vertix::Engine::ModelMaterialLoadCallbackContext* context) -> void {
        Vertix::Engine::TextureAsyncLoader textureAsyncLoader {texturePool, graphicsDevice, copyQueue, computeQueue};
        for (const auto &[aiMaterial, name] : context->Materials) {
            const auto materialHandle = materialPool->Allocate(std::make_unique<Vertix::Engine::DefaultPBRMaterial>());
            const auto material = materialPool->GetAs<Vertix::Engine::DefaultPBRMaterial>(materialHandle);

            material->ReadPropertiesFromAssimp(aiMaterial);
            material->ReadTexturesFromAssimp(aiMaterial, [materialPool, materialHandle, &textureAsyncLoader, directory] (const aiString &path, const aiTextureType textureType, Vertix::TextureHandle* destPtr) -> void {
                textureAsyncLoader.LoadTextureAsync(directory / std::string(path.C_Str()),
                    nullptr, Vertix::Engine::DefaultPBRMaterial::GetWicLoaderFlags(textureType) | DirectX::WIC_LOADER_MIP_RESERVE,
                    [destPtr, materialPool, materialHandle] (const Vertix::TextureHandle textureHandle) {
                        *destPtr = textureHandle;
                        materialPool->MarkDirty(materialHandle);
                    }
                );
            });

            context->MaterialHandles.emplace_back(materialHandle);
        }
        textureAsyncLoader.ExecuteAsync(dispatcherQueue);
    };

    Vertix::Engine::ModelAsyncLoader modelAsyncLoader {&renderContext->modelPool, graphicsDevice, copyCommandQueue, computeCommandQueue/*, modelMaterialLoadCallback*/};
    Vertix::Engine::ModelLoadOptions options{};
    options.AssimpPostProcessSteps |=
        aiProcess_OptimizeGraph |
        aiProcess_RemoveRedundantMaterials;

    modelAsyncLoader.LoadModelAsync((directory / fileName).string(), options, [
        modelPool = &renderContext->modelPool,
        sceneObjects = &renderContext->sceneObjects
    ] (const Vertix::ModelHandle handle) -> void {
        auto* model = modelPool->Get(handle);
        auto* sceneObject = sceneObjects->emplace_back(std::make_unique<Vertix::Engine::SceneObject3D>()).get();

        sceneObject->SceneModel = model;
        sceneObject->SetScale(model->Transformation.Scale);
        sceneObject->SetPosition(model->Transformation.Position);
        sceneObject->SetOrientation(model->Transformation.Orientation);
    });

    modelAsyncLoader.ExecuteAsync(&dispatcherQueue);
}

void MainWindow::OnRender(const double deltaTime) {
    if (GetWindowState() == Vertix::Minimized) return;

    dispatcherQueue.FlushQueue();
    renderContext->materialPool.FlushDirty();

    renderPipeline->Execute();
}

void MainWindow::OnUpdate(const double deltaTime) {
    if (!GetFocusingState()) return;

    if (!imGuiIO || !imGuiIO->WantCaptureMouse) mouseControllerInput.Update(deltaTime);
    if (!imGuiIO || !imGuiIO->WantCaptureKeyboard) keyboardControllerInput.Update(deltaTime);

    renderContext->UpdateFrameConstants();
    renderContext->UpdateLightConstants();
    renderContext->UpdateCascadeShadowConstants();
    renderContext->UpdateObjectConstants();
}

void MainWindow::OnResized(const Vertix::Vector2D<unsigned> &size) {
    renderPipeline->Resize(size);
}

void MainWindow::OnFocusLost() {
    if (mouseControllerInput.EnableRotating) {
        ShowCursor(true);
        mouseControllerInput.EnableRotating = false;
    }
}
