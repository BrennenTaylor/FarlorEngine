#include "FMath/Vector3.h"
#include "ImathPlatform.h"
#include <DirectXMath.h>
#define WIN32_LEAN_AND_MEAN

#include "Engine.h"
#include "Window.h"

#include "../Util/HashedString.h"

#include "../Input/InputStateManager.h"

#include "../ECS/EntityManager.h"

#include "FixedUpdate.h"

#include "../Physics/PhysicsSystem.h"

#include "../NewRenderer/Camera.h"
#include "../NewRenderer/RenderingComponent.h"
#include "../NewRenderer/DesertModels.h"

#include "../ECS/TransformManager.h"

#include "../JsonCpp/json.h"

#include "../Util/Logger.h"

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_win32.h"

#include <btBulletDynamicsCommon.h>

#include <fstream>

constexpr float FARLOR_PI = 3.14159265f;

constexpr float DegreeToRad(const float degreeAngle) { return degreeAngle * (FARLOR_PI / 180.0f); }

namespace Farlor {
// Global Declarations of engine systems
// EventManager g_EventManager;
TransformManager g_TransformManager;

Engine::Engine()
    : m_gameWindow((*this))
    , m_renderingSystem((*this))
    , m_cameras()
{
}

void Engine::Initialize(EngineConfig config)
{
    m_fullscreen = config.m_fullscreen;
    m_vsync = config.m_vsync;

    // Set up imgui
    // TODO: Should this be owned by the engine class?
    IMGUI_CHECKVERSION();

    // Creates the context we will use for this application.
    // Returns a context which we could cache, but we can alsays call GetCurrentContext() to get, since we only have one.
    ImGui::CreateContext();
    // Allows access to the io structure.
    ImGui::StyleColorsDark();

    // Create and show the game window
    m_gameWindow.Initialize(m_fullscreen);
    m_renderingSystem.Initialize(m_gameWindow);
    m_cameras[0].SetViewport(
          m_gameWindow.GetWidth(), m_gameWindow.GetHeight(), 0.1f, 10000.0f, 0.4f * DirectX::XM_PI);
    m_cameras[1].SetViewport(
          m_gameWindow.GetWidth(), m_gameWindow.GetHeight(), 0.1f, 10000.0f, 0.4f * DirectX::XM_PI);
}

int Engine::Run()
{
    m_gameWindow.ShowGameWindow();

    // Ok, all this could really be initialization shit
    ECSManager entityManager;

    // Timing code
    const double dt = 1000.0 / 60.0 / 1000.0;

    // Read in Scene
    Json::Value sceneRoot;
    std::fstream sceneConfig("configs/EngineConfig.json");
    sceneConfig >> sceneRoot;

    // Create entities
    auto entities = sceneRoot["scene"]["entities"];
    for (auto entity : entities) {
        Entity newEntity = entityManager.CreateEntity();
        FARLOR_LOG_INFO("Creating Entity with ID: {}", newEntity);

        auto transformComponentValue = entity["TransformComponent"];
        if (!transformComponentValue.isNull()) {
            TransformComponent *pEntityTransformComponent
                  = g_TransformManager.RegisterEntity(newEntity);
            pEntityTransformComponent->m_entity = newEntity;
            pEntityTransformComponent->m_position.x
                  = transformComponentValue["position"]["x"].asFloat();
            pEntityTransformComponent->m_position.y
                  = transformComponentValue["position"]["y"].asFloat();
            pEntityTransformComponent->m_position.z
                  = transformComponentValue["position"]["z"].asFloat();

            pEntityTransformComponent->m_rotation.x
                  = DegreeToRad(transformComponentValue["rotation"]["x"].asFloat());
            pEntityTransformComponent->m_rotation.y
                  = DegreeToRad(transformComponentValue["rotation"]["y"].asFloat());
            pEntityTransformComponent->m_rotation.z
                  = DegreeToRad(transformComponentValue["rotation"]["z"].asFloat());

            pEntityTransformComponent->m_scale.x = transformComponentValue["scale"]["x"].asFloat();
            pEntityTransformComponent->m_scale.y = transformComponentValue["scale"]["y"].asFloat();
            pEntityTransformComponent->m_scale.z = transformComponentValue["scale"]["z"].asFloat();
            FARLOR_LOG_INFO("Loaded transform component")
        }

        auto physicsComponentValue = entity["PhysicsComponent"];
        if (!physicsComponentValue.isNull()) {
            PhysicsComponent component;
            component.m_entity = newEntity;
            component.m_mass = physicsComponentValue["mass"].asFloat();

            m_physicsSystem.AddComponent(component);
            FARLOR_LOG_INFO("Loaded physics component")
        }

        auto renderingComponentValue = entity["RenderingComponent"];
        if (!renderingComponentValue.isNull()) {
            RenderingComponent component;
            component.m_entity = newEntity;
            component.m_mesh = HashedString(renderingComponentValue["mesh"].asString());

            m_renderingSystem.RegisterEntityAsRenderable(newEntity, component);
            FARLOR_LOG_INFO("Loaded rendering component")
        }

        auto lightComponentValue = entity["LightComponent"];
        if (!lightComponentValue.isNull()) {
            LightComponent component;
            component.m_entity = newEntity;

            component.m_direction.x = lightComponentValue["direction"]["x"].asFloat();
            component.m_direction.y = lightComponentValue["direction"]["y"].asFloat();
            component.m_direction.z = lightComponentValue["direction"]["z"].asFloat();

            component.m_color.x = lightComponentValue["color"]["r"].asFloat();
            component.m_color.y = lightComponentValue["color"]["g"].asFloat();
            component.m_color.z = lightComponentValue["color"]["b"].asFloat();

            component.m_lightRange.y = lightComponentValue["light_range"].asFloat();
            component.m_lightRange.x = lightComponentValue["light_range"].asFloat();
            component.m_lightRange.z = lightComponentValue["light_range"].asFloat();

            m_renderingSystem.RegisterEntityAsLight(newEntity, component);
            FARLOR_LOG_INFO("Loaded lighting component")
        }
    }

    Json::Value renderConfigRoot;
    std::fstream renderConfig("configs/RendererConfig.json");
    renderConfig >> renderConfigRoot;

    auto rendererGlobalResources = renderConfigRoot["global-resources"];
    auto rendererGlobalOptions = renderConfigRoot["options"];

    bool vsync = rendererGlobalOptions["vsync"].asBool();

    for (auto &resource : rendererGlobalResources) {
        std::string name = resource["name"].asString();
        std::string type = resource["type"].asString();
        int widthScale = resource["width-scale"].asInt();
        int heightScale = resource["height-scale"].asInt();
        std::string format = resource["format"].asString();

        m_renderingSystem.RegisterGlobalResource(name, type, widthScale, heightScale, format);
    }

    m_renderingSystem.CreateGlobalResources();

    ShaderBlobManager &shaderBlobManager = m_renderingSystem.GetShaderBlobManager();

    // TODO: This should be a seperate function or location
    // Load game shaders
    Json::Value shadersConfigRoot;
    std::fstream shaderConfig("configs/ShaderConfig.json");
    shaderConfig >> shadersConfigRoot;

    auto shaderIncludePaths = shadersConfigRoot["includePaths"];
    for (const auto &includePathVal : shaderIncludePaths) {
        std::string includePath = includePathVal.asString();
        shaderBlobManager.RegisterIncludeDirectoryRelative(includePath);
    }
    auto shaders = shadersConfigRoot["shaders"];
    for (auto &shader : shaders) {
        std::string filename = shader["filename"].asString();
        std::string shaderName = shader["name"].asString();

        IncludedShaders includedShaders = IncludedShaders::Flags_None;

        includedShaders = includedShaders
              | (shader["hasVS"].asBool() ? IncludedShaders::Flags_Vertex
                                          : IncludedShaders::Flags_None);
        includedShaders = includedShaders
              | (shader["hasPS"].asBool() ? IncludedShaders::Flags_Pixel
                                          : IncludedShaders::Flags_None);
        includedShaders = includedShaders
              | (shader["hasTess"].asBool() ? IncludedShaders::Flags_EnableTess
                                            : IncludedShaders::Flags_None);
        includedShaders = includedShaders
              | (shader["hasGS"].asBool() ? IncludedShaders::Flags_Geometry
                                          : IncludedShaders::Flags_None);
        includedShaders = includedShaders
              | (shader["hasCS"].asBool() ? IncludedShaders::Flags_Compute
                                          : IncludedShaders::Flags_None);

        if ((includedShaders % 2) == 1 && (includedShaders > 1)) {
            FARLOR_LOG_ERROR(
                  "Attempted to create shader that is both compute and graphics: " + shaderName);
            continue;
        }

        std::vector<std::string> defines;
        auto defineValues = shader["defines"];
        for (auto &define : defineValues) {
            auto val = define.asString();
            defines.push_back(val);
        }

        m_renderingSystem.RegisterShader(filename, shaderName, includedShaders, defines);
    }

    m_renderingSystem.CreateShaders();
    // Load the game resources

    // Frame counting averages
    const int numFrameCount = 10;
    std::array<float, numFrameCount> frameTimes;
    float runningTotal = 0.0f;
    int currentFrame = 0;

    // These are all the fixed updates
    FixedUpdate physicsUpdate { static_cast<float>(dt) };

    m_timerMaster.Reset();

    const float lightRadius = 500.0f;
    float angle = 0.0f;
    float angleSpeed = 0.1f;

    Entity animatedLight = entityManager.CreateEntity();
    TransformComponent *pAnimatedLightTransform = g_TransformManager.RegisterEntity(animatedLight);
    pAnimatedLightTransform->m_entity = animatedLight;
    pAnimatedLightTransform->m_position
          = Farlor::Vector3(std::sin(angle) * lightRadius, 200.0f, std::cos(angle) * lightRadius);
    pAnimatedLightTransform->m_rotation = Farlor::Vector3(0.0f, 0.0f, 0.0f);
    pAnimatedLightTransform->m_scale = Farlor::Vector3(1.0f, 1.0f, 1.0f);

    LightComponent *pAnimatedLightLightComponent
          = m_renderingSystem.RegisterEntityAsLight(animatedLight);
    pAnimatedLightLightComponent->m_color = Farlor::Vector3(23.47f, 21.31f, 20.79f);

    // This is the game loop
    while (m_running) {
        m_timerMaster.Tick();
        m_inputStateManager.Tick();

        const ImGuiIO &imguiIO = ImGui::GetIO();
        if (imguiIO.WantCaptureKeyboard || imguiIO.WantCaptureMouse) {
            m_inputStateManager.Clear();
        }

        const float frameTime = m_timerMaster.DeltaTime();

        // Frame timing stuff

        m_frameTimes.push_back(frameTime);
        if (m_frameTimes.size() > 100) {
            m_frameTimes.pop_front();
        }

        frameTimes[currentFrame] = frameTime;

        if (currentFrame >= numFrameCount) {
            std::stringstream titleTextStream;
            titleTextStream << "Farlor Engine: " << (int)(1.0f / (runningTotal / numFrameCount));
            m_gameWindow.SetWindowTitleText(titleTextStream.str());
            currentFrame = 0;
            runningTotal = 0.0;
        }

        physicsUpdate.AccumulateTime(frameTime);

        angle += angleSpeed * dt;
        if (angle > (2.0f * M_PI)) {
            angle -= (2.0f * M_PI);
        }

        TransformComponent *pAnimatedLightTransform
              = g_TransformManager.GetComponent(animatedLight);
        pAnimatedLightTransform->m_position = Farlor::Vector3(
              std::sin(angle) * lightRadius, 200.0f, std::cos(angle) * lightRadius);

        // Process messages
        m_gameWindow.ProcessSystemMessages();

        const InputState &latestInputState = m_inputStateManager.GetLatestInputState();
        HandleInput(latestInputState);

        if (m_renderingSystem.LargeDesertSimulation().WasTerrainUpdated()) {
            const std::vector<float> &terrainValues
                  = m_renderingSystem.LargeDesertSimulation().GetCachedTerrainValues();
            m_physicsSystem.UpdateTerrainValues(terrainValues);
            std::cout << "Updated terrain values" << std::endl;
            m_renderingSystem.LargeDesertSimulation().HandleTerrainUpdated();
        }

        m_physicsSystem.Tick();

        for (auto &camera : m_cameras) {
            camera.Update(frameTime, latestInputState, m_physicsSystem);
        }

        // Draw our scene
        m_renderingSystem.Render(m_cameras[m_currentCameraIdx], frameTime);
    }

    FARLOR_LOG_INFO("Shutting down the engine system");

    m_renderingSystem.Shutdown();

    // // Shutdown subsystems here
    m_gameWindow.Shutdown();

    return 0;
}

void Engine::HandleInput(const InputState &latestInputState)
{
    switch (latestInputState.m_latestInputType) {
        case InputType::CONTROLLER: {
            if (latestInputState.m_controllerState.m_controllerButtons[ControllerButtons::Start]
                        .endedDown) {
                m_running = false;
            }
        } break;

        case InputType::KEYBOARD_MOUSE:
        default: {
            if (latestInputState.m_keyboardButtonStates[KeyboardButtons::Escape].endedDown) {
                m_running = false;
            }
        } break;
    };

    switch (latestInputState.m_latestInputType) {
        case InputType::KEYBOARD_MOUSE:
        default: {
            if (latestInputState.m_keyboardButtonStates[KeyboardButtons::F].endedDown
                  && latestInputState.m_keyboardButtonStates[KeyboardButtons::F].numHalfSteps
                        == 1) {
                m_renderingSystem.ResetSimulation();
            }
        } break;
    };

    switch (latestInputState.m_latestInputType) {
        case InputType::KEYBOARD_MOUSE:
        default: {
            if (latestInputState.m_keyboardButtonStates[KeyboardButtons::P].endedDown
                  && latestInputState.m_keyboardButtonStates[KeyboardButtons::P].numHalfSteps
                        == 1) {
                m_renderingSystem.TriggerSequenceExport();
            }
        } break;
    };
}
}
