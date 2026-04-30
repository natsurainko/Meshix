//
// Created by Natsurainko on 2026/1/27.
//

#ifndef MESHIX_RENDER_PIPELINE_H
#define MESHIX_RENDER_PIPELINE_H

#include <Vertix/Rendering/RenderPipeline.hpp>
#include <Vertix/Windowing/GameWindow.h>

#include "RenderContext.h"

class RenderPipeline : public Vertix::RenderPipeline<RenderContext> {
public:
    RenderPipeline(
        Vertix::GraphicsDevice* graphicsDevice,
        Vertix::FrameCommandList* commandList,
        Vertix::GameWindow* gameWindow);

    void Execute() override;
    void Resize(const Vertix::Vector2D<unsigned> &size) override;

private:
    Vertix::SwapChain* swapChain;
    Vertix::GameWindow* window;
    Vertix::FrameCommandList* frameCommandList;
};

#endif //MESHIX_RENDER_PIPELINE_H
