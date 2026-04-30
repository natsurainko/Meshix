//
// Created by Natsurainko on 2026/4/30.
//

#include <crtdbg.h>
#include <Vertix/Hosting/GameApplicationBuilder.hpp>

#include "MainWindow.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, const int nShowCmd) {
#ifndef NDEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    constexpr Vertix::GraphicsDeviceOptions graphicsDeviceOptions {
        .enableDebugLayer = false
    };

    const Vertix::WindowOptions windowOptions {
        .renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        .windowSize = { 1280, 768 },
        .windowTitle = L"Meshix",
    };

    return Vertix::GameApplicationBuilder(hInstance, lpCmdLine, nShowCmd)
        .ConfigureGraphicsDevice(graphicsDeviceOptions)
        .ConfigureWindow<MainWindow>(windowOptions)
        .Build()
        .Run();
}
