cmake_minimum_required(VERSION 3.12)

project(FarlorEngine LANGUAGES CXX)

set(FarlorEngine_VERSION 1.0.0)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

add_subdirectory(dependencies)

add_library(FarlorEngine

    # Source Files
    Core/Events/ButtonEvent.cpp
    Core/Events/ControllerConnectionEvent.cpp
    Core/Events/ControllerVibrationEvent.cpp
    Core/Events/EventManager.cpp
    Core/Events/KeyboardEvent.cpp
    Core/Events/ThumbstickEvent.cpp

    Core/Memory/BaseAllocator.cpp
    Core/Memory/LinearAllocator.cpp
    Core/Memory/PoolAllocator.cpp
    Core/Memory/ProxyAllocator.cpp
    Core/Memory/StackAllocator.cpp

    Core/Engine.cpp
    Core/FixedUpdate.cpp
    Core/Timer.cpp
    Core/Win32Window.cpp

    ECS/Component.cpp
    ECS/EntityManager.cpp
    ECS/TransformManager.cpp

    dependencies/imgui/imgui_demo.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_tables.cpp
    dependencies/imgui/imgui_widgets.cpp
    dependencies/imgui/imgui.cpp
    dependencies/imgui/backends/imgui_impl_win32.cpp
    dependencies/imgui/backends/imgui_impl_dx11.cpp

    Input/ControllerManager.cpp
    Input/InputProcessor.cpp
    Input/InputStateManager.cpp

    JsonCpp/jsoncpp.cpp

    Networking/NetworkManager.cpp
    Networking/Packet.cpp
    Networking/Socket.cpp

    NewRenderer/D3D11/D3D11_Utils.cpp

    NewRenderer/ComputePipelineState.cpp
    NewRenderer/GraphicsPipelineState.cpp
    NewRenderer/Camera.cpp
    NewRenderer/LargeScaleDesertModel.cpp
    NewRenderer/SmallScaleDesertModel.cpp
    NewRenderer/LargeScaleDesertModel_Rasterization.cpp
    NewRenderer/ScalarField2D.cpp
    NewRenderer/Mesh.cpp
    NewRenderer/Renderer.cpp
    NewRenderer/Shader.cpp
    NewRenderer/Vertex.cpp
    NewRenderer/WICTextureLoader.cpp

    Physics/CollisionFunctions.cpp
    Physics/PhysicsComponent.cpp
    Physics/PhysicsSystem.cpp
    Physics/LineSegment.cpp

    Util/HashedString.cpp
    Util/StringUtil.cpp
    Util/SystemCheck.cpp
    Util/UIDGenerator.cpp

    Core/Events/ButtonEvent.h
    Core/Events/ControllerConnectionEvent.h
    Core/Events/ControllerVibrationEvent.h
    Core/Events/EventData.h
    Core/Events/EventListener.h
    Core/Events/EventManager.h
    Core/Events/KeyboardEvent.h
    Core/Events/ThumbstickEvent.h

    Core/Memory/BaseAllocator.h
    Core/Memory/LinearAllocator.h
    Core/Memory/MemoryAlignmentUtils.h
    Core/Memory/PoolAllocator.h
    Core/Memory/ProxyAllocator.h
    Core/Memory/StackAllocator.h

    Core/Engine.h
    Core/FixedUpdate.h
    Core/EngineConfig.h
    Core/Timer.h
    Core/Win32Window.h
    Core/Window.h

    ECS/Component.h
    ECS/EntityManager.h
    ECS/Entity.h
    ECS/TransformComponent.h
    ECS/TransformManager.h

    dependencies/imgui/imconfig.h
    dependencies/imgui/imgui_internal.h
    dependencies/imgui/imgui.h
    dependencies/imgui/backends/imgui_impl_win32.h
    dependencies/imgui/backends/imgui_impl_dx11.h

    Input/Buttons.h
    Input/ControllerButton.h
    Input/ControllerManager.h
    Input/InputProcessor.h
    Input/InputStateManager.h
    Input/KeyCodes.h
    Input/Keys.h
    Input/MouseButton.h

    JsonCpp/json.h
    JsonCpp/json-forwards.h

    Networking/NetworkManager.h
    Networking/Packet.h
    Networking/Socket.h

    NewRenderer/D3D11/D3D11_Utils.h

    NewRenderer/ComputePipelineState.h
    NewRenderer/GraphicsPipelineState.h
    NewRenderer/Camera.h
    NewRenderer/CBs.h
    NewRenderer/DesertModels.h
    NewRenderer/LargeScaleDesertModel.h
    NewRenderer/SmallScaleDesertModel.h
    NewRenderer/LargeScaleDesertModel_Rasterization.h
    NewRenderer/ScalarField2D.h
    NewRenderer/IRenderer.h
    NewRenderer/LightComponents.h
    NewRenderer/Mesh.h
    NewRenderer/Renderer.h
    NewRenderer/RenderingComponent.h
    NewRenderer/Shader.h
    NewRenderer/Vertex.h
    NewRenderer/WICTextureLoader.h

    Physics/CollisionFunctions.h
    Physics/PhysicsComponent.h
    Physics/Plane.h
    Physics/Sample.h

    Platform/Platform.h

    Util/HashedString.h
    Util/StringUtil.h
    Util/SystemCheck.h
    Util/UIDGenerator.h
)

# target_link_libraries(Engine Game d3d11 dxgi winmm d3dcompiler xinput)
target_link_libraries(FarlorEngine
    PUBLIC FMath::FMath
    PUBLIC spdlog::spdlog_header_only
    PUBLIC tinyobjloader
    PUBLIC AGZSMAA
    PUBLIC d3d11
    PUBLIC dxgi
    PUBLIC winmm
    PUBLIC d3dcompiler
    PUBLIC xinput
    PUBLIC Ws2_32
    PUBLIC dxguid
    PUBLIC DirectXTK
    PRIVATE xxHash::xxhash
    PUBLIC OpenEXR::OpenEXR
    PUBLIC Bullet3Common
    PUBLIC LinearMath
    PUBLIC BulletInverseDynamics
    PUBLIC BulletCollision
    PUBLIC BulletDynamics
    PUBLIC BulletSoftBody
)

target_include_directories(FarlorEngine
    PUBLIC dependencies/imgui/
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_compile_definitions(FarlorEngine
    PUBLIC _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
)

add_subdirectory(Tools)