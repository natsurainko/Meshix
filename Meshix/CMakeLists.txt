add_executable(Meshix WIN32)
set_target_properties(Meshix PROPERTIES LINKER_LANGUAGE CXX)
target_sources(Meshix
    PRIVATE
        src/Rendering/Passes/GeometryPass.cpp
        src/Rendering/Passes/ShadowGeometryPass.cpp
        src/Rendering/Passes/ShadowPass.cpp
        src/Rendering/Passes/ImGuiPass.cpp
        src/Rendering/Passes/LightingPass.cpp
        src/Rendering/Passes/AmbientOcclusionPass.cpp
        src/Rendering/RenderPipeline.cpp
        src/MainWindow.cpp
        src/Main.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/MainWindow.h

            include/Controlling/KeyboardControllerInput.h
            include/Controlling/MouseControllerInput.h
            include/Rendering/RenderPipeline.h
            include/Rendering/RenderContext.h
            include/Rendering/Passes/GeometryPass.h
            include/Rendering/Passes/ShadowGeometryPass.h
            include/Rendering/Passes/ShadowPass.h
            include/Rendering/Passes/ImGuiPass.h
            include/Rendering/Passes/LightingPass.h
            include/Rendering/Passes/AmbientOcclusionPass.h
)

set(HLSL_SHADER_FILES
        shaders/csm.hlsli
        shaders/hbao.hlsli
        shaders/noise.hlsli
        shaders/pbr.hlsli
        shaders/pcss.hlsli
        shaders/poisson.hlsli

        shaders/passes/AmbientOcclusionPass.hlsl
        shaders/passes/GeometryPass.hlsl
        shaders/passes/LightingPass.hlsl
        shaders/passes/ShadowGeometryPass.hlsl
        shaders/passes/ShadowPass.hlsl
)

set_source_files_properties(shaders/passes/AmbientOcclusionPass.hlsl PROPERTIES
        ShaderModel  "6_0"
        VSMain       "VSMain"
        PSMain       "PSMain"
        ByteCodeName "AMBIENT_OCCLUSION_PASS"
)
set_source_files_properties(shaders/passes/GeometryPass.hlsl PROPERTIES
        ShaderModel  "6_6"
        VSMain       "VSMain"
        PSMain       "PSMain"
        ByteCodeName "GEOMETRY_PASS"
)
set_source_files_properties(shaders/passes/LightingPass.hlsl PROPERTIES
        ShaderModel  "6_0"
        VSMain       "VSMain"
        PSMain       "PSMain"
        ByteCodeName "LIGHTING_PASS"
)
set_source_files_properties(shaders/passes/ShadowGeometryPass.hlsl PROPERTIES
        ShaderModel  "6_0"
        VSMain       "VSMain"
        PSMain       "PSMain"
        ByteCodeName "SHADOW_GEOMETRY_PASS"
)
set_source_files_properties(shaders/passes/ShadowPass.hlsl PROPERTIES
        ShaderModel  "6_0"
        VSMain       "VSMain"
        PSMain       "PSMain"
        ByteCodeName "SHADOW_PASS"
)

add_hlsl_shaders(Meshix FILES ${HLSL_SHADER_FILES})
target_include_directories(Meshix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

add_dependencies(Meshix Vertix Vertix.Engine)
target_link_libraries(Meshix PRIVATE Vertix Vertix.Engine imgui)

add_custom_command(TARGET Meshix POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:Meshix>
        $<TARGET_FILE_DIR:Meshix>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime DLLs..."
)
