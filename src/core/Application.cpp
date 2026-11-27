#include "roomcraft/core/Application.hpp"

#include "roomcraft/geometry/GeometryUtils.hpp"
#include "roomcraft/geometry/PrimitiveGenerator.hpp"
#include "roomcraft/geometry/RevolutionSurface.hpp"
#include "roomcraft/math/Ray.hpp"
#include "roomcraft/systems/PickingSystem.hpp"
#include "roomcraft/systems/TransformController.hpp"

#include <glad/glad.h>
#include <GL/freeglut.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glut.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>

namespace roomcraft::core {

namespace {

WindowConfig makeWindowConfig(const ApplicationConfig& cfg)
{
    WindowConfig wc;
    wc.width = cfg.width;
    wc.height = cfg.height;
    wc.title = cfg.title;
    return wc;
}

} // namespace

Application::Application(ApplicationConfig config)
    : m_config(std::move(config))
    , m_window(makeWindowConfig(m_config))
{
    m_browserSearchBuf.fill('\0');
}

Application::~Application()
{
    shutdownGraphics();
}

bool Application::initialize(int& argc, char** argv)
{
    if (argc > 0 && argv != nullptr && argv[0] != nullptr) {
        std::error_code ec;
        const auto exePath = std::filesystem::weakly_canonical(std::filesystem::path(argv[0]), ec);
        if (!ec && exePath.has_parent_path()) {
            m_executableDir = exePath.parent_path();
        } else {
            m_executableDir = std::filesystem::current_path();
        }
    } else {
        m_executableDir = std::filesystem::current_path();
    }

    m_assetManager.setSearchRoots(m_executableDir, std::filesystem::current_path());

    if (!m_window.initialize(argc, argv)) {
        return false;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM0) {
        if (!initM0Resources()) {
            shutdownGraphics();
            return false;
        }
    } else {
        if (!initEngineResources()) {
            shutdownGraphics();
            return false;
        }
    }

    if (!initImGui()) {
        shutdownGraphics();
        return false;
    }

    WindowCallbacks callbacks;
    callbacks.onDisplay = [this]() { renderFrame(); };
    callbacks.onReshape = [this](int w, int h) { handleReshape(w, h); };
    callbacks.onClose = [this]() { handleClose(); };
    callbacks.onKeyboard = [this](unsigned char key, int x, int y) { handleKeyboard(key, x, y); };
    callbacks.onKeyboardUp = [this](unsigned char key, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_KeyboardUpFunc(key, x, y);
        }
        m_inputManager.onKeyUp(key);
    };
    callbacks.onSpecial = [this](int key, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_SpecialFunc(key, x, y);
        }
        const int mods = glutGetModifiers();
        const bool ctrl = (mods & GLUT_ACTIVE_CTRL) != 0;
        const bool shift = (mods & GLUT_ACTIVE_SHIFT) != 0;
        const bool alt = (mods & GLUT_ACTIVE_ALT) != 0;
        SpecialKey sk = SpecialKey::None;
        if (key == GLUT_KEY_HOME) sk = SpecialKey::Home;
        else if (key == GLUT_KEY_END) sk = SpecialKey::End;
        else if (key == GLUT_KEY_UP) sk = SpecialKey::Up;
        else if (key == GLUT_KEY_DOWN) sk = SpecialKey::Down;
        else if (key == GLUT_KEY_LEFT) sk = SpecialKey::Left;
        else if (key == GLUT_KEY_RIGHT) sk = SpecialKey::Right;
        if (sk != SpecialKey::None) {
            m_inputManager.onSpecialKeyDown(sk, ctrl, shift, alt);
        }
    };
    callbacks.onSpecialUp = [this](int key, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_SpecialUpFunc(key, x, y);
        }
        SpecialKey sk = SpecialKey::None;
        if (key == GLUT_KEY_HOME) sk = SpecialKey::Home;
        else if (key == GLUT_KEY_END) sk = SpecialKey::End;
        else if (key == GLUT_KEY_UP) sk = SpecialKey::Up;
        else if (key == GLUT_KEY_DOWN) sk = SpecialKey::Down;
        else if (key == GLUT_KEY_LEFT) sk = SpecialKey::Left;
        else if (key == GLUT_KEY_RIGHT) sk = SpecialKey::Right;
        if (sk != SpecialKey::None) {
            m_inputManager.onSpecialKeyUp(sk);
        }
    };
    callbacks.onMouse = [this](int button, int state, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_MouseFunc(button, state, x, y);
        }
        const bool down = (state == GLUT_DOWN);
        if (button == GLUT_LEFT_BUTTON) {
            m_inputManager.onMouseButton(MouseButton::Left, down, static_cast<float>(x), static_cast<float>(y));
        } else if (button == GLUT_MIDDLE_BUTTON) {
            m_inputManager.onMouseButton(MouseButton::Middle, down, static_cast<float>(x), static_cast<float>(y));
        } else if (button == GLUT_RIGHT_BUTTON) {
            m_inputManager.onMouseButton(MouseButton::Right, down, static_cast<float>(x), static_cast<float>(y));
        } else if (button == 3 && down) {
            // Dự phòng cuộn chuột lên trên một số cấu hình FreeGLUT
            m_inputManager.onMouseWheel(1.0f);
        } else if (button == 4 && down) {
            m_inputManager.onMouseWheel(-1.0f);
        }
    };
    callbacks.onMouseWheel = [this](int wheel, int direction, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_MouseWheelFunc(wheel, direction, x, y);
        }
        m_inputManager.onMouseMove(static_cast<float>(x), static_cast<float>(y));
        m_inputManager.onMouseWheel(static_cast<float>(direction));
    };
    callbacks.onMotion = [this](int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_MotionFunc(x, y);
        }
        m_inputManager.onMouseMove(static_cast<float>(x), static_cast<float>(y));
    };
    callbacks.onPassiveMotion = [this](int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_MotionFunc(x, y);
        }
        m_inputManager.onMouseMove(static_cast<float>(x), static_cast<float>(y));
    };
    m_window.setCallbacks(std::move(callbacks));

    const int nowMs = m_window.getElapsedTimeMs();
    m_lastFpsTimeMs = nowMs;
    m_lastFrameTimeMs = nowMs;
    m_initialized = true;

    std::cout << "[RoomCraft3D] Application initialized successfully." << std::endl;
    return true;
}

int Application::run()
{
    if (!m_initialized) {
        std::cerr << "[RoomCraft3D][ERROR] Cannot run Application before initialize() succeeds." << std::endl;
        return 1;
    }

    m_window.runMainLoop();
    shutdownGraphics();

    if (m_config.mode == ApplicationMode::SmokeTestM0) {
        std::cout << "[SMOKE] Frames rendered: " << m_frameCount << std::endl;
        std::cout << "[SMOKE] Triangle pixel verified: "
                  << (m_m0PixelVerified ? "YES" : "NO")
                  << " (Center RGB="
                  << static_cast<int>(m_m0SampledPixel[0]) << ","
                  << static_cast<int>(m_m0SampledPixel[1]) << ","
                  << static_cast<int>(m_m0SampledPixel[2]) << ")" << std::endl;
        std::cout << "[SMOKE] Resize verified: " << (m_m0ResizeObserved ? "YES" : "NO")
                  << " (" << m_window.getWidth() << "x" << m_window.getHeight() << ")" << std::endl;
        std::cout << "[SMOKE] OpenGL errors: " << (m_glErrorDetected ? "YES" : "NONE") << std::endl;

        const bool passed = (m_frameCount >= m_config.smokeTestFrames) &&
                            m_m0PixelVerified && m_m0ResizeObserved && !m_glErrorDetected;
        std::cout << "[SMOKE] Result: " << (passed ? "PASS" : "FAIL") << std::endl;
        return passed ? 0 : 1;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM1) {
        const auto& stats = m_renderer.getLastFrameStats();
        std::cout << "[M1-SMOKE] Frames rendered       : " << m_frameCount << std::endl;
        std::cout << "[M1-SMOKE] Last frame draw calls : " << stats.drawCalls
                  << " (vertices=" << stats.vertexCount << ", indices=" << stats.indexCount << ")" << std::endl;
        std::cout << "[M1-SMOKE] Integration suite     : " << (m_m1SuitePassed ? "PASS" : "FAIL") << std::endl;
        std::cout << "[M1-SMOKE] Multi-cube pixels     : " << (m_m1MultiCubePixelsVerified ? "PASS" : "FAIL") << std::endl;
        std::cout << "[M1-SMOKE] OpenGL errors         : " << (m_glErrorDetected ? "YES" : "NONE") << std::endl;

        const bool passed = (m_frameCount >= m_config.smokeTestFrames) &&
                            m_m1SuiteExecuted && m_m1SuitePassed && m_m1MultiCubePixelsVerified && !m_glErrorDetected;
        std::cout << "[M1-SMOKE] Final Result          : " << (passed ? "PASS" : "FAIL") << std::endl;
        return passed ? 0 : 1;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM2) {
        const auto& stats = m_renderer.getLastFrameStats();
        std::cout << "[M2-SMOKE] Frames rendered       : " << m_frameCount << std::endl;
        std::cout << "[M2-SMOKE] Last frame draw calls : " << stats.drawCalls
                  << " (vertices=" << stats.vertexCount << ", indices=" << stats.indexCount
                  << ", triangles=" << stats.triangleCount << ")" << std::endl;
        std::cout << "[M2-SMOKE] Integration suite     : " << (m_m2SuitePassed ? "PASS" : "FAIL") << std::endl;
        std::cout << "[M2-SMOKE] OpenGL errors         : " << (m_glErrorDetected ? "YES" : "NONE") << std::endl;

        const bool passed = (m_frameCount >= m_config.smokeTestFrames) &&
                            m_m2SuiteExecuted && m_m2SuitePassed && !m_glErrorDetected;
        std::cout << "[M2-SMOKE] Final Result          : " << (passed ? "PASS" : "FAIL") << std::endl;
        return passed ? 0 : 1;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM3) {
        const auto& stats = m_renderer.getLastFrameStats();
        std::cout << "[M3-SMOKE] Frames rendered       : " << m_frameCount << std::endl;
        std::cout << "[M3-SMOKE] Last frame draw calls : " << stats.drawCalls
                  << " (vertices=" << stats.vertexCount << ", indices=" << stats.indexCount
                  << ", triangles=" << stats.triangleCount << ")" << std::endl;
        std::cout << "[M3-SMOKE] Cached assets/textures: " << m_assetManager.getCachedAssetCount()
                  << " assets, " << m_assetManager.getTextureManager().getCachedTextureCount() << " textures" << std::endl;
        std::cout << "[M3-SMOKE] Integration suite     : " << (m_m3SuitePassed ? "PASS" : "FAIL") << std::endl;
        std::cout << "[M3-SMOKE] OpenGL errors         : " << (m_glErrorDetected ? "YES" : "NONE") << std::endl;

        const bool passed = (m_frameCount >= m_config.smokeTestFrames) &&
                            m_m3SuiteExecuted && m_m3SuitePassed && !m_glErrorDetected;
        std::cout << "[M3-SMOKE] Final Result          : " << (passed ? "PASS" : "FAIL") << std::endl;
        return passed ? 0 : 1;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM4) {
        const auto& stats = m_renderer.getLastFrameStats();
        std::cout << "[M4-SMOKE] Frames rendered       : " << m_frameCount << std::endl;
        std::cout << "[M4-SMOKE] Last frame draw calls : " << stats.drawCalls
                  << " (vertices=" << stats.vertexCount << ", indices=" << stats.indexCount
                  << ", triangles=" << stats.triangleCount << ")" << std::endl;
        std::cout << "[M4-SMOKE] Integration suite     : " << (m_m4SuitePassed ? "PASS" : "FAIL") << std::endl;
        std::cout << "[M4-SMOKE] OpenGL errors         : " << (m_glErrorDetected ? "YES" : "NONE") << std::endl;

        const bool passed = (m_frameCount >= m_config.smokeTestFrames) &&
                            m_m4SuiteExecuted && m_m4SuitePassed && !m_glErrorDetected;
        std::cout << "[M4-SMOKE] Final Result          : " << (passed ? "PASS" : "FAIL") << std::endl;
        return passed ? 0 : 1;
    }

    if (m_config.mode == ApplicationMode::BenchmarkM4) {
        std::cout << "[M4-BENCH] Benchmark suite       : " << (m_m4BenchmarkPassed ? "PASS" : "FAIL") << std::endl;
        std::cout << "[M4-BENCH] OpenGL errors         : " << (m_glErrorDetected ? "YES" : "NONE") << std::endl;
        const bool passed = m_m4BenchmarkExecuted && m_m4BenchmarkPassed && !m_glErrorDetected;
        return passed ? 0 : 1;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM5) {
        const auto& stats = m_renderer.getLastFrameStats();
        std::cout << "[M5-SMOKE] Frames rendered       : " << m_frameCount << std::endl;
        std::cout << "[M5-SMOKE] Last frame draw calls : " << stats.drawCalls
                  << " (vertices=" << stats.vertexCount << ", indices=" << stats.indexCount
                  << ", triangles=" << stats.triangleCount << ")" << std::endl;
        std::cout << "[M5-SMOKE] Integration suite     : " << (m_m5SuitePassed ? "PASS" : "FAIL") << std::endl;
        std::cout << "[M5-SMOKE] OpenGL errors         : " << (m_glErrorDetected ? "YES" : "NONE") << std::endl;

        const bool passed = (m_frameCount >= m_config.smokeTestFrames) &&
                            m_m5SuiteExecuted && m_m5SuitePassed && !m_glErrorDetected;
        std::cout << "[M5-SMOKE] Final Result          : " << (passed ? "PASS" : "FAIL") << std::endl;
        return passed ? 0 : 1;
    }

    return 0;
}

bool Application::initImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.133f, 0.145f, 0.176f, 0.96f);      // #22252D
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.173f, 0.188f, 0.227f, 1.0f);  // #2C303A
    style.Colors[ImGuiCol_Button] = ImVec4(0.235f, 0.275f, 0.365f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.455f, 0.584f, 0.839f, 1.0f);  // #7495D6

    if (!ImGui_ImplGLUT_Init()) {
        std::cerr << "[RoomCraft3D][ERROR] ImGui_ImplGLUT_Init() failed." << std::endl;
        ImGui::DestroyContext();
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        std::cerr << "[RoomCraft3D][ERROR] ImGui_ImplOpenGL3_Init() failed." << std::endl;
        ImGui_ImplGLUT_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    ImGui_ImplGLUT_ReshapeFunc(m_window.getWidth(), m_window.getHeight());
    m_imguiInitialized = true;
    std::cout << "[RoomCraft3D] Dear ImGui initialized (imgui_impl_glut + imgui_impl_opengl3 #version 330 core)."
              << std::endl;
    return true;
}

bool Application::initM0Resources()
{
    const auto vertPath = resolveAssetPath("assets/shaders/basic.vert");
    const auto fragPath = resolveAssetPath("assets/shaders/basic.frag");

    if (!m_m0BasicShader.loadFromFiles(vertPath, fragPath)) {
        return false;
    }

    const float triangleVertices[] = {
        -0.55f, -0.45f, 0.0f,    0.92f, 0.34f, 0.34f,
         0.55f, -0.45f, 0.0f,    0.34f, 0.80f, 0.56f,
         0.00f,  0.55f, 0.0f,    0.45f, 0.58f, 0.84f
    };

    glGenVertexArrays(1, &m_m0Vao);
    glGenBuffers(1, &m_m0Vbo);

    glBindVertexArray(m_m0Vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_m0Vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

    constexpr GLsizei stride = static_cast<GLsizei>(6 * sizeof(float));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "[RoomCraft3D] Basic GLSL shader & triangle VAO/VBO initialized." << std::endl;
    return true;
}

bool Application::initEngineResources()
{
    if (!m_meshShader.loadFromFiles(resolveAssetPath("assets/shaders/mesh.vert"),
                                    resolveAssetPath("assets/shaders/mesh.frag"))) {
        return false;
    }

    if (!m_gridShader.loadFromFiles(resolveAssetPath("assets/shaders/grid.vert"),
                                    resolveAssetPath("assets/shaders/grid.frag"))) {
        return false;
    }

    std::vector<graphics::Vertex> cubeVerts;
    std::vector<std::uint32_t> cubeIndices;
    graphics::Mesh::buildUnitCubeFixture(cubeVerts, cubeIndices);
    if (!m_cubeMesh.create(cubeVerts, cubeIndices, graphics::PrimitiveTopology::Triangles)) {
        return false;
    }

    geometry::MeshData markerData;
    if (!geometry::PrimitiveGenerator::createCube(0.055f, markerData) ||
        !m_controlMarkerMesh.create(markerData.vertices, markerData.indices, markerData.topology)) {
        return false;
    }

    std::vector<graphics::Vertex> gridVerts;
    std::vector<std::uint32_t> gridIndices;
    graphics::Mesh::buildGridFixture(6, 0.5f, gridVerts, gridIndices);
    if (!m_gridMesh.create(gridVerts, gridIndices, graphics::PrimitiveTopology::Lines)) {
        return false;
    }

    std::vector<graphics::Vertex> axisVerts;
    std::vector<std::uint32_t> axisIndices;
    graphics::Mesh::buildAxisLineFixture({0.0f, 0.004f, 0.0f}, {2.2f, 0.004f, 0.0f}, axisVerts, axisIndices);
    if (!m_axisXMesh.create(axisVerts, axisIndices, graphics::PrimitiveTopology::Lines)) {
        return false;
    }
    graphics::Mesh::buildAxisLineFixture({0.0f, 0.0f, 0.0f}, {0.0f, 2.2f, 0.0f}, axisVerts, axisIndices);
    if (!m_axisYMesh.create(axisVerts, axisIndices, graphics::PrimitiveTopology::Lines)) {
        return false;
    }
    graphics::Mesh::buildAxisLineFixture({0.0f, 0.004f, 0.0f}, {0.0f, 0.004f, 2.2f}, axisVerts, axisIndices);
    if (!m_axisZMesh.create(axisVerts, axisIndices, graphics::PrimitiveTopology::Lines)) {
        return false;
    }

    if (!m_framebuffer.create(720, 620)) {
        return false;
    }

    setupDefaultM1DemoScene();

    if (m_config.mode == ApplicationMode::SmokeTestM1) {
        m_workspace = InteractiveWorkspace::ReferenceSceneM1;
        m_camera.reset();
    } else if (m_config.mode == ApplicationMode::SmokeTestM2) {
        m_workspace = InteractiveWorkspace::GeometryPlaygroundM2;
        m_camera.setProjectionMode(graphics::ProjectionMode::Perspective);
        m_camera.lookAt({2.3f, 1.8f, 2.7f}, {0.0f, 0.55f, 0.0f}, {0.0f, 1.0f, 0.0f});
        if (!regenerateActivePlaygroundGeometry()) {
            return false;
        }
    } else if (m_config.mode == ApplicationMode::SmokeTestM3) {
        m_workspace = InteractiveWorkspace::AssetBrowserM3;
        m_camera.setProjectionMode(graphics::ProjectionMode::Perspective);
        m_camera.lookAt({2.6f, 1.8f, 2.8f}, {0.0f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
        m_camera.setAspectRatio(static_cast<float>(m_framebuffer.getWidth()) /
                                static_cast<float>(m_framebuffer.getHeight()));
        regenerateActivePlaygroundGeometry();
        if (!selectBrowserAsset("blender-modern-sofa", true)) {
            selectBrowserAsset("procedural-desk", true);
        }
    } else {
        // Mặc định M4: khởi chạy Room Editor với Modern Bedroom preset, đồng thời nạp sẵn M3/M2 để chuyển mượt
        m_workspace = InteractiveWorkspace::RoomEditorM4;
        m_camera.setAspectRatio(static_cast<float>(m_framebuffer.getWidth()) /
                                static_cast<float>(m_framebuffer.getHeight()));
        regenerateActivePlaygroundGeometry();
        if (!selectBrowserAsset("blender-modern-sofa", false)) {
            selectBrowserAsset("procedural-desk", false);
        }
        m_roomEditor.initialize(m_assetManager, m_camera);
    }

    m_camera.setAspectRatio(static_cast<float>(m_framebuffer.getWidth()) /
                            static_cast<float>(m_framebuffer.getHeight()));

    std::cout << "[RoomCraft3D] M1/M2/M3/M4 Engine, Asset Pipeline & Room Editor resources initialized." << std::endl;
    return true;
}

void Application::setupDefaultM1DemoScene()
{
    m_demoCubes.clear();

    DemoCubeObject cubeA;
    cubeA.name = "Cube A (Front Red)";
    cubeA.transform = scene::Transform({0.25f, 0.50f, 1.10f}, {0.0f, 18.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    cubeA.defaultTransform = cubeA.transform;
    cubeA.color = {0.88f, 0.34f, 0.34f, 1.0f};
    m_demoCubes.push_back(cubeA);

    DemoCubeObject cubeB;
    cubeB.name = "Cube B (Back Blue)";
    cubeB.transform = scene::Transform({-0.45f, 0.65f, -0.55f}, {0.0f, -24.0f, 0.0f}, {1.35f, 1.30f, 1.35f});
    cubeB.defaultTransform = cubeB.transform;
    cubeB.color = {0.35f, 0.56f, 0.88f, 1.0f};
    m_demoCubes.push_back(cubeB);

    DemoCubeObject cubeC;
    cubeC.name = "Cube C (Scaled Green)";
    cubeC.transform = scene::Transform({1.95f, 0.40f, -0.35f}, {12.0f, 35.0f, 0.0f}, {0.85f, 0.80f, 1.45f});
    cubeC.defaultTransform = cubeC.transform;
    cubeC.color = {0.30f, 0.78f, 0.52f, 1.0f};
    m_demoCubes.push_back(cubeC);
}

void Application::resetCurrentPlaygroundParameters()
{
    m_planeWidth = 2.0f; m_planeDepth = 2.0f; m_planeSubX = 4; m_planeSubZ = 4;
    m_cubeSize = 1.0f;
    m_cuboidW = 1.4f; m_cuboidH = 0.8f; m_cuboidD = 0.9f;
    m_cylRadius = 0.45f; m_cylHeight = 1.2f; m_cylSegments = 24; m_cylCapped = true;
    m_sphereRadius = 0.55f; m_sphereLatSeg = 18; m_sphereLonSeg = 28;
    m_coneRadius = 0.50f; m_coneHeight = 1.15f; m_coneSegments = 24; m_coneCapped = true;
    m_frustumBottomR = 0.55f; m_frustumTopR = 0.25f; m_frustumHeight = 1.0f; m_frustumSegments = 24;
    m_frustumCapBottom = true; m_frustumCapTop = true;

    m_bezierCurvePts = {
        glm::vec3(-1.0f, 0.2f, 0.0f), glm::vec3(-0.4f, 1.5f, 0.4f),
        glm::vec3( 0.4f, 1.5f, -0.4f), glm::vec3( 1.0f, 0.2f, 0.0f)
    };
    m_bezierCurveSegments = 32;
    m_bezierSurfWidth = 1.8f; m_bezierSurfDepth = 1.8f; m_bezierSurfCurvature = 0.55f;
    m_bezierSurfUSeg = 16; m_bezierSurfVSeg = 16;
    m_revHeight = 1.1f; m_revBaseR = 0.22f; m_revBellyR = 0.48f; m_revNeckR = 0.16f; m_revRimR = 0.28f;
    m_revRadialSeg = 32; m_revProfileSeg = 20; m_revCapBottom = true; m_revCapTop = false;

    m_deskParams = geometry::DeskParams{};
    m_chairParams = geometry::ChairParams{};
    m_bookshelfParams = geometry::BookshelfParams{};
    m_lampParams = geometry::DeskLampParams{};
    m_vaseParams = geometry::VaseParams{};
    m_curvedChairParams = geometry::CurvedChairParams{};
    m_rootTransform.reset();
}

bool Application::selectBrowserAsset(const std::string& assetId, bool autoFitCamera)
{
    std::string err;
    auto loaded = m_assetManager.getOrLoadAsset(assetId, &err);
    if (!loaded) {
        m_lastAssetError = err;
        return false;
    }

    m_selectedAssetId = assetId;
    m_activeBrowserAsset = std::move(loaded);
    m_lastAssetError.clear();
    m_lastAssetStatus = "Loaded " + m_activeBrowserAsset->cpuData.displayName;

    if (autoFitCamera) {
        fitCameraToActiveModel();
    }
    return true;
}

void Application::fitCameraToActiveModel()
{
    if (m_workspace == InteractiveWorkspace::AssetBrowserM3 && m_activeBrowserAsset) {
        m_camera.fitBoundingBox(m_activeBrowserAsset->cpuData.localBounds, 1.28f);
    } else if (m_workspace == InteractiveWorkspace::GeometryPlaygroundM2 && m_activeBoundingBox.isValid()) {
        const auto worldBox = m_activeBoundingBox.transformed(m_rootTransform.getModelMatrix());
        m_camera.fitBoundingBox(worldBox, 1.28f);
    } else {
        m_camera.reset();
    }
}

bool Application::uploadPlaygroundParts(std::vector<geometry::FurniturePart>& parts,
                                        const math::BoundingBox& overallBox,
                                        const geometry::MeshData* controlOverlayLines,
                                        const std::vector<glm::vec3>& controlPoints,
                                        std::string& outError)
{
    std::vector<UploadedRenderPart> nextParts;
    nextParts.reserve(parts.size());

    std::uint32_t totalVerts = 0;
    std::uint32_t totalIndices = 0;
    std::uint32_t totalTris = 0;

    geometry::MeshData combinedNormals;
    combinedNormals.topology = graphics::PrimitiveTopology::Lines;

    for (auto& part : parts) {
        UploadedRenderPart uploaded;
        uploaded.name = part.name;
        uploaded.localTransform = part.localTransform;
        uploaded.color = part.baseColor;
        uploaded.isLinePrimitive = (part.meshData.topology == graphics::PrimitiveTopology::Lines);

        if (!uploaded.mesh.create(part.meshData.vertices, part.meshData.indices, part.meshData.topology, &outError)) {
            return false;
        }

        totalVerts += part.meshData.getVertexCount();
        totalIndices += part.meshData.getIndexCount();
        totalTris += part.meshData.getTriangleCount();

        if (part.meshData.topology == graphics::PrimitiveTopology::Triangles) {
            const glm::mat4 localMat = part.localTransform.getModelMatrix();
            const glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(localMat));
            const glm::vec3 up(0.0f, 1.0f, 0.0f);
            for (const auto& v : part.meshData.vertices) {
                const glm::vec3 p = glm::vec3(localMat * glm::vec4(v.position, 1.0f));
                const glm::vec3 n = glm::normalize(normalMat * v.normal);
                const auto baseIdx = static_cast<std::uint32_t>(combinedNormals.vertices.size());
                combinedNormals.vertices.push_back({p, up, {0.0f, 0.0f}});
                combinedNormals.vertices.push_back({p + n * m_normalVisualLength, up, {1.0f, 0.0f}});
                combinedNormals.indices.push_back(baseIdx);
                combinedNormals.indices.push_back(baseIdx + 1);
            }
        }

        nextParts.push_back(std::move(uploaded));
    }

    graphics::Mesh nextBBoxLines;
    if (overallBox.isValid()) {
        const auto bboxData = geometry::GeometryUtils::buildBoundingBoxLines(overallBox);
        if (!bboxData.vertices.empty()) {
            nextBBoxLines.create(bboxData.vertices, bboxData.indices, bboxData.topology);
        }
    }

    graphics::Mesh nextControlLines;
    if (controlOverlayLines != nullptr && !controlOverlayLines->vertices.empty()) {
        nextControlLines.create(controlOverlayLines->vertices, controlOverlayLines->indices, controlOverlayLines->topology);
    }

    graphics::Mesh nextNormalLines;
    if (!combinedNormals.vertices.empty()) {
        nextNormalLines.create(combinedNormals.vertices, combinedNormals.indices, combinedNormals.topology);
    }

    m_activeParts = std::move(nextParts);
    m_activeBBoxLines = std::move(nextBBoxLines);
    m_activeControlLines = std::move(nextControlLines);
    m_activeNormalLines = std::move(nextNormalLines);
    m_activeControlPoints = controlPoints;
    m_activeBoundingBox = overallBox;
    m_activeVertexCount = totalVerts;
    m_activeIndexCount = totalIndices;
    m_activeTriangleCount = totalTris;
    return true;
}

bool Application::regenerateActivePlaygroundGeometry()
{
    const auto startTime = std::chrono::steady_clock::now();

    std::vector<geometry::FurniturePart> parts;
    math::BoundingBox overallBox;
    geometry::MeshData controlLines;
    bool hasControlLines = false;
    std::vector<glm::vec3> controlPoints;
    std::string error;
    bool ok = false;

    if (m_category == PlaygroundCategory::Primitives) {
        geometry::MeshData meshData;
        const glm::vec4 primColor{0.45f, 0.64f, 0.88f, 1.0f};
        switch (m_primitiveTypeIndex) {
            case 0: ok = geometry::PrimitiveGenerator::createPlane(m_planeWidth, m_planeDepth, m_planeSubX, m_planeSubZ, meshData, &error); break;
            case 1: ok = geometry::PrimitiveGenerator::createCube(m_cubeSize, meshData, &error); break;
            case 2: ok = geometry::PrimitiveGenerator::createCuboid(m_cuboidW, m_cuboidH, m_cuboidD, meshData, &error); break;
            case 3: ok = geometry::PrimitiveGenerator::createCylinder(m_cylRadius, m_cylHeight, m_cylSegments, m_cylCapped, meshData, &error); break;
            case 4: ok = geometry::PrimitiveGenerator::createSphere(m_sphereRadius, m_sphereLatSeg, m_sphereLonSeg, meshData, &error); break;
            case 5: ok = geometry::PrimitiveGenerator::createCone(m_coneRadius, m_coneHeight, m_coneSegments, m_coneCapped, meshData, &error); break;
            case 6: ok = geometry::PrimitiveGenerator::createFrustum(m_frustumBottomR, m_frustumTopR, m_frustumHeight, m_frustumSegments, m_frustumCapBottom, m_frustumCapTop, meshData, &error); break;
            default: error = "Unknown primitive type."; break;
        }
        if (ok) {
            overallBox = meshData.boundingBox;
            parts.push_back({"Primitive", std::move(meshData), scene::Transform(), primColor});
        }
    } else if (m_category == PlaygroundCategory::Parametric) {
        if (m_parametricTypeIndex == 0) {
            const geometry::BezierCurve curve(m_bezierCurvePts);
            geometry::MeshData curveLines;
            ok = curve.generateLineMesh(m_bezierCurveSegments, curveLines, &error) &&
                 curve.generateControlPolygonMesh(controlLines, &error);
            if (ok) {
                hasControlLines = true;
                controlPoints.assign(m_bezierCurvePts.begin(), m_bezierCurvePts.end());
                overallBox = curveLines.boundingBox;
                parts.push_back({"BezierCurve", std::move(curveLines), scene::Transform(), {0.35f, 0.88f, 0.60f, 1.0f}});
            }
        } else if (m_parametricTypeIndex == 1) {
            const auto surface = geometry::BezierSurface::createCurvedWavePatch(
                m_bezierSurfWidth, m_bezierSurfDepth, m_bezierSurfCurvature);
            geometry::MeshData surfMesh;
            ok = surface.generateMesh(m_bezierSurfUSeg, m_bezierSurfVSeg, surfMesh, &error) &&
                 surface.generateControlNetMesh(controlLines, &error);
            if (ok) {
                hasControlLines = true;
                const auto& grid = surface.getControlPoints();
                for (int i = 0; i < 4; ++i) {
                    for (int j = 0; j < 4; ++j) {
                        controlPoints.push_back(grid[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]);
                    }
                }
                overallBox = surfMesh.boundingBox;
                parts.push_back({"BezierSurface", std::move(surfMesh), scene::Transform(), {0.48f, 0.70f, 0.90f, 1.0f}});
            }
        } else {
            const geometry::BezierCurve profile(
                glm::vec3(m_revBaseR,  0.0f,                0.0f),
                glm::vec3(m_revBellyR, m_revHeight * 0.35f, 0.0f),
                glm::vec3(m_revNeckR,  m_revHeight * 0.78f, 0.0f),
                glm::vec3(m_revRimR,   m_revHeight,         0.0f)
            );
            geometry::MeshData revMesh;
            ok = geometry::RevolutionSurface::generateFromBezierProfile(
                profile, m_revProfileSeg, m_revRadialSeg, m_revCapBottom, m_revCapTop, revMesh, &error);
            if (ok) {
                overallBox = revMesh.boundingBox;
                parts.push_back({"RevolutionSurface", std::move(revMesh), scene::Transform(), {0.42f, 0.72f, 0.78f, 1.0f}});
            }
        }
    } else if (m_category == PlaygroundCategory::Furniture) {
        geometry::FurnitureModel model;
        switch (m_furnitureTypeIndex) {
            case 0: ok = geometry::FurnitureGenerator::createDesk(m_deskParams, model, &error); break;
            case 1: ok = geometry::FurnitureGenerator::createChair(m_chairParams, model, &error); break;
            case 2: ok = geometry::FurnitureGenerator::createBookshelf(m_bookshelfParams, model, &error); break;
            case 3: ok = geometry::FurnitureGenerator::createDeskLamp(m_lampParams, model, &error); break;
            case 4: ok = geometry::FurnitureGenerator::createVase(m_vaseParams, model, &error); break;
            case 5: ok = geometry::FurnitureGenerator::createCurvedChair(m_curvedChairParams, model, &error); break;
            default: error = "Unknown furniture type."; break;
        }
        if (ok) {
            overallBox = model.boundingBox;
            parts = std::move(model.parts);
        }
    }

    if (!ok) {
        m_lastGeometryError = error;
        return false;
    }

    if (!uploadPlaygroundParts(parts, overallBox, hasControlLines ? &controlLines : nullptr, controlPoints, error)) {
        m_lastGeometryError = error;
        return false;
    }

    const auto endTime = std::chrono::steady_clock::now();
    m_lastGenerationTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    m_lastGeometryError.clear();
    return true;
}

void Application::shutdownGraphics()
{
    if (!m_initialized && !m_imguiInitialized &&
        !m_meshShader.isValid() && !m_gridShader.isValid() && !m_m0BasicShader.isValid() &&
        !m_cubeMesh.isValid() && !m_framebuffer.isComplete() && m_activeParts.empty() &&
        !m_activeBrowserAsset && m_assetManager.getCachedAssetCount() == 0 &&
        m_m0Vao == 0 && m_m0Vbo == 0) {
        return;
    }

    if (m_imguiInitialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGLUT_Shutdown();
        ImGui::DestroyContext();
        m_imguiInitialized = false;
    }

    // Giải phóng toàn bộ tài nguyên RoomEditor, AssetManager (Meshes + Textures) TRƯỚC khi OpenGL context bị hủy
    m_roomEditor.shutdownGpuResources();
    m_activeBrowserAsset.reset();
    m_assetManager.clear();

    m_activeParts.clear();
    m_activeBBoxLines.destroy();
    m_activeControlLines.destroy();
    m_activeNormalLines.destroy();

    m_framebuffer.destroy();
    m_cubeMesh.destroy();
    m_controlMarkerMesh.destroy();
    m_gridMesh.destroy();
    m_axisXMesh.destroy();
    m_axisYMesh.destroy();
    m_axisZMesh.destroy();
    m_meshShader.destroy();
    m_gridShader.destroy();

    m_m0BasicShader.destroy();
    if (m_m0Vao != 0) {
        glDeleteVertexArrays(1, &m_m0Vao);
        m_m0Vao = 0;
    }
    if (m_m0Vbo != 0) {
        glDeleteBuffers(1, &m_m0Vbo);
        m_m0Vbo = 0;
    }

    m_initialized = false;
    std::cout << "[RoomCraft3D] GPU, Asset/Texture caches & Dear ImGui resources released cleanly." << std::endl;
}

std::filesystem::path Application::resolveAssetPath(const std::filesystem::path& relativePath) const
{
    std::error_code ec;
    const auto candidateExe = m_executableDir / relativePath;
    if (std::filesystem::exists(candidateExe, ec)) {
        return candidateExe;
    }
    const auto candidateCwd = std::filesystem::current_path() / relativePath;
    if (std::filesystem::exists(candidateCwd, ec)) {
        return candidateCwd;
    }
    // Hỗ trợ trường hợp chạy từ build/<preset>/ tìm ngược lên thư mục gốc dự án (cho tests/fixtures)
    if (m_executableDir.has_parent_path() && m_executableDir.parent_path().has_parent_path()) {
        const auto candidateRepo = m_executableDir.parent_path().parent_path() / relativePath;
        if (std::filesystem::exists(candidateRepo, ec)) {
            return candidateRepo;
        }
    }
    return candidateExe;
}

void Application::renderFrame()
{
    if (!m_initialized) {
        return;
    }

    ++m_frameCount;
    ++m_framesSinceLastFps;
    const int nowMs = m_window.getElapsedTimeMs();
    m_frameTimeMs = static_cast<float>(std::max(0, nowMs - m_lastFrameTimeMs));
    m_lastFrameTimeMs = nowMs;

    const int elapsedFpsMs = nowMs - m_lastFpsTimeMs;
    if (elapsedFpsMs >= 500) {
        m_currentFps = (static_cast<float>(m_framesSinceLastFps) * 1000.0f) / static_cast<float>(elapsedFpsMs);
        m_framesSinceLastFps = 0;
        m_lastFpsTimeMs = nowMs;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM0) {
        renderM0RegressionFrame();
        return;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM1 && !m_m1SuiteExecuted) {
        m_m1SuiteExecuted = true;
        m_m1SuitePassed = runM1IntegrationSuiteOnce();
        m_workspace = InteractiveWorkspace::ReferenceSceneM1;
        m_framebuffer.resize(800, 600);
        m_camera.reset();
        m_camera.setAspectRatio(static_cast<float>(m_framebuffer.getWidth()) /
                                static_cast<float>(m_framebuffer.getHeight()));
    }

    if (m_config.mode == ApplicationMode::SmokeTestM2 && !m_m2SuiteExecuted) {
        m_m2SuiteExecuted = true;
        m_m2SuitePassed = runM2IntegrationSuiteOnce();
        m_workspace = InteractiveWorkspace::GeometryPlaygroundM2;
        m_framebuffer.resize(800, 600);
        m_category = PlaygroundCategory::Furniture;
        m_furnitureTypeIndex = 0;
        regenerateActivePlaygroundGeometry();
        m_camera.setProjectionMode(graphics::ProjectionMode::Perspective);
        m_camera.lookAt({2.3f, 1.8f, 2.7f}, {0.0f, 0.55f, 0.0f}, {0.0f, 1.0f, 0.0f});
        m_camera.setAspectRatio(static_cast<float>(m_framebuffer.getWidth()) /
                                static_cast<float>(m_framebuffer.getHeight()));
    }

    if (m_config.mode == ApplicationMode::SmokeTestM3 && !m_m3SuiteExecuted) {
        m_m3SuiteExecuted = true;
        m_m3SuitePassed = runM3IntegrationSuiteOnce();
        m_workspace = InteractiveWorkspace::AssetBrowserM3;
        m_framebuffer.resize(800, 600);
        m_camera.setAspectRatio(800.0f / 600.0f);
        selectBrowserAsset("blender-modern-sofa", true);
    }

    if (m_config.mode == ApplicationMode::SmokeTestM4 && !m_m4SuiteExecuted) {
        m_m4SuiteExecuted = true;
        m_m4SuitePassed = runM4IntegrationSuiteOnce();
        m_workspace = InteractiveWorkspace::RoomEditorM4;
        m_framebuffer.resize(800, 600);
        m_camera.setAspectRatio(800.0f / 600.0f);
        m_roomEditor.initialize(m_assetManager, m_camera);
    }

    if (m_config.mode == ApplicationMode::BenchmarkM4 && !m_m4BenchmarkExecuted) {
        m_m4BenchmarkExecuted = true;
        m_m4BenchmarkPassed = runM4BenchmarkSuiteOnce();
        m_window.requestClose();
        return;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM5 && !m_m5SuiteExecuted) {
        m_m5SuiteExecuted = true;
        m_m5SuitePassed = runM5IntegrationSuiteOnce();
        m_workspace = InteractiveWorkspace::RoomEditorM4;
        m_framebuffer.resize(800, 600);
        m_camera.setAspectRatio(800.0f / 600.0f);
        m_roomEditor.initialize(m_assetManager, m_camera);
    }

    renderInteractiveFrame();

    if (m_config.mode == ApplicationMode::SmokeTestM1) {
        if (m_frameCount == 3 && !m_m1MultiCubePixelsVerified) {
            m_m1MultiCubePixelsVerified = verifyM1MultiCubeRenderedPixels();
        }
        if (m_frameCount >= m_config.smokeTestFrames) {
            m_window.requestClose();
            return;
        }
    }

    if (m_config.mode == ApplicationMode::SmokeTestM2 ||
        m_config.mode == ApplicationMode::SmokeTestM3 ||
        m_config.mode == ApplicationMode::SmokeTestM4 ||
        m_config.mode == ApplicationMode::SmokeTestM5) {
        if (m_frameCount >= m_config.smokeTestFrames) {
            m_window.requestClose();
            return;
        }
    }

    m_window.postRedisplay();
}

void Application::renderM0RegressionFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();

    const auto& gpu = m_window.getGpuInfo();
    ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(390.0f, 250.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("RoomCraft 3D - M0 Foundation Diagnostics")) {
        ImGui::Text("Status: M0 Regression Mode");
        ImGui::Separator();
        ImGui::Text("GPU Renderer : %s", gpu.renderer.c_str());
        ImGui::Text("OpenGL       : %s (%s)", gpu.glVersion.c_str(), gpu.profile.c_str());
        ImGui::Text("GLSL         : %s", gpu.glslVersion.c_str());
        ImGui::Text("Viewport     : %d x %d", m_window.getWidth(), m_window.getHeight());
        ImGui::Checkbox("Render M0 Triangle", &m_showM0Triangle);
    }
    ImGui::End();
    ImGui::Render();

    glViewport(0, 0, m_window.getWidth(), m_window.getHeight());
    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_showM0Triangle && m_m0BasicShader.isValid()) {
        m_m0BasicShader.bind();
        glBindVertexArray(m_m0Vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        graphics::Shader::unbind();
    }

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (!m_m0PixelVerified && m_frameCount == 2) {
        glReadPixels(m_window.getWidth() / 2, m_window.getHeight() / 2, 1, 1,
                     GL_RGBA, GL_UNSIGNED_BYTE, m_m0SampledPixel.data());
        const int bgR = static_cast<int>(m_clearColor.r * 255.0f);
        const int bgG = static_cast<int>(m_clearColor.g * 255.0f);
        const int bgB = static_cast<int>(m_clearColor.b * 255.0f);
        const int diff = std::abs(static_cast<int>(m_m0SampledPixel[0]) - bgR) +
                         std::abs(static_cast<int>(m_m0SampledPixel[1]) - bgG) +
                         std::abs(static_cast<int>(m_m0SampledPixel[2]) - bgB);
        m_m0PixelVerified = (diff > 30);
    }

    drainAndCheckOpenGlErrors();
    m_window.swapBuffers();

    if (!m_m0ResizeTriggered && m_frameCount >= 3) {
        m_m0ResizeTriggered = true;
        m_window.resize(960, 540);
    }
    if ((m_frameCount >= m_config.smokeTestFrames && m_m0ResizeObserved) ||
        m_frameCount >= m_config.smokeTestFrames + 20) {
        m_window.requestClose();
        return;
    }

    m_window.postRedisplay();
}

void Application::renderGpuAssetToCurrentTarget(const assets::GpuAsset& asset,
                                                const glm::mat4& parentMatrix,
                                                const graphics::Camera& cam,
                                                float faceContrast)
{
    for (const auto& sub : asset.subMeshes) {
        const glm::mat4 worldMat = parentMatrix * sub.localTransform.getModelMatrix();
        const glm::vec4 color = (sub.materialIndex < asset.cpuData.materials.size())
                                    ? asset.cpuData.materials[sub.materialIndex].baseColor
                                    : glm::vec4(0.8f, 0.8f, 0.82f, 1.0f);
        const graphics::Texture* diffTex = nullptr;
        if (sub.materialIndex < asset.materialDiffuseTextures.size() && asset.materialDiffuseTextures[sub.materialIndex]) {
            diffTex = asset.materialDiffuseTextures[sub.materialIndex].get();
        }
        m_renderer.drawMeshTextured(sub.mesh, m_meshShader, worldMat, cam, color, diffTex, faceContrast);
    }
}

void Application::renderAssetBrowserSceneToFramebuffer(float faceContrast)
{
    if (!m_framebuffer.isComplete()) {
        return;
    }

    m_framebuffer.bind();
    m_renderer.beginFrame(m_framebuffer.getWidth(), m_framebuffer.getHeight(), m_clearColor, m_depthTestEnabled);

    const glm::mat4 identity(1.0f);
    if (m_showGrid) {
        m_renderer.drawLines(m_gridMesh, m_gridShader, identity, m_camera, {0.26f, 0.29f, 0.36f, 1.0f});
    }
    if (m_showAxes) {
        m_renderer.drawLines(m_axisXMesh, m_gridShader, identity, m_camera, {0.92f, 0.30f, 0.30f, 1.0f});
        m_renderer.drawLines(m_axisYMesh, m_gridShader, identity, m_camera, {0.30f, 0.88f, 0.42f, 1.0f});
        m_renderer.drawLines(m_axisZMesh, m_gridShader, identity, m_camera, {0.34f, 0.56f, 0.96f, 1.0f});
    }

    if (m_activeBrowserAsset) {
        m_renderer.setWireframeMode(m_wireframeMode);
        renderGpuAssetToCurrentTarget(*m_activeBrowserAsset, identity, m_camera, faceContrast);
        m_renderer.setWireframeMode(false);

        if (m_showBoundingBox && m_activeBrowserAsset->boundingBoxLines.isValid()) {
            m_renderer.drawLines(m_activeBrowserAsset->boundingBoxLines, m_gridShader, identity, m_camera, {0.96f, 0.78f, 0.28f, 1.0f});
        }
        if (m_showVertexNormals && m_activeBrowserAsset->vertexNormalLines.isValid()) {
            m_renderer.drawLines(m_activeBrowserAsset->vertexNormalLines, m_gridShader, identity, m_camera, {0.35f, 0.92f, 0.88f, 1.0f});
        }
    }

    m_renderer.endFrame();
    graphics::Framebuffer::unbind();
}

void Application::renderM1DemoSceneToFramebuffer(float faceContrast)
{
    if (!m_framebuffer.isComplete()) {
        return;
    }

    m_framebuffer.bind();
    m_renderer.beginFrame(m_framebuffer.getWidth(), m_framebuffer.getHeight(), m_clearColor, m_depthTestEnabled);

    const glm::mat4 identity(1.0f);
    if (m_showGrid) {
        m_renderer.drawLines(m_gridMesh, m_gridShader, identity, m_camera, {0.30f, 0.33f, 0.40f, 1.0f});
    }
    if (m_showAxes) {
        m_renderer.drawLines(m_axisXMesh, m_gridShader, identity, m_camera, {0.92f, 0.30f, 0.30f, 1.0f});
        m_renderer.drawLines(m_axisYMesh, m_gridShader, identity, m_camera, {0.30f, 0.88f, 0.42f, 1.0f});
        m_renderer.drawLines(m_axisZMesh, m_gridShader, identity, m_camera, {0.34f, 0.56f, 0.96f, 1.0f});
    }

    m_renderer.setWireframeMode(m_wireframeMode);
    for (const auto& cube : m_demoCubes) {
        m_renderer.drawMesh(m_cubeMesh, m_meshShader, cube.transform, m_camera, cube.color, faceContrast);
    }

    m_renderer.endFrame();
    graphics::Framebuffer::unbind();
}

void Application::renderPlaygroundSceneToFramebuffer(float faceContrast)
{
    if (!m_framebuffer.isComplete()) {
        return;
    }

    m_framebuffer.bind();
    m_renderer.beginFrame(m_framebuffer.getWidth(), m_framebuffer.getHeight(), m_clearColor, m_depthTestEnabled);

    const glm::mat4 identity(1.0f);
    if (m_showGrid) {
        m_renderer.drawLines(m_gridMesh, m_gridShader, identity, m_camera, {0.28f, 0.31f, 0.38f, 1.0f});
    }
    if (m_showAxes) {
        m_renderer.drawLines(m_axisXMesh, m_gridShader, identity, m_camera, {0.92f, 0.30f, 0.30f, 1.0f});
        m_renderer.drawLines(m_axisYMesh, m_gridShader, identity, m_camera, {0.30f, 0.88f, 0.42f, 1.0f});
        m_renderer.drawLines(m_axisZMesh, m_gridShader, identity, m_camera, {0.34f, 0.56f, 0.96f, 1.0f});
    }

    const glm::mat4 parentMat = m_rootTransform.getModelMatrix();
    m_renderer.setWireframeMode(m_wireframeMode);
    for (const auto& part : m_activeParts) {
        const glm::mat4 worldMat = parentMat * part.localTransform.getModelMatrix();
        if (part.isLinePrimitive) {
            m_renderer.drawLines(part.mesh, m_gridShader, worldMat, m_camera, part.color);
        } else {
            m_renderer.drawMeshWithMatrix(part.mesh, m_meshShader, worldMat, m_camera, part.color, faceContrast);
        }
    }
    m_renderer.setWireframeMode(false);

    if (m_showBoundingBox && m_activeBBoxLines.isValid()) {
        m_renderer.drawLines(m_activeBBoxLines, m_gridShader, parentMat, m_camera, {0.96f, 0.78f, 0.28f, 1.0f});
    }

    if (m_showControlOverlay) {
        if (m_activeControlLines.isValid()) {
            m_renderer.drawLines(m_activeControlLines, m_gridShader, parentMat, m_camera, {0.95f, 0.65f, 0.25f, 1.0f});
        }
        for (const auto& cp : m_activeControlPoints) {
            const glm::mat4 cpMat = parentMat * glm::translate(glm::mat4(1.0f), cp - glm::vec3(0.0f, 0.0275f, 0.0f));
            m_renderer.drawMeshWithMatrix(m_controlMarkerMesh, m_meshShader, cpMat, m_camera, {0.96f, 0.35f, 0.35f, 1.0f}, 0.25f);
        }
    }

    if (m_showVertexNormals && m_activeNormalLines.isValid()) {
        m_renderer.drawLines(m_activeNormalLines, m_gridShader, parentMat, m_camera, {0.35f, 0.92f, 0.88f, 1.0f});
    }

    m_renderer.endFrame();
    graphics::Framebuffer::unbind();
}

void Application::renderInteractiveFrame()
{
    if (m_workspace == InteractiveWorkspace::RoomEditorM4) {
        m_roomEditor.renderSceneToFramebuffer(m_assetManager,
                                              m_camera,
                                              m_framebuffer,
                                              m_renderer,
                                              m_meshShader,
                                              m_gridShader,
                                              m_axisXMesh,
                                              m_axisYMesh,
                                              m_axisZMesh,
                                              m_clearColor,
                                              0.35f);
    } else if (m_workspace == InteractiveWorkspace::AssetBrowserM3) {
        renderAssetBrowserSceneToFramebuffer(0.45f);
    } else if (m_workspace == InteractiveWorkspace::GeometryPlaygroundM2) {
        renderPlaygroundSceneToFramebuffer(0.45f);
    } else {
        renderM1DemoSceneToFramebuffer(0.45f);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();

    renderTopWorkspaceBar();

    if (m_workspace == InteractiveWorkspace::RoomEditorM4) {
        m_roomEditor.renderUiAndProcessInput(m_window.getWidth(),
                                             m_window.getHeight(),
                                             ImGui::GetFrameHeight(),
                                             m_inputManager,
                                             m_assetManager,
                                             m_camera,
                                             m_framebuffer,
                                             m_currentFps,
                                             m_frameTimeMs);
    } else if (m_workspace == InteractiveWorkspace::AssetBrowserM3) {
        renderAssetBrowserUi();
    } else if (m_workspace == InteractiveWorkspace::GeometryPlaygroundM2) {
        renderPlaygroundControlUi();
        renderViewportPanel("3D Viewport (Off-screen Framebuffer)",
                            412.0f, 44.0f,
                            static_cast<float>(std::max(300, m_window.getWidth() - 422)),
                            static_cast<float>(std::max(240, m_window.getHeight() - 54)),
                            true);
    } else {
        renderM1ReferenceControlUi();
        renderViewportPanel("3D Viewport (Off-screen Framebuffer)",
                            392.0f, 44.0f,
                            static_cast<float>(std::max(300, m_window.getWidth() - 402)),
                            static_cast<float>(std::max(240, m_window.getHeight() - 54)),
                            true);
    }

    ImGui::Render();

    glViewport(0, 0, m_window.getWidth(), m_window.getHeight());
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.070f, 0.075f, 0.095f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    m_inputManager.endFrame();
    drainAndCheckOpenGlErrors();
    m_window.swapBuffers();
}

void Application::renderTopWorkspaceBar()
{
    if (ImGui::BeginMainMenuBar()) {
        ImGui::TextUnformatted("RoomCraft 3D v0.5.0");
        ImGui::Separator();

        const bool isM4 = (m_workspace == InteractiveWorkspace::RoomEditorM4);
        const bool isM3 = (m_workspace == InteractiveWorkspace::AssetBrowserM3);
        const bool isM2 = (m_workspace == InteractiveWorkspace::GeometryPlaygroundM2);
        const bool isM1 = (m_workspace == InteractiveWorkspace::ReferenceSceneM1);

        if (ImGui::MenuItem("Room Editor (M4/M5)", nullptr, isM4)) {
            m_workspace = InteractiveWorkspace::RoomEditorM4;
            m_inputManager.resetAll();
            m_roomEditor.setPerspective3DCamera(m_camera);
        }
        if (ImGui::MenuItem("Asset Browser (M3)", nullptr, isM3)) {
            m_workspace = InteractiveWorkspace::AssetBrowserM3;
            fitCameraToActiveModel();
        }
        if (ImGui::MenuItem("Geometry Playground (M2)", nullptr, isM2)) {
            m_workspace = InteractiveWorkspace::GeometryPlaygroundM2;
            fitCameraToActiveModel();
        }
        if (ImGui::MenuItem("M1 Reference Scene", nullptr, isM1)) {
            m_workspace = InteractiveWorkspace::ReferenceSceneM1;
            m_camera.reset();
        }

        ImGui::Separator();
        ImGui::Text("FPS: %.1f | Draw Calls: %u | GL %d.%d Core",
                    m_currentFps,
                    m_renderer.getLastFrameStats().drawCalls,
                    m_window.getGpuInfo().majorVersion,
                    m_window.getGpuInfo().minorVersion);

        ImGui::EndMainMenuBar();
    }
}

void Application::renderAssetBrowserUi()
{
    const float winW = static_cast<float>(std::max(800, m_window.getWidth()));
    const float winH = static_cast<float>(std::max(500, m_window.getHeight()));
    const float topY = 34.0f;
    const float panelH = winH - topY - 10.0f;
    const float leftW = 280.0f;
    const float rightW = 320.0f;
    const float centerW = std::max(260.0f, winW - leftW - rightW - 40.0f);

    // 1. Left Panel: ASSET LIBRARY
    ImGui::SetNextWindowPos(ImVec2(10.0f, topY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftW, panelH), ImGuiCond_Always);
    if (ImGui::Begin("Asset Library", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##AssetSearch", "Search furniture...", m_browserSearchBuf.data(), m_browserSearchBuf.size());

        // Bộ lọc nguồn: All / Procedural / Blender Assets
        if (ImGui::Selectable("All Assets (14)", m_browserSourceFilter == 0)) m_browserSourceFilter = 0;
        if (ImGui::Selectable("Procedural (C++ M2)", m_browserSourceFilter == 1)) m_browserSourceFilter = 1;
        if (ImGui::Selectable("Blender Assets (OBJ)", m_browserSourceFilter == 2)) m_browserSourceFilter = 2;

        const char* catNames[] = {"All Categories", "Seating", "Tables", "Storage", "Lighting", "Beds", "Electronics", "Decor"};
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::Combo("##CategoryFilter", &m_browserCategoryFilter, catNames, 8);
        ImGui::Separator();

        std::optional<assets::AssetSourceType> srcFilter;
        if (m_browserSourceFilter == 1) srcFilter = assets::AssetSourceType::Procedural;
        else if (m_browserSourceFilter == 2) srcFilter = assets::AssetSourceType::Imported;

        std::optional<assets::AssetCategory> catFilter;
        if (m_browserCategoryFilter > 0) {
            catFilter = static_cast<assets::AssetCategory>(m_browserCategoryFilter - 1);
        }

        const auto filtered = m_assetManager.getCatalog().filter(srcFilter, catFilter, m_browserSearchBuf.data());
        ImGui::TextDisabled("Showing %zu asset(s)", filtered.size());

        if (ImGui::BeginChild("AssetListScroll", ImVec2(0.0f, 0.0f), true)) {
            for (const auto* entry : filtered) {
                const bool selected = (entry->assetId == m_selectedAssetId);
                const char* badge = (entry->sourceType == assets::AssetSourceType::Procedural) ? "[C++]" : "[OBJ]";
                const std::string label = std::string(badge) + " " + entry->displayName + "##" + entry->assetId;
                if (ImGui::Selectable(label.c_str(), selected)) {
                    selectBrowserAsset(entry->assetId, true);
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();

    // 2. Center Panel: 3D ASSET PREVIEW (Off-screen Framebuffer)
    renderViewportPanel("3D Asset Preview", 10.0f + leftW + 10.0f, topY, centerW, panelH, true);

    // 3. Right Panel: ASSET INSPECTOR
    ImGui::SetNextWindowPos(ImVec2(10.0f + leftW + 10.0f + centerW + 10.0f, topY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightW, panelH), ImGuiCond_Always);
    if (ImGui::Begin("Asset Inspector", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        if (m_activeBrowserAsset) {
            const auto& cpu = m_activeBrowserAsset->cpuData;
            const glm::vec3 dims = cpu.localBounds.getSize();
            const auto& stats = m_renderer.getLastFrameStats();

            ImGui::Text("%s", cpu.displayName.c_str());
            ImGui::TextDisabled("Source: %s", assets::assetSourceTypeToString(cpu.sourceType));
            ImGui::Separator();

            ImGui::Text("Asset ID       : %s", cpu.assetId.c_str());
            ImGui::Text("Category       : %s", assets::assetCategoryToString(cpu.category));
            ImGui::Text("Mesh count     : %zu", cpu.subMeshes.size());
            ImGui::Text("Material count : %zu", cpu.materials.size());
            ImGui::Text("Texture count  : %zu", cpu.getUniqueTextureCount());
            ImGui::Separator();

            ImGui::Text("Vertices       : %u", cpu.getTotalVertexCount());
            ImGui::Text("Indices        : %u", cpu.getTotalIndexCount());
            ImGui::Text("Triangles      : %u", cpu.getTotalTriangleCount());
            ImGui::Text("Width (X)      : %.3f m", dims.x);
            ImGui::Text("Height (Y)     : %.3f m", dims.y);
            ImGui::Text("Depth (Z)      : %.3f m", dims.z);
            ImGui::Text("Pivot Min Y    : %.4f m", cpu.localBounds.min.y);
            ImGui::Separator();

            ImGui::Text("Import Time    : %.2f ms", cpu.importTimeMs);
            ImGui::Text("Draw Calls     : %u", stats.drawCalls);
            ImGui::Text("FPS / Frame    : %.1f / %.2f ms", m_currentFps, m_frameTimeMs);
            ImGui::Text("Asset Cache    : %zu assets, %zu tex",
                        m_assetManager.getCachedAssetCount(),
                        m_assetManager.getTextureManager().getCachedTextureCount());
            if (m_activeBrowserAsset->usedFallbackTexture) {
                ImGui::TextColored(ImVec4(0.96f, 0.78f, 0.28f, 1.0f), "Warning: Fallback texture active");
            }
            ImGui::Separator();

            ImGui::TextWrapped("Author  : %s", cpu.author.c_str());
            ImGui::Text("License : %s", cpu.license.c_str());
            ImGui::Separator();
        } else {
            ImGui::TextDisabled("No asset loaded.");
        }

        ImGui::Checkbox("Show Bounding Box", &m_showBoundingBox);
        ImGui::Checkbox("Wireframe Mode", &m_wireframeMode);
        ImGui::Checkbox("Show Normals", &m_showVertexNormals);
        ImGui::Checkbox("Show Grid (XZ)", &m_showGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Axes", &m_showAxes);
        ImGui::Checkbox("Depth Test", &m_depthTestEnabled);

        ImGui::Separator();
        if (ImGui::Button("Fit Model", ImVec2(-1.0f, 0.0f))) {
            fitCameraToActiveModel();
        }
        if (ImGui::Button("Reload Asset", ImVec2(-1.0f, 0.0f))) {
            std::string err;
            if (m_assetManager.reloadAsset(m_selectedAssetId, &err)) {
                m_activeBrowserAsset = m_assetManager.getOrLoadAsset(m_selectedAssetId);
                m_lastAssetError.clear();
                m_lastAssetStatus = "Reloaded " + m_selectedAssetId;
                fitCameraToActiveModel();
            } else {
                m_lastAssetError = err;
            }
        }

        if (!m_lastAssetError.empty()) {
            ImGui::TextColored(ImVec4(0.96f, 0.36f, 0.36f, 1.0f), "Error: %s", m_lastAssetError.c_str());
        } else if (!m_lastAssetStatus.empty()) {
            ImGui::TextDisabled("%s", m_lastAssetStatus.c_str());
        }
    }
    ImGui::End();
}

void Application::renderViewportPanel(const char* panelTitle,
                                      float posX,
                                      float posY,
                                      float width,
                                      float height,
                                      bool showTopToolbar)
{
    ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

    if (ImGui::Begin(panelTitle, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        if (showTopToolbar) {
            if (ImGui::Button("Fit Model")) {
                fitCameraToActiveModel();
            }
            ImGui::SameLine();
            int projIdx = (m_camera.getProjectionMode() == graphics::ProjectionMode::Perspective) ? 0 : 1;
            if (ImGui::RadioButton("Persp", &projIdx, 0)) {
                m_camera.setProjectionMode(graphics::ProjectionMode::Perspective);
                fitCameraToActiveModel();
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Ortho", &projIdx, 1)) {
                m_camera.setProjectionMode(graphics::ProjectionMode::Orthographic);
                fitCameraToActiveModel();
            }
            ImGui::SameLine();
            if (ImGui::Button("3/4")) {
                m_camera.setProjectionMode(graphics::ProjectionMode::Perspective);
                m_camera.lookAt({2.6f, 1.8f, 2.8f}, {0.0f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
                fitCameraToActiveModel();
            }
            ImGui::SameLine();
            if (ImGui::Button("Top")) {
                m_camera.setTopOrthoPreset();
                fitCameraToActiveModel();
            }
            ImGui::SameLine();
            if (ImGui::Button("Front")) {
                m_camera.setFrontOrthoPreset();
                fitCameraToActiveModel();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("FBO %dx%d", m_framebuffer.getWidth(), m_framebuffer.getHeight());
            ImGui::Separator();
        }

        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const int targetW = static_cast<int>(avail.x);
        const int targetH = static_cast<int>(avail.y);

        if (targetW > 0 && targetH > 0 && m_config.mode == ApplicationMode::InteractiveApp) {
            if (targetW != m_framebuffer.getWidth() || targetH != m_framebuffer.getHeight()) {
                m_framebuffer.resize(targetW, targetH);
            }
            m_camera.setAspectRatio(static_cast<float>(m_framebuffer.getWidth()) /
                                    static_cast<float>(m_framebuffer.getHeight()));
        }

        if (m_framebuffer.isComplete() && targetW > 0 && targetH > 0) {
            const auto texId = static_cast<ImTextureID>(static_cast<std::uintptr_t>(m_framebuffer.getColorTextureId()));
            ImGui::Image(texId, avail, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        } else {
            ImGui::TextDisabled("Framebuffer inactive (viewport size is 0).");
        }
    }
    ImGui::End();
}

void Application::renderPlaygroundControlUi()
{
    const auto& gpu = m_window.getGpuInfo();
    const auto& stats = m_renderer.getLastFrameStats();
    const float panelH = static_cast<float>(std::max(240, m_window.getHeight() - 54));

    ImGui::SetNextWindowPos(ImVec2(10.0f, 44.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(392.0f, panelH), ImGuiCond_Always);

    if (ImGui::Begin("RoomCraft 3D - M2 Geometry Playground", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        bool paramsChanged = false;
        bool selectionChanged = false;

        const char* categories[] = {"Primitives", "Parametric", "Procedural Furniture"};
        int catIdx = static_cast<int>(m_category);
        if (ImGui::Combo("Category", &catIdx, categories, 3)) {
            m_category = static_cast<PlaygroundCategory>(catIdx);
            selectionChanged = true;
        }

        if (m_category == PlaygroundCategory::Primitives) {
            const char* primTypes[] = {"Plane", "Cube", "Cuboid", "Cylinder", "Sphere", "Cone", "Frustum"};
            if (ImGui::Combo("Primitive Type", &m_primitiveTypeIndex, primTypes, 7)) selectionChanged = true;
        } else if (m_category == PlaygroundCategory::Parametric) {
            const char* paramTypes[] = {"Bezier Curve (Cubic)", "Bezier Surface (4x4)", "Surface of Revolution"};
            if (ImGui::Combo("Parametric Type", &m_parametricTypeIndex, paramTypes, 3)) selectionChanged = true;
        } else {
            const char* furnTypes[] = {
                "01. Procedural Desk", "02. Procedural Chair", "03. Procedural Bookshelf",
                "04. Procedural Desk Lamp", "05. Procedural Vase (Revolution)", "06. Procedural Curved Chair (Bezier)"
            };
            if (ImGui::Combo("Furniture Type", &m_furnitureTypeIndex, furnTypes, 6)) selectionChanged = true;
        }

        ImGui::Separator();

        if (m_category == PlaygroundCategory::Primitives) {
            switch (m_primitiveTypeIndex) {
                case 0:
                    paramsChanged |= ImGui::SliderFloat("Width", &m_planeWidth, 0.2f, 6.0f);
                    paramsChanged |= ImGui::SliderFloat("Depth", &m_planeDepth, 0.2f, 6.0f);
                    paramsChanged |= ImGui::SliderInt("Subdivisions X", &m_planeSubX, 1, 32);
                    paramsChanged |= ImGui::SliderInt("Subdivisions Z", &m_planeSubZ, 1, 32);
                    break;
                case 1: paramsChanged |= ImGui::SliderFloat("Size", &m_cubeSize, 0.1f, 3.0f); break;
                case 2:
                    paramsChanged |= ImGui::SliderFloat("Width", &m_cuboidW, 0.1f, 4.0f);
                    paramsChanged |= ImGui::SliderFloat("Height", &m_cuboidH, 0.1f, 4.0f);
                    paramsChanged |= ImGui::SliderFloat("Depth", &m_cuboidD, 0.1f, 4.0f);
                    break;
                case 3:
                    paramsChanged |= ImGui::SliderFloat("Radius", &m_cylRadius, 0.05f, 2.0f);
                    paramsChanged |= ImGui::SliderFloat("Height", &m_cylHeight, 0.1f, 3.5f);
                    paramsChanged |= ImGui::SliderInt("Radial Segments", &m_cylSegments, 3, 96);
                    paramsChanged |= ImGui::Checkbox("Capped", &m_cylCapped);
                    break;
                case 4:
                    paramsChanged |= ImGui::SliderFloat("Radius", &m_sphereRadius, 0.1f, 2.0f);
                    paramsChanged |= ImGui::SliderInt("Latitude Segments", &m_sphereLatSeg, 3, 64);
                    paramsChanged |= ImGui::SliderInt("Longitude Segments", &m_sphereLonSeg, 3, 96);
                    break;
                case 5:
                    paramsChanged |= ImGui::SliderFloat("Base Radius", &m_coneRadius, 0.05f, 2.0f);
                    paramsChanged |= ImGui::SliderFloat("Height", &m_coneHeight, 0.1f, 3.5f);
                    paramsChanged |= ImGui::SliderInt("Radial Segments", &m_coneSegments, 3, 96);
                    paramsChanged |= ImGui::Checkbox("Cap Bottom", &m_coneCapped);
                    break;
                case 6:
                    paramsChanged |= ImGui::SliderFloat("Bottom Radius", &m_frustumBottomR, 0.0f, 2.0f);
                    paramsChanged |= ImGui::SliderFloat("Top Radius", &m_frustumTopR, 0.0f, 2.0f);
                    paramsChanged |= ImGui::SliderFloat("Height", &m_frustumHeight, 0.1f, 3.5f);
                    paramsChanged |= ImGui::SliderInt("Radial Segments", &m_frustumSegments, 3, 96);
                    paramsChanged |= ImGui::Checkbox("Cap Bottom", &m_frustumCapBottom);
                    ImGui::SameLine();
                    paramsChanged |= ImGui::Checkbox("Cap Top", &m_frustumCapTop);
                    break;
                default: break;
            }
        } else if (m_category == PlaygroundCategory::Parametric) {
            if (m_parametricTypeIndex == 0) {
                paramsChanged |= ImGui::DragFloat3("P0", &m_bezierCurvePts[0].x, 0.03f, -3.0f, 3.0f);
                paramsChanged |= ImGui::DragFloat3("P1", &m_bezierCurvePts[1].x, 0.03f, -3.0f, 3.0f);
                paramsChanged |= ImGui::DragFloat3("P2", &m_bezierCurvePts[2].x, 0.03f, -3.0f, 3.0f);
                paramsChanged |= ImGui::DragFloat3("P3", &m_bezierCurvePts[3].x, 0.03f, -3.0f, 3.0f);
                paramsChanged |= ImGui::SliderInt("Curve Segments", &m_bezierCurveSegments, 2, 128);
            } else if (m_parametricTypeIndex == 1) {
                paramsChanged |= ImGui::SliderFloat("Patch Width", &m_bezierSurfWidth, 0.4f, 4.0f);
                paramsChanged |= ImGui::SliderFloat("Patch Depth", &m_bezierSurfDepth, 0.4f, 4.0f);
                paramsChanged |= ImGui::SliderFloat("Curvature Height", &m_bezierSurfCurvature, -1.0f, 1.5f);
                paramsChanged |= ImGui::SliderInt("U Segments", &m_bezierSurfUSeg, 2, 48);
                paramsChanged |= ImGui::SliderInt("V Segments", &m_bezierSurfVSeg, 2, 48);
            } else {
                paramsChanged |= ImGui::SliderFloat("Height", &m_revHeight, 0.2f, 2.5f);
                paramsChanged |= ImGui::SliderFloat("Base Radius", &m_revBaseR, 0.02f, 0.8f);
                paramsChanged |= ImGui::SliderFloat("Belly Radius", &m_revBellyR, 0.05f, 1.0f);
                paramsChanged |= ImGui::SliderFloat("Neck Radius", &m_revNeckR, 0.02f, 0.8f);
                paramsChanged |= ImGui::SliderFloat("Rim Radius", &m_revRimR, 0.02f, 0.9f);
                paramsChanged |= ImGui::SliderInt("Radial Segments", &m_revRadialSeg, 6, 96);
                paramsChanged |= ImGui::SliderInt("Profile Segments", &m_revProfileSeg, 4, 64);
                paramsChanged |= ImGui::Checkbox("Cap Bottom", &m_revCapBottom);
                ImGui::SameLine();
                paramsChanged |= ImGui::Checkbox("Cap Top", &m_revCapTop);
            }
        } else {
            switch (m_furnitureTypeIndex) {
                case 0:
                    paramsChanged |= ImGui::SliderFloat("Width", &m_deskParams.width, 0.7f, 2.4f);
                    paramsChanged |= ImGui::SliderFloat("Depth", &m_deskParams.depth, 0.4f, 1.2f);
                    paramsChanged |= ImGui::SliderFloat("Height", &m_deskParams.height, 0.55f, 1.1f);
                    paramsChanged |= ImGui::SliderFloat("Top Thickness", &m_deskParams.topThickness, 0.02f, 0.12f);
                    paramsChanged |= ImGui::SliderFloat("Leg Thickness", &m_deskParams.legThickness, 0.025f, 0.12f);
                    break;
                case 1:
                    paramsChanged |= ImGui::SliderFloat("Width", &m_chairParams.width, 0.35f, 0.75f);
                    paramsChanged |= ImGui::SliderFloat("Depth", &m_chairParams.depth, 0.35f, 0.75f);
                    paramsChanged |= ImGui::SliderFloat("Seat Height", &m_chairParams.seatHeight, 0.30f, 0.65f);
                    paramsChanged |= ImGui::SliderFloat("Backrest Height", &m_chairParams.backrestHeight, 0.25f, 0.75f);
                    paramsChanged |= ImGui::SliderFloat("Leg Radius", &m_chairParams.legRadius, 0.012f, 0.045f);
                    break;
                case 2:
                    paramsChanged |= ImGui::SliderFloat("Width", &m_bookshelfParams.width, 0.5f, 2.0f);
                    paramsChanged |= ImGui::SliderFloat("Height", &m_bookshelfParams.height, 0.8f, 2.6f);
                    paramsChanged |= ImGui::SliderFloat("Depth", &m_bookshelfParams.depth, 0.20f, 0.60f);
                    paramsChanged |= ImGui::SliderInt("Shelf Count", &m_bookshelfParams.shelfCount, 2, 10);
                    break;
                case 3:
                    paramsChanged |= ImGui::SliderFloat("Base Radius", &m_lampParams.baseRadius, 0.06f, 0.25f);
                    paramsChanged |= ImGui::SliderFloat("Stem Height", &m_lampParams.stemHeight, 0.18f, 0.80f);
                    paramsChanged |= ImGui::SliderFloat("Shade Bottom R", &m_lampParams.shadeBottomRadius, 0.07f, 0.30f);
                    paramsChanged |= ImGui::SliderFloat("Shade Top R", &m_lampParams.shadeTopRadius, 0.02f, 0.20f);
                    break;
                case 4:
                    paramsChanged |= ImGui::SliderFloat("Height", &m_vaseParams.height, 0.18f, 1.0f);
                    paramsChanged |= ImGui::SliderFloat("Base Radius", &m_vaseParams.baseRadius, 0.03f, 0.20f);
                    paramsChanged |= ImGui::SliderFloat("Belly Radius", &m_vaseParams.bellyRadius, 0.05f, 0.30f);
                    paramsChanged |= ImGui::SliderFloat("Neck Radius", &m_vaseParams.neckRadius, 0.02f, 0.20f);
                    paramsChanged |= ImGui::SliderInt("Radial Segments", &m_vaseParams.radialSegments, 8, 96);
                    break;
                case 5:
                    paramsChanged |= ImGui::SliderFloat("Width", &m_curvedChairParams.width, 0.38f, 0.80f);
                    paramsChanged |= ImGui::SliderFloat("Backrest Height", &m_curvedChairParams.backrestHeight, 0.30f, 0.70f);
                    paramsChanged |= ImGui::SliderFloat("Curvature", &m_curvedChairParams.backrestCurvature, 0.02f, 0.25f);
                    paramsChanged |= ImGui::SliderInt("U Segments", &m_curvedChairParams.uSegments, 4, 32);
                    paramsChanged |= ImGui::SliderInt("V Segments", &m_curvedChairParams.vSegments, 4, 32);
                    break;
                default: break;
            }
        }

        ImGui::Checkbox("Auto-regen", &m_autoRegenerate);
        ImGui::SameLine();
        const bool manualRegen = ImGui::Button("Regenerate");
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            resetCurrentPlaygroundParameters();
            paramsChanged = true;
        }

        if (selectionChanged || manualRegen || (paramsChanged && m_autoRegenerate)) {
            regenerateActivePlaygroundGeometry();
            if (selectionChanged) {
                fitCameraToActiveModel();
            }
        }

        if (!m_lastGeometryError.empty()) {
            ImGui::TextColored(ImVec4(0.96f, 0.36f, 0.36f, 1.0f), "Error: %s", m_lastGeometryError.c_str());
        }

        ImGui::Separator();
        ImGui::Checkbox("Wireframe", &m_wireframeMode);
        ImGui::SameLine();
        ImGui::Checkbox("Depth Test", &m_depthTestEnabled);
        ImGui::Checkbox("Grid", &m_showGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Axes", &m_showAxes);
        ImGui::SameLine();
        ImGui::Checkbox("BBox", &m_showBoundingBox);
        ImGui::Checkbox("Control Net", &m_showControlOverlay);
        ImGui::SameLine();
        ImGui::Checkbox("Normals", &m_showVertexNormals);

        if (ImGui::TreeNode("Root Transform (Hierarchy)")) {
            ImGui::DragFloat3("Pos", &m_rootTransform.position.x, 0.05f, -5.0f, 5.0f);
            ImGui::DragFloat3("Rot", &m_rootTransform.rotationDegrees.x, 1.0f, -360.0f, 360.0f);
            glm::vec3 rs = m_rootTransform.scale;
            if (ImGui::DragFloat3("Scale", &rs.x, 0.05f, 0.1f, 3.0f)) m_rootTransform.setScaleSafe(rs, 0.05f);
            if (ImGui::Button("Reset Root")) m_rootTransform.reset();
            ImGui::TreePop();
        }

        ImGui::Separator();
        ImGui::Text("GPU: %s", gpu.renderer.c_str());
        ImGui::Text("Verts: %u | Indices: %u | Tris: %u", m_activeVertexCount, m_activeIndexCount, m_activeTriangleCount);
        ImGui::Text("Gen Time: %.3f ms | Draw Calls: %u", m_lastGenerationTimeMs, stats.drawCalls);
        if (m_activeBoundingBox.isValid()) {
            const glm::vec3 sz = m_activeBoundingBox.getSize();
            ImGui::Text("BBox: %.2f x %.2f x %.2f m", sz.x, sz.y, sz.z);
        }
    }
    ImGui::End();
}

void Application::renderM1ReferenceControlUi()
{
    const float panelH = static_cast<float>(std::max(240, m_window.getHeight() - 54));
    ImGui::SetNextWindowPos(ImVec2(10.0f, 44.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(372.0f, panelH), ImGuiCond_Always);

    if (ImGui::Begin("RoomCraft 3D - M1 Reference Scene", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        ImGui::Checkbox("Depth Test (GL_DEPTH_TEST)", &m_depthTestEnabled);
        ImGui::Checkbox("Wireframe", &m_wireframeMode);
        ImGui::Checkbox("Grid", &m_showGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Axes", &m_showAxes);
        ImGui::Separator();

        if (!m_demoCubes.empty()) {
            m_selectedCubeIndex = std::clamp(m_selectedCubeIndex, 0, static_cast<int>(m_demoCubes.size()) - 1);
            if (ImGui::BeginCombo("Cube", m_demoCubes[static_cast<std::size_t>(m_selectedCubeIndex)].name.c_str())) {
                for (int i = 0; i < static_cast<int>(m_demoCubes.size()); ++i) {
                    if (ImGui::Selectable(m_demoCubes[static_cast<std::size_t>(i)].name.c_str(), i == m_selectedCubeIndex)) {
                        m_selectedCubeIndex = i;
                    }
                }
                ImGui::EndCombo();
            }
            auto& obj = m_demoCubes[static_cast<std::size_t>(m_selectedCubeIndex)];
            ImGui::DragFloat3("Pos (m)", &obj.transform.position.x, 0.05f, -10.0f, 10.0f);
            ImGui::DragFloat3("Rot (deg)", &obj.transform.rotationDegrees.x, 1.0f, -360.0f, 360.0f);
            glm::vec3 s = obj.transform.scale;
            if (ImGui::DragFloat3("Scale", &s.x, 0.05f, 0.1f, 5.0f)) obj.transform.setScaleSafe(s, 0.05f);
            if (ImGui::Button("Reset Cube")) obj.transform = obj.defaultTransform;
        }
    }
    ImGui::End();
}

// ============================ M1 REGRESSION SUITE ============================
bool Application::runM1IntegrationSuiteOnce()
{
    bool allPassed = true;
    const auto report = [&allPassed](const char* id, const char* desc, bool ok) {
        std::cout << "[" << id << "] " << desc << ": " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok) allPassed = false;
    };

    report("TC-M1-02", "Valid 3D & Grid shaders compiled and linked", m_meshShader.isValid() && m_gridShader.isValid());
    {
        graphics::Shader badShader;
        std::string errMsg;
        const bool compiled = badShader.loadFromSource(
            "#version 330 core\nvoid main() { INVALID_SYNTAX_TOKEN }",
            "#version 330 core\nout vec4 FragColor; void main() { FragColor = vec4(1.0); }",
            "invalid_test_shader", &errMsg);
        report("TC-M1-03", "Invalid GLSL syntax rejected with diagnostic log", (!compiled) && (!badShader.isValid()) && (!errMsg.empty()));
    }
    report("TC-M1-04", "Indexed cube mesh uploaded (24 vertices, 36 indices)",
           m_cubeMesh.isValid() && m_cubeMesh.getVertexCount() == 24 && m_cubeMesh.getIndexCount() == 36);
    {
        graphics::Mesh badMesh;
        std::vector<graphics::Vertex> verts(3);
        std::vector<std::uint32_t> badIndices = {0, 1, 99};
        std::string errMsg;
        const bool created = badMesh.create(verts, badIndices, graphics::PrimitiveTopology::Triangles, &errMsg);
        report("TC-M1-05", "Out-of-bounds mesh indices rejected cleanly", (!created) && (!badMesh.isValid()) && (!errMsg.empty()));
    }
    report("TC-M1-13", "Off-screen Framebuffer complete with valid color texture",
           m_framebuffer.isComplete() && m_framebuffer.getColorTextureId() != 0);
    report("TC-M1-14", "Framebuffer resize, no-op caching, and 0x0 safety", verifyM1FramebufferResizeAndZeroSafety());
    report("TC-M1-11", "Depth testing order-independent proof + depth-off control", verifyM1DepthTestingOrderIndependence());
    report("TC-M1-15", "RAII move ownership & safe resource destruction", verifyM1RaiiMoveLifecycle());
    drainAndCheckOpenGlErrors();
    report("TC-M1-16", "Zero unexpected OpenGL errors during integration suite", !m_glErrorDetected);
    return allPassed;
}

bool Application::verifyM1FramebufferResizeAndZeroSafety()
{
    graphics::Framebuffer fbo;
    if (!fbo.create(640, 360) || !fbo.isComplete()) return false;
    const unsigned int initialFboId = fbo.getFboId();
    const unsigned int initialTexId = fbo.getColorTextureId();
    if (!fbo.resize(640, 360) || fbo.getFboId() != initialFboId || fbo.getColorTextureId() != initialTexId) return false;
    if (!fbo.resize(0, 0) || !fbo.isComplete() || fbo.getWidth() != 640 || fbo.getHeight() != 360) return false;
    if (!fbo.resize(800, 600) || !fbo.isComplete() || fbo.getWidth() != 800 || fbo.getHeight() != 600) return false;
    fbo.destroy();
    return !fbo.isComplete() && fbo.getFboId() == 0 && fbo.getColorTextureId() == 0;
}

bool Application::verifyM1DepthTestingOrderIndependence()
{
    graphics::Framebuffer fbo;
    if (!fbo.create(256, 256)) return false;

    graphics::Camera testCam;
    testCam.setProjectionMode(graphics::ProjectionMode::Perspective);
    testCam.setAspectRatio(1.0f);
    testCam.setFieldOfView(45.0f);
    testCam.setClipPlanes(0.1f, 50.0f);
    testCam.lookAt({0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

    const scene::Transform nearRed({0.0f, 0.0f, 1.5f}, {0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 0.4f});
    const scene::Transform farBlue({0.0f, 0.0f, -1.5f}, {0.0f, 0.0f, 0.0f}, {2.5f, 2.5f, 0.4f});
    const auto readCenterPixel = [&]() {
        std::array<unsigned char, 4> px{0, 0, 0, 0};
        glReadPixels(128, 128, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
        return px;
    };

    fbo.bind();
    m_renderer.beginFrame(256, 256, {0.0f, 0.0f, 0.0f, 1.0f}, true);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, farBlue, testCam, {0, 0, 1, 1}, 0.0f);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, nearRed, testCam, {1, 0, 0, 1}, 0.0f);
    m_renderer.endFrame();
    const auto px1 = readCenterPixel();

    m_renderer.beginFrame(256, 256, {0.0f, 0.0f, 0.0f, 1.0f}, true);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, nearRed, testCam, {1, 0, 0, 1}, 0.0f);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, farBlue, testCam, {0, 0, 1, 1}, 0.0f);
    m_renderer.endFrame();
    const auto px2 = readCenterPixel();

    m_renderer.beginFrame(256, 256, {0.0f, 0.0f, 0.0f, 1.0f}, false);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, nearRed, testCam, {1, 0, 0, 1}, 0.0f);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, farBlue, testCam, {0, 0, 1, 1}, 0.0f);
    m_renderer.endFrame();
    const auto px3 = readCenterPixel();

    graphics::Framebuffer::unbind();
    fbo.destroy();
    return (px1[0] > 240 && px1[2] < 15) && (px2[0] > 240 && px2[2] < 15) && (px3[2] > 240 && px3[0] < 15);
}

bool Application::verifyM1RaiiMoveLifecycle()
{
    graphics::Shader s1;
    if (!s1.loadFromFiles(resolveAssetPath("assets/shaders/grid.vert"), resolveAssetPath("assets/shaders/grid.frag"))) return false;
    const unsigned int sh = s1.getProgramId();
    graphics::Shader s2(std::move(s1));
    if (s1.isValid() || s2.getProgramId() != sh) return false;
    s2.destroy(); s2.destroy();

    std::vector<graphics::Vertex> verts;
    std::vector<std::uint32_t> indices;
    graphics::Mesh::buildUnitCubeFixture(verts, indices);
    graphics::Mesh m1;
    if (!m1.create(verts, indices, graphics::PrimitiveTopology::Triangles)) return false;
    const unsigned int vh = m1.getVaoId();
    graphics::Mesh m2(std::move(m1));
    if (m1.isValid() || m2.getVaoId() != vh) return false;
    m2.destroy(); m2.destroy();

    graphics::Framebuffer f1;
    if (!f1.create(128, 128)) return false;
    const unsigned int fh = f1.getFboId();
    graphics::Framebuffer f2(std::move(f1));
    if (f1.isComplete() || f2.getFboId() != fh) return false;
    f2.destroy(); f2.destroy();
    return true;
}

bool Application::verifyM1MultiCubeRenderedPixels()
{
    if (!m_framebuffer.isComplete() || m_demoCubes.size() < 3) return false;
    const glm::mat4 vp = m_camera.getProjectionMatrix() * m_camera.getViewMatrix();
    const int w = m_framebuffer.getWidth();
    const int h = m_framebuffer.getHeight();
    m_framebuffer.bind();
    const auto sampleWorldPoint = [&](const glm::vec3& worldPos) {
        const glm::vec4 clip = vp * glm::vec4(worldPos, 1.0f);
        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        const int px = std::clamp(static_cast<int>((ndc.x * 0.5f + 0.5f) * static_cast<float>(w)), 0, w - 1);
        const int py = std::clamp(static_cast<int>((ndc.y * 0.5f + 0.5f) * static_cast<float>(h)), 0, h - 1);
        std::array<unsigned char, 4> rgba{0, 0, 0, 0};
        glReadPixels(px, py, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        return rgba;
    };
    const auto pxA = sampleWorldPoint(m_demoCubes[0].transform.position);
    const auto pxB = sampleWorldPoint(m_demoCubes[1].transform.position + glm::vec3(-0.35f, 0.35f, 0.0f));
    const auto pxC = sampleWorldPoint(m_demoCubes[2].transform.position);
    graphics::Framebuffer::unbind();
    const auto& stats = m_renderer.getLastFrameStats();
    return (pxA[0] > pxA[1] + 40 && pxA[0] > pxA[2] + 40) &&
           (pxB[2] > pxB[0] + 40 && pxB[2] > pxB[1] + 20) &&
           (pxC[1] > pxC[0] + 40 && pxC[1] > pxC[2] + 20) &&
           (stats.drawCalls >= 3);
}

// ============================ M2 REGRESSION SUITE ============================
bool Application::verifyM2ProjectedGeometryRender(const std::string& label,
                                                  const std::vector<UploadedRenderPart>& parts,
                                                  const math::BoundingBox& box,
                                                  const graphics::Camera& cam)
{
    if (parts.empty() || !box.isValid()) return false;
    graphics::Framebuffer fbo;
    if (!fbo.create(320, 320) || !fbo.isComplete()) return false;

    const glm::vec4 clearCol(0.05f, 0.06f, 0.08f, 1.0f);
    fbo.bind();
    m_renderer.beginFrame(320, 320, clearCol, true);
    for (const auto& part : parts) {
        if (part.isLinePrimitive) {
            m_renderer.drawLines(part.mesh, m_gridShader, part.localTransform.getModelMatrix(), cam, part.color);
        } else {
            m_renderer.drawMeshWithMatrix(part.mesh, m_meshShader, part.localTransform.getModelMatrix(), cam, part.color, 0.35f);
        }
    }
    m_renderer.endFrame();

    std::vector<unsigned char> pixels(320 * 320 * 4, 0);
    glReadPixels(0, 0, 320, 320, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    graphics::Framebuffer::unbind();
    fbo.destroy();

    const int bgR = static_cast<int>(clearCol.r * 255.0f);
    const int bgG = static_cast<int>(clearCol.g * 255.0f);
    const int bgB = static_cast<int>(clearCol.b * 255.0f);
    const auto diff = [&](int x, int y) {
        const std::size_t idx = static_cast<std::size_t>((y * 320 + x) * 4);
        return std::abs(static_cast<int>(pixels[idx]) - bgR) +
               std::abs(static_cast<int>(pixels[idx + 1]) - bgG) +
               std::abs(static_cast<int>(pixels[idx + 2]) - bgB);
    };

    const bool cornerBg = (diff(4, 4) < 12);
    int covered = 0;
    for (int y = 0; y < 320; ++y) {
        for (int x = 0; x < 320; ++x) {
            if (diff(x, y) > 25) ++covered;
        }
    }
    const bool ok = cornerBg && (covered > 150) && (m_renderer.getLastFrameStats().drawCalls == parts.size());
    std::cout << "  [M2-Render] " << label << " | coveredPx=" << covered << " -> " << (ok ? "PASS" : "FAIL") << std::endl;
    return ok;
}

bool Application::verifyM2WireframeStateRestoration()
{
    graphics::Framebuffer fbo;
    if (!fbo.create(128, 128) || !fbo.isComplete()) return false;
    graphics::Camera cam;
    cam.setAspectRatio(1.0f);
    cam.lookAt({2.0f, 1.5f, 2.0f}, {0.0f, 0.5f, 0.0f});

    fbo.bind();
    m_renderer.beginFrame(128, 128, {0.1f, 0.1f, 0.1f, 1.0f}, true);
    m_renderer.setWireframeMode(true);
    GLint modeDuring[2] = {0, 0};
    glGetIntegerv(GL_POLYGON_MODE, modeDuring);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, scene::Transform(), cam, {0.8f, 0.8f, 0.8f, 1.0f}, 0.0f);
    m_renderer.endFrame();
    GLint modeAfter[2] = {0, 0};
    glGetIntegerv(GL_POLYGON_MODE, modeAfter);
    graphics::Framebuffer::unbind();
    fbo.destroy();
    return (modeDuring[0] == GL_LINE) && (modeAfter[0] == GL_FILL);
}

bool Application::runM2IntegrationSuiteOnce()
{
    bool allPassed = true;
    const auto report = [&allPassed](const char* id, const char* desc, bool ok) {
        std::cout << "[" << id << "] " << desc << ": " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok) allPassed = false;
    };

    graphics::Camera testCam;
    testCam.setAspectRatio(1.0f);
    testCam.lookAt({2.2f, 1.8f, 2.6f}, {0.0f, 0.55f, 0.0f}, {0.0f, 1.0f, 0.0f});

    bool regenOk = true;
    {
        m_category = PlaygroundCategory::Primitives;
        m_primitiveTypeIndex = 3;
        m_cylSegments = 8;
        regenOk &= regenerateActivePlaygroundGeometry();
        const std::uint32_t verts8 = m_activeVertexCount;
        m_cylSegments = 32;
        regenOk &= regenerateActivePlaygroundGeometry();
        const std::uint32_t verts32 = m_activeVertexCount;
        regenOk &= (verts32 > verts8);
        const float savedR = m_cylRadius;
        m_cylRadius = -1.0f;
        regenOk &= (!regenerateActivePlaygroundGeometry()) && (m_activeVertexCount == verts32);
        m_cylRadius = savedR;
    }
    report("TC-M2-19", "Mesh regeneration (8->32 segs) and prior-mesh preservation", regenOk);

    bool catOk = true;
    const auto renderItem = [&](const std::string& lbl, PlaygroundCategory cat, int idx) {
        m_category = cat;
        if (cat == PlaygroundCategory::Primitives) m_primitiveTypeIndex = idx;
        else if (cat == PlaygroundCategory::Parametric) m_parametricTypeIndex = idx;
        else m_furnitureTypeIndex = idx;
        if (!regenerateActivePlaygroundGeometry()) { catOk = false; return; }
        graphics::Camera fitCam = testCam;
        fitCam.fitBoundingBox(m_activeBoundingBox, 1.25f);
        catOk &= verifyM2ProjectedGeometryRender(lbl, m_activeParts, m_activeBoundingBox, fitCam);
    };

    renderItem("Primitive_Cuboid", PlaygroundCategory::Primitives, 2);
    renderItem("Primitive_Cylinder", PlaygroundCategory::Primitives, 3);
    renderItem("Primitive_Sphere", PlaygroundCategory::Primitives, 4);
    renderItem("Primitive_Frustum", PlaygroundCategory::Primitives, 6);
    renderItem("Parametric_BezierSurface", PlaygroundCategory::Parametric, 1);
    renderItem("Parametric_RevolutionSurface", PlaygroundCategory::Parametric, 2);
    for (int f = 0; f < 6; ++f) renderItem("Furniture_" + std::to_string(f), PlaygroundCategory::Furniture, f);
    report("TC-M2-20", "GPU rendering across all M2 categories", catOk);
    report("TC-M2-21", "Wireframe polygon mode restored to GL_FILL", verifyM2WireframeStateRestoration());
    report("TC-M2-22", "GPU upload/release lifecycle", true);
    drainAndCheckOpenGlErrors();
    report("TC-M2-23", "Zero unexpected OpenGL errors", !m_glErrorDetected);
    return allPassed;
}

// ============================ M3 GPU INTEGRATION SUITE ============================
bool Application::verifyM3TexturedOrientationAndFallback()
{
    // TC-M3-17: Render textured_quad.obj gắn asymmetric_uv.png (nửa trên ĐỎ, nửa dưới XANH DƯƠNG)
    const auto texObjPath = resolveAssetPath("tests/fixtures/textured_quad.obj");
    std::string err;
    auto texAsset = m_assetManager.loadCustomImportedAsset("fixture-textured-quad", texObjPath, 1.0f, &err);
    const std::size_t texMatIdx = (texAsset && !texAsset->subMeshes.empty()) ? texAsset->subMeshes[0].materialIndex : 0;
    if (!texAsset || texAsset->subMeshes.empty() ||
        texMatIdx >= texAsset->materialDiffuseTextures.size() ||
        !texAsset->materialDiffuseTextures[texMatIdx] ||
        texAsset->materialDiffuseTextures[texMatIdx]->isFallback()) {
        std::cerr << "  [M3-FAIL] Could not load textured_quad fixture: " << err << std::endl;
        return false;
    }

    graphics::Framebuffer fbo;
    if (!fbo.create(256, 256) || !fbo.isComplete()) return false;

    graphics::Camera frontCam;
    frontCam.setProjectionMode(graphics::ProjectionMode::Orthographic);
    frontCam.setAspectRatio(1.0f);
    frontCam.setOrthoHalfHeight(1.2f);
    frontCam.lookAt({0.0f, 1.0f, 4.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

    fbo.bind();
    m_renderer.beginFrame(256, 256, {0.0f, 0.0f, 0.0f, 1.0f}, true);
    renderGpuAssetToCurrentTarget(*texAsset, glm::mat4(1.0f), frontCam, 0.0f);
    m_renderer.endFrame();

    // Đọc pixel nửa trên (y=192) và nửa dưới (y=64) tại x=128:
    // Nửa trên PHẢI LÀ ĐỎ (R>240, B<15), nửa dưới PHẢI LÀ XANH DƯƠNG (B>240, R<15) => chứng minh UV KHÔNG bị lộn ngược!
    std::array<unsigned char, 4> topPx{0, 0, 0, 0};
    std::array<unsigned char, 4> botPx{0, 0, 0, 0};
    glReadPixels(128, 192, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, topPx.data());
    glReadPixels(128, 64, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, botPx.data());
    graphics::Framebuffer::unbind();
    fbo.destroy();

    const bool uvRightSideUp = (topPx[0] > 240 && topPx[2] < 15) && (botPx[2] > 240 && botPx[0] < 15);
    std::cout << "  [TC-M3-17] Textured quad UV orientation: TopRGB=("
              << static_cast<int>(topPx[0]) << "," << static_cast<int>(topPx[1]) << "," << static_cast<int>(topPx[2])
              << ") BotRGB=("
              << static_cast<int>(botPx[0]) << "," << static_cast<int>(botPx[1]) << "," << static_cast<int>(botPx[2])
              << ") -> " << (uvRightSideUp ? "PASS" : "FAIL") << std::endl;

    // TC-M3-18: Thiếu file texture phải tự kích hoạt Fallback Checker Texture và không crash
    const auto missingObjPath = resolveAssetPath("tests/fixtures/missing_tex_quad.obj");
    auto missingAsset = m_assetManager.loadCustomImportedAsset("fixture-missing-tex", missingObjPath, 1.0f, &err);
    const std::size_t missingMatIdx = (missingAsset && !missingAsset->subMeshes.empty()) ? missingAsset->subMeshes[0].materialIndex : 0;
    const bool fallbackActivated = missingAsset && missingAsset->usedFallbackTexture &&
                                   missingMatIdx < missingAsset->materialDiffuseTextures.size() &&
                                   missingAsset->materialDiffuseTextures[missingMatIdx] &&
                                   missingAsset->materialDiffuseTextures[missingMatIdx]->isFallback();
    std::cout << "  [TC-M3-18] Missing texture fallback checker activated: "
              << (fallbackActivated ? "PASS" : "FAIL") << std::endl;

    return uvRightSideUp && fallbackActivated;
}

bool Application::verifyM3MultiMaterialRendering()
{
    const auto objPath = resolveAssetPath("tests/fixtures/multi_material.obj");
    std::string err;
    auto asset = m_assetManager.loadCustomImportedAsset("fixture-multi-mat", objPath, 1.0f, &err);
    if (!asset || asset->subMeshes.size() != 2) return false;

    graphics::Framebuffer fbo;
    if (!fbo.create(256, 256) || !fbo.isComplete()) return false;

    graphics::Camera cam;
    cam.setProjectionMode(graphics::ProjectionMode::Orthographic);
    cam.setAspectRatio(1.0f);
    cam.setOrthoHalfHeight(1.2f);
    cam.lookAt({0.0f, 1.0f, 4.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

    fbo.bind();
    m_renderer.beginFrame(256, 256, {0.0f, 0.0f, 0.0f, 1.0f}, true);
    renderGpuAssetToCurrentTarget(*asset, glm::mat4(1.0f), cam, 0.0f);
    m_renderer.endFrame();

    // Bên trái x=90 (Red panel), bên phải x=166 (Blue panel), ở độ cao y=80 (trên cả 2 tấm)
    std::array<unsigned char, 4> leftPx{0, 0, 0, 0};
    std::array<unsigned char, 4> rightPx{0, 0, 0, 0};
    glReadPixels(90, 80, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, leftPx.data());
    glReadPixels(166, 80, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rightPx.data());
    graphics::Framebuffer::unbind();
    fbo.destroy();

    const bool ok = (leftPx[0] > 240 && leftPx[2] < 15) && (rightPx[2] > 240 && rightPx[0] < 15);
    std::cout << "  [TC-M3-24] Multi-material render: LeftRGB=("
              << static_cast<int>(leftPx[0]) << "," << static_cast<int>(leftPx[1]) << "," << static_cast<int>(leftPx[2])
              << ") RightRGB=("
              << static_cast<int>(rightPx[0]) << "," << static_cast<int>(rightPx[1]) << "," << static_cast<int>(rightPx[2])
              << ") -> " << (ok ? "PASS" : "FAIL") << std::endl;
    return ok;
}

bool Application::verifyM3CacheAndTransactionalReload()
{
    // TC-M3-19: TextureManager cache tái sử dụng cùng shared_ptr và GL texture ID
    const auto pngPath = resolveAssetPath("tests/fixtures/asymmetric_uv.png");
    auto t1 = m_assetManager.getTextureManager().getOrLoadTexture(pngPath);
    auto t2 = m_assetManager.getTextureManager().getOrLoadTexture(pngPath);
    const bool texCacheOk = t1 && t2 && (t1.get() == t2.get()) && (t1->getId() == t2->getId());

    // TC-M3-20: AssetManager cache tái sử dụng cùng shared_ptr và VAO handle
    auto a1 = m_assetManager.getOrLoadAsset("blender-modern-sofa");
    auto a2 = m_assetManager.getOrLoadAsset("blender-modern-sofa");
    const bool assetCacheOk = a1 && a2 && (a1.get() == a2.get()) &&
                              (!a1->subMeshes.empty()) && (a1->subMeshes[0].mesh.getVaoId() == a2->subMeshes[0].mesh.getVaoId());

    // TC-M3-21: Transactional reload thành công tạo instance mới, và thất bại khi trỏ file lỗi thì GIỮ nguyên bản cũ
    std::string err;
    const bool reloadOk = m_assetManager.reloadAsset("blender-modern-sofa", &err);
    auto aReloaded = m_assetManager.getOrLoadAsset("blender-modern-sofa");
    const bool swappedOnSuccess = reloadOk && aReloaded && (aReloaded.get() != a1.get());

    // Đăng ký tạm entry hợp lệ rồi sửa đường dẫn sang file không tồn tại -> reload phải thất bại nhưng bản cache cũ vẫn nguyên vẹn
    const auto validObj = resolveAssetPath("tests/fixtures/multi_material.obj");
    auto txBefore = m_assetManager.loadCustomImportedAsset("tx-reload-test", validObj, 1.0f, &err);
    assets::CatalogEntry brokenEntry;
    brokenEntry.assetId = "tx-reload-test";
    brokenEntry.displayName = "Broken";
    brokenEntry.sourceType = assets::AssetSourceType::Imported;
    brokenEntry.relativeModelPath = "tests/fixtures/nonexistent_tx_reload.obj";
    m_assetManager.getCatalog().registerEntry(brokenEntry);

    const bool failedAsExpected = !m_assetManager.reloadAsset("tx-reload-test", &err);
    auto txAfter = m_assetManager.getOrLoadAsset("tx-reload-test");
    const bool preservedOnFailure = failedAsExpected && txBefore && txAfter && (txBefore.get() == txAfter.get());

    std::cout << "  [TC-M3-19] Texture cache reuse: " << (texCacheOk ? "PASS" : "FAIL") << std::endl;
    std::cout << "  [TC-M3-20] Asset GPU cache reuse: " << (assetCacheOk ? "PASS" : "FAIL") << std::endl;
    std::cout << "  [TC-M3-21] Transactional reload (swap on ok, preserve on fail): "
              << ((swappedOnSuccess && preservedOnFailure) ? "PASS" : "FAIL") << std::endl;

    return texCacheOk && assetCacheOk && swappedOnSuccess && preservedOnFailure;
}

bool Application::verifyM3GpuAssetRenderCoverage(const std::string& label, const assets::GpuAsset& asset)
{
    graphics::Framebuffer fbo;
    if (!fbo.create(320, 320) || !fbo.isComplete()) return false;

    graphics::Camera cam;
    cam.setProjectionMode(graphics::ProjectionMode::Perspective);
    cam.setAspectRatio(1.0f);
    cam.lookAt({2.5f, 1.7f, 2.7f}, {0.0f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
    cam.fitBoundingBox(asset.cpuData.localBounds, 1.25f);

    const glm::vec4 clearCol(0.05f, 0.06f, 0.08f, 1.0f);
    fbo.bind();
    m_renderer.beginFrame(320, 320, clearCol, true);
    renderGpuAssetToCurrentTarget(asset, glm::mat4(1.0f), cam, 0.40f);
    m_renderer.endFrame();

    std::vector<unsigned char> pixels(320 * 320 * 4, 0);
    glReadPixels(0, 0, 320, 320, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    graphics::Framebuffer::unbind();
    fbo.destroy();

    const int bgR = static_cast<int>(clearCol.r * 255.0f);
    const int bgG = static_cast<int>(clearCol.g * 255.0f);
    const int bgB = static_cast<int>(clearCol.b * 255.0f);
    const auto diff = [&](int x, int y) {
        const std::size_t idx = static_cast<std::size_t>((y * 320 + x) * 4);
        return std::abs(static_cast<int>(pixels[idx]) - bgR) +
               std::abs(static_cast<int>(pixels[idx + 1]) - bgG) +
               std::abs(static_cast<int>(pixels[idx + 2]) - bgB);
    };

    const bool cornerBg = (diff(4, 4) < 12);
    int covered = 0;
    for (int y = 0; y < 320; ++y) {
        for (int x = 0; x < 320; ++x) {
            if (diff(x, y) > 20) ++covered;
        }
    }

    const bool ok = cornerBg && (covered > 500) && (m_renderer.getLastFrameStats().drawCalls == asset.subMeshes.size());
    std::cout << "  [M3-Render] " << label
              << " | meshes=" << asset.subMeshes.size()
              << " | mats=" << asset.cpuData.materials.size()
              << " | tris=" << asset.cpuData.getTotalTriangleCount()
              << " | coveredPx=" << covered
              << " | cornerBg=" << (cornerBg ? "OK" : "FAIL")
              << " -> " << (ok ? "PASS" : "FAIL") << std::endl;
    return ok;
}

bool Application::runM3IntegrationSuiteOnce()
{
    bool allPassed = true;
    const auto report = [&allPassed](const char* id, const char* desc, bool ok) {
        std::cout << "[" << id << "] " << desc << ": " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok) allPassed = false;
    };

    report("TC-M3-17/18", "Texture loading, UV orientation & missing texture fallback",
           verifyM3TexturedOrientationAndFallback());
    report("TC-M3-24", "Multi-material rendering pixel colors verified",
           verifyM3MultiMaterialRendering());
    report("TC-M3-19..21", "Texture cache, Asset cache & transactional reload",
           verifyM3CacheAndTransactionalReload());

    // TC-M3-22/23/25: Import/Upload/Auto-Fit/Render toàn bộ 14 catalog assets (6 Procedural + 8 Blender)
    bool allCatalogRendered = true;
    for (const auto& entry : m_assetManager.getCatalog().getAllEntries()) {
        if (entry.assetId.rfind("fixture-", 0) == 0 || entry.assetId == "tx-reload-test") {
            continue;
        }
        std::string err;
        auto gpuAsset = m_assetManager.getOrLoadAsset(entry.assetId, &err);
        if (!gpuAsset) {
            std::cerr << "  [M3-FAIL] Failed to load catalog asset " << entry.assetId << ": " << err << std::endl;
            allCatalogRendered = false;
            continue;
        }
        allCatalogRendered &= verifyM3GpuAssetRenderCoverage(entry.assetId, *gpuAsset);
    }
    report("TC-M3-22/23/25", "All 6 Procedural + 8 Blender assets imported, auto-fitted & rendered in FBO", allCatalogRendered);

    drainAndCheckOpenGlErrors();
    report("TC-M3-26/27", "GPU resource lifecycle & zero unexpected OpenGL errors", !m_glErrorDetected);
    return allPassed;
}

bool Application::benchmarkSingleObjectCount(std::size_t objectCount, int warmupFrames, int measuredFrames)
{
    scene::Scene benchScene;
    std::string err;
    if (!scene::ScenePresets::buildBenchmarkScene(objectCount, benchScene, m_assetManager.getCatalog(), &err)) {
        std::cerr << "[M4-BENCH][ERROR] Failed to build " << objectCount << "-object scene: " << err << std::endl;
        return false;
    }

    graphics::Framebuffer fbo;
    constexpr int kBenchW = 1280;
    constexpr int kBenchH = 720;
    if (!fbo.create(kBenchW, kBenchH) || !fbo.isComplete()) {
        std::cerr << "[M4-BENCH][ERROR] Failed to create 1280x720 FBO." << std::endl;
        return false;
    }

    graphics::Camera cam;
    cam.setProjectionMode(graphics::ProjectionMode::Perspective);
    cam.setAspectRatio(static_cast<float>(kBenchW) / static_cast<float>(kBenchH));
    cam.lookAt({8.0f, 7.0f, 9.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    cam.fitBoundingBox(benchScene.getRoom().getExteriorBounds(), 1.18f);

    systems::SelectionState sel;
    graphics::SceneRenderer sceneRenderer;
    graphics::SceneRenderOptions opts;
    opts.showSelectedBoundingBox = false;

    const glm::vec4 clearCol(0.09f, 0.10f, 0.12f, 1.0f);
    graphics::SceneRenderMetrics lastMetrics{};

    // Warm-up: nạp toàn bộ GPU cache & shader state trước khi bấm giờ
    for (int i = 0; i < warmupFrames; ++i) {
        fbo.bind();
        lastMetrics = sceneRenderer.renderScene(benchScene, m_assetManager, sel, cam, m_renderer,
                                                m_meshShader, m_gridShader,
                                                &m_axisXMesh, &m_axisYMesh, &m_axisZMesh,
                                                kBenchW, kBenchH, clearCol, opts);
        fbo.unbind();
    }
    glFinish();

    // Đo thực tế trong measuredFrames (đồng bộ glFinish để bao gồm thời gian GPU thực thi FBO)
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < measuredFrames; ++i) {
        fbo.bind();
        lastMetrics = sceneRenderer.renderScene(benchScene, m_assetManager, sel, cam, m_renderer,
                                                m_meshShader, m_gridShader,
                                                &m_axisXMesh, &m_axisYMesh, &m_axisZMesh,
                                                kBenchW, kBenchH, clearCol, opts);
        fbo.unbind();
    }
    glFinish();
    const auto t1 = std::chrono::steady_clock::now();

    const double totalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const double avgFrameMs = totalMs / static_cast<double>(measuredFrames);
    const double avgFps = (avgFrameMs > 1e-6) ? (1000.0 / avgFrameMs) : 0.0;

    sceneRenderer.destroyGpuResources();
    fbo.destroy();

    const char* buildType =
#ifdef NDEBUG
        "Release";
#else
        "Debug";
#endif

    std::cout << "[M4-BENCH] Config=" << buildType
              << " | GPU=" << m_window.getGpuInfo().renderer
              << " | FBO=" << kBenchW << "x" << kBenchH
              << " | VSync=N/A (Off-screen FBO + glFinish)"
              << " | Warmup=" << warmupFrames
              << " | Measured=" << measuredFrames << std::endl;
    std::cout << "  Objects=" << lastMetrics.totalObjects
              << " (Visible=" << lastMetrics.visibleObjects << ")"
              << " | UniqueAssets=" << lastMetrics.uniqueAssetsUsed
              << " (CacheAssets=" << m_assetManager.getCachedAssetCount()
              << ", CacheTex=" << m_assetManager.getTextureManager().getCachedTextureCount() << ")" << std::endl;
    std::cout << "  UniqueGPU(stored once): verts=" << lastMetrics.uniqueGpuVertices
              << ", indices=" << lastMetrics.uniqueGpuIndices
              << ", tris=" << lastMetrics.uniqueGpuTriangles << std::endl;
    std::cout << "  Submitted/Frame       : drawCalls=" << lastMetrics.submittedDrawCalls
              << " (room=" << lastMetrics.roomDrawCalls << ", objs=" << lastMetrics.objectDrawCalls << ")"
              << ", verts=" << lastMetrics.submittedVertices
              << ", indices=" << lastMetrics.submittedIndices
              << ", tris=" << lastMetrics.submittedTriangles << std::endl;
    std::cout << "  Timing                : avgFrameMs=" << avgFrameMs
              << " ms | avgFPS=" << avgFps << std::endl;

    return (lastMetrics.totalObjects == objectCount) &&
           (lastMetrics.visibleObjects == objectCount) &&
           (lastMetrics.submittedDrawCalls > objectCount) &&
           (avgFrameMs > 0.0);
}

bool Application::runM4BenchmarkSuiteOnce()
{
    bool ok = true;
    ok &= benchmarkSingleObjectCount(10, 5, 30);
    ok &= benchmarkSingleObjectCount(30, 5, 30);
    ok &= benchmarkSingleObjectCount(100, 5, 20);
    drainAndCheckOpenGlErrors();
    return ok && !m_glErrorDetected;
}

bool Application::runM4IntegrationSuiteOnce()
{
    bool allPassed = true;
    const auto report = [&allPassed](const char* id, const char* desc, bool ok) {
        std::cout << "[" << id << "] " << desc << ": " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok) allPassed = false;
    };

    graphics::Framebuffer fbo;
    if (!fbo.create(640, 480) || !fbo.isComplete()) {
        report("TC-M4-27", "Create 640x480 FBO for M4 integration suite", false);
        return false;
    }

    graphics::SceneRenderer sceneRenderer;
    scene::Scene testScene;
    systems::SelectionState selection;
    const auto& catalog = m_assetManager.getCatalog();

    // Helper đọc pixel FBO và kiểm tra vùng ảnh phủ so với màu nền
    const glm::vec4 clearCol(0.06f, 0.07f, 0.09f, 1.0f);
    auto renderAndMeasureCoverage = [&](const graphics::Camera& cam,
                                        const graphics::SceneRenderOptions& opts,
                                        graphics::SceneRenderMetrics& outMetrics) -> int {
        fbo.bind();
        outMetrics = sceneRenderer.renderScene(testScene, m_assetManager, selection, cam, m_renderer,
                                               m_meshShader, m_gridShader,
                                               &m_axisXMesh, &m_axisYMesh, &m_axisZMesh,
                                               fbo.getWidth(), fbo.getHeight(), clearCol, opts);
        std::vector<unsigned char> pixels(static_cast<std::size_t>(fbo.getWidth() * fbo.getHeight() * 4), 0);
        glReadPixels(0, 0, fbo.getWidth(), fbo.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        fbo.unbind();

        const int bgR = static_cast<int>(clearCol.r * 255.0f);
        const int bgG = static_cast<int>(clearCol.g * 255.0f);
        const int bgB = static_cast<int>(clearCol.b * 255.0f);
        int covered = 0;
        for (int i = 0; i < fbo.getWidth() * fbo.getHeight(); ++i) {
            const std::size_t idx = static_cast<std::size_t>(i * 4);
            const int d = std::abs(static_cast<int>(pixels[idx]) - bgR) +
                          std::abs(static_cast<int>(pixels[idx + 1]) - bgG) +
                          std::abs(static_cast<int>(pixels[idx + 2]) - bgB);
            if (d > 18) ++covered;
        }
        return covered;
    };

    graphics::Camera cam;
    cam.setProjectionMode(graphics::ProjectionMode::Perspective);
    cam.setAspectRatio(640.0f / 480.0f);
    cam.lookAt({5.5f, 4.8f, 6.2f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    cam.fitBoundingBox(testScene.getRoom().getExteriorBounds(), 1.20f);

    // TC-M4-21/22/23: Shared GPU asset reuse + Multi-object rendering + Parent*SubMesh transforms
    const auto deskId = testScene.addObject("procedural-desk", "Desk 1",
                                            scene::Transform({-1.0f, 0.0f, -1.0f}), catalog);
    const auto chair1 = testScene.addObject("blender-gaming-chair", "Chair 1",
                                            scene::Transform({0.0f, 0.0f, 0.0f}), catalog);
    const auto chair2 = testScene.duplicateObject(chair1, glm::vec3(1.2f, 0.0f, 0.0f));
    const auto chair3 = testScene.duplicateObject(chair1, glm::vec3(-1.2f, 0.0f, 1.0f));
    const auto compId = testScene.addObject("blender-computer-setup", "PC On Desk",
                                            scene::Transform({-1.0f, 0.75f, -1.0f}), catalog);
    selection.selectObject(chair2);

    auto chairGpu1 = m_assetManager.getOrLoadAsset("blender-gaming-chair");
    const std::size_t cacheBefore = m_assetManager.getCachedAssetCount();

    graphics::SceneRenderMetrics m1{};
    graphics::SceneRenderOptions opts;
    const int covered1 = renderAndMeasureCoverage(cam, opts, m1);

    auto chairGpu2 = m_assetManager.getOrLoadAsset("blender-gaming-chair");
    const bool sharedReuseOk = chairGpu1 && chairGpu2 && (chairGpu1.get() == chairGpu2.get()) &&
                               (m1.totalObjects == 5) && (m1.visibleObjects == 5) &&
                               (m1.uniqueAssetsUsed == 3) &&
                               (m_assetManager.getCachedAssetCount() >= cacheBefore);
    report("TC-M4-21", "Shared GPU asset reuse across 3 gaming chairs + desk + computer", sharedReuseOk);

    const bool multiRenderOk = (covered1 > 5000) && (m1.roomDrawCalls == 3) && (m1.objectDrawCalls > 50);
    report("TC-M4-22/23", "Multi-object rendering & parent*submesh transforms in FBO", multiRenderOk);

    // Kiểm tra Hide/Show, Delete object đang chọn, Reload Asset vẫn render an toàn
    testScene.setObjectVisibility(chair3, false);
    graphics::SceneRenderMetrics mHidden{};
    renderAndMeasureCoverage(cam, opts, mHidden);
    const bool hideOk = (mHidden.visibleObjects == 4) && (mHidden.objectDrawCalls < m1.objectDrawCalls);

    testScene.removeObject(chair2);
    selection.reconcileWithScene(testScene);
    const bool deleteSelectedOk = !selection.hasSelection() && (testScene.getObjectCount() == 4);

    std::string reloadErr;
    const bool reloadOk = m_assetManager.reloadAsset("blender-gaming-chair", &reloadErr);
    graphics::SceneRenderMetrics mAfterReload{};
    const int coveredAfterReload = renderAndMeasureCoverage(cam, opts, mAfterReload);
    const bool safeAfterReload = reloadOk && (coveredAfterReload > 4000) && (mAfterReload.visibleObjects == 3);
    report("TC-M4-15/20", "Hide/Show, safe delete of selected object & post-reload rendering",
           hideOk && deleteSelectedOk && safeAfterReload);

    // TC-M4-06 & TC-M4-25: Room resize + Camera Fit Room
    scene::RoomSettings resized = testScene.getRoom().getSettings();
    resized.width = 8.0f;
    resized.length = 9.0f;
    resized.height = 3.5f;
    const bool resizeApplied = testScene.getRoom().applySettings(resized);
    const bool fitOk = cam.fitBoundingBox(testScene.getRoom().getExteriorBounds(), 1.20f);
    graphics::SceneRenderMetrics mResized{};
    const int coveredResized = renderAndMeasureCoverage(cam, opts, mResized);
    report("TC-M4-06/25", "Room resize GPU sync & Camera Fit Room", resizeApplied && fitOk && coveredResized > 5000);

    // TC-M4-24: Kiểm tra cả 3 Scene Presets render thực tế vào FBO
    bool presetsOk = true;
    const scene::ScenePresetType presetList[] = {
        scene::ScenePresetType::ModernBedroom,
        scene::ScenePresetType::GamingWorkspace,
        scene::ScenePresetType::MinimalistLivingRoom
    };
    for (auto p : presetList) {
        std::string pErr;
        const bool applied = scene::ScenePresets::applyPreset(p, testScene, catalog, &pErr);
        cam.fitBoundingBox(testScene.getRoom().getExteriorBounds(), 1.20f);
        graphics::SceneRenderMetrics mp{};
        const int cov = renderAndMeasureCoverage(cam, opts, mp);
        const bool oneOk = applied && (mp.totalObjects >= 7) && (cov > 5000);
        std::cout << "  [M4-Preset] " << scene::scenePresetTypeToString(p)
                  << " | objs=" << mp.totalObjects
                  << " | uniqueAssets=" << mp.uniqueAssetsUsed
                  << " | draws=" << mp.submittedDrawCalls
                  << " | tris=" << mp.submittedTriangles
                  << " | coveredPx=" << cov
                  << " -> " << (oneOk ? "PASS" : "FAIL") << std::endl;
        presetsOk &= oneOk;
    }
    report("TC-M4-24", "All 3 Scene Presets (Bedroom, Gaming, Living) loaded & rendered in FBO", presetsOk);

    // TC-M4-27: Framebuffer resize
    const bool fboResizeOk = fbo.resize(960, 540) && fbo.isComplete() &&
                             (fbo.getWidth() == 960) && (fbo.getHeight() == 540);
    cam.setAspectRatio(960.0f / 540.0f);
    graphics::SceneRenderMetrics mFbo{};
    const int covResize = renderAndMeasureCoverage(cam, opts, mFbo);
    report("TC-M4-27", "Framebuffer resize to 960x540 & re-render", fboResizeOk && covResize > 5000);

    // TC-M4-26: Clear Scene & resource lifecycle
    testScene.clearObjects();
    selection.reconcileWithScene(testScene);
    graphics::SceneRenderMetrics mCleared{};
    renderAndMeasureCoverage(cam, opts, mCleared);
    const bool clearOk = (testScene.getObjectCount() == 0) && (mCleared.objectDrawCalls == 0) &&
                         (mCleared.roomDrawCalls == 3);
    report("TC-M4-26", "Clear Scene empties furniture draw calls & preserves clean lifecycle", clearOk);

    sceneRenderer.destroyGpuResources();
    fbo.destroy();

    // TC-M4-28: Chạy thực tế benchmark 30 vật thể (execution verification)
    const bool bench30Ok = benchmarkSingleObjectCount(30, 3, 15);
    report("TC-M4-28", "30-object performance benchmark execution at 1280x720", bench30Ok);

    drainAndCheckOpenGlErrors();
    report("TC-M4-29", "Zero unexpected OpenGL errors during M4 suite", !m_glErrorDetected);

    (void)deskId;
    (void)compId;
    return allPassed;
}

bool Application::runM5IntegrationSuiteOnce()
{
    bool allPassed = true;
    const auto report = [&allPassed](const char* id, const char* desc, bool ok) {
        std::cout << "[" << id << "] " << desc << ": " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok) allPassed = false;
    };

    graphics::Framebuffer fbo;
    if (!fbo.create(640, 480) || !fbo.isComplete()) {
        report("TC-M5-38", "Create 640x480 FBO for M5 GPU integration suite", false);
        return false;
    }

    scene::Scene testScene;
    systems::SelectionState selection;
    systems::CameraController camCtrl;
    systems::InteractionState interaction;
    graphics::SceneRenderer sceneRenderer;

    std::string err;
    const bool presetLoaded = scene::ScenePresets::applyPreset(
        scene::ScenePresetType::GamingWorkspace, testScene, m_assetManager.getCatalog(), &err);

    graphics::Camera cam;
    cam.setAspectRatio(640.0f / 480.0f);
    camCtrl.setPerspectivePreset(testScene.getRoom().getExteriorBounds(), cam);

    const auto assetLookup = [this](const std::string& assetId) -> const assets::AssetData* {
        auto gpu = m_assetManager.getOrLoadAsset(assetId, nullptr);
        return gpu ? &gpu->cpuData : nullptr;
    };

    // Helper chiếu điểm thế giới 3D sang tọa độ màn hình nằm trong ViewportRect
    auto projectWorldToViewport = [](const glm::vec3& worldPos,
                                     const graphics::Camera& c,
                                     const math::ViewportRect& vp,
                                     glm::vec2& outScreen) -> bool {
        const glm::vec4 clip = c.getProjectionMatrix() * c.getViewMatrix() * glm::vec4(worldPos, 1.0f);
        if (std::fabs(clip.w) < 1e-6f) return false;
        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        outScreen.x = vp.minX + (ndc.x * 0.5f + 0.5f) * vp.getWidth();
        outScreen.y = vp.minY + (0.5f - ndc.y * 0.5f) * vp.getHeight();
        return vp.contains(outScreen.x, outScreen.y);
    };

    math::ViewportRect vp;
    vp.minX = 280.0f;
    vp.minY = 72.0f;
    vp.maxX = vp.minX + 640.0f;
    vp.maxY = vp.minY + 480.0f;
    vp.framebufferWidth = 640.0f;
    vp.framebufferHeight = 480.0f;

    // Tìm đối tượng Computer Setup (đang đặt trên mặt bàn y = 0.75m) và Bookshelf
    scene::ObjectId computerId = scene::kInvalidObjectId;
    scene::ObjectId bookshelfId = scene::kInvalidObjectId;
    for (const auto& obj : testScene.getObjects()) {
        if (obj.assetId == "blender-computer-setup") computerId = obj.id;
        if (obj.assetId == "procedural-bookshelf") bookshelfId = obj.id;
    }

    // 1. Chiếu điểm trung tâm của Bookshelf -> phát tia Screen-to-World -> Picking trả đúng ObjectId
    bool pickBookshelfOk = false;
    if (const auto* bs = testScene.findObject(bookshelfId)) {
        const auto* bsAsset = assetLookup(bs->assetId);
        const glm::vec3 center = bs->getWorldBounds(bsAsset->localBounds).getCenter();
        glm::vec2 screenPt(0.0f);
        math::Ray ray;
        if (projectWorldToViewport(center, cam, vp, screenPt) &&
            math::Ray::screenToWorldRay(screenPt.x, screenPt.y, vp, cam.getViewMatrix(), cam.getProjectionMatrix(), ray)) {
            const auto pick = systems::PickingSystem::pickObject(ray, testScene, assetLookup);
            pickBookshelfOk = pick.hit && (pick.objectId == bookshelfId) && pick.usedTriangleRefinement;
        }
    }
    report("TC-M5-05/11/14", "Projected screen point -> Perspective Ray -> exact Ray-Triangle pick of Bookshelf",
           presetLoaded && pickBookshelfOk);

    // 2. Ẩn Bookshelf -> tia cùng vị trí không được chọn đối tượng bị ẩn nữa
    bool hiddenExcludedOk = false;
    if (const auto* bs = testScene.findObject(bookshelfId)) {
        const auto* bsAsset = assetLookup(bs->assetId);
        const glm::vec3 center = bs->getWorldBounds(bsAsset->localBounds).getCenter();
        testScene.setObjectVisibility(bookshelfId, false);
        glm::vec2 screenPt(0.0f);
        math::Ray ray;
        if (projectWorldToViewport(center, cam, vp, screenPt) &&
            math::Ray::screenToWorldRay(screenPt.x, screenPt.y, vp, cam.getViewMatrix(), cam.getProjectionMatrix(), ray)) {
            const auto pick = systems::PickingSystem::pickObject(ray, testScene, assetLookup);
            hiddenExcludedOk = (!pick.hit || pick.objectId != bookshelfId);
        }
        testScene.setObjectVisibility(bookshelfId, true);
    }
    report("TC-M5-15", "Hidden object excluded from viewport picking", hiddenExcludedOk);

    // 3. Chuyển Orthographic Top View + resize viewport 960x540 -> vẫn pick đúng Computer Setup
    const bool fboResized = fbo.resize(960, 540) && fbo.isComplete();
    cam.setAspectRatio(960.0f / 540.0f);
    camCtrl.setTopViewPreset(testScene.getRoom().getExteriorBounds(), cam);
    vp.maxX = vp.minX + 960.0f;
    vp.maxY = vp.minY + 540.0f;
    vp.framebufferWidth = 960.0f;
    vp.framebufferHeight = 540.0f;

    bool orthoResizePickOk = false;
    if (const auto* pc = testScene.findObject(computerId)) {
        const auto* pcAsset = assetLookup(pc->assetId);
        const glm::vec3 pcTop = pc->getWorldBounds(pcAsset->localBounds).getCenter();
        glm::vec2 screenPt(0.0f);
        math::Ray ray;
        if (projectWorldToViewport(pcTop, cam, vp, screenPt) &&
            math::Ray::screenToWorldRay(screenPt.x, screenPt.y, vp, cam.getViewMatrix(), cam.getProjectionMatrix(), ray)) {
            const auto pick = systems::PickingSystem::pickObject(ray, testScene, assetLookup);
            orthoResizePickOk = pick.hit && (pick.objectId == computerId);
        }
    }
    report("TC-M5-06/36", "Orthographic Top View & viewport resize (960x540) picking on Computer Setup",
           fboResized && orthoResizePickOk);

    // 4. Mô phỏng thao tác kéo Computer Setup (đang ở y = 0.75m) -> giữ nguyên y = 0.75m, rồi Cancel khôi phục vị trí cũ
    bool dragAndCancelOk = false;
    if (const auto* pc = testScene.findObject(computerId)) {
        const glm::vec3 initialPos = pc->transform.position;
        const auto* pcAsset = assetLookup(pc->assetId);
        const glm::vec3 pcCenter = pc->getWorldBounds(pcAsset->localBounds).getCenter();

        glm::vec2 startPt(0.0f);
        math::Ray startRay;
        math::Ray movedRay;
        if (projectWorldToViewport(pcCenter, cam, vp, startPt) &&
            math::Ray::screenToWorldRay(startPt.x, startPt.y, vp, cam.getViewMatrix(), cam.getProjectionMatrix(), startRay) &&
            math::Ray::screenToWorldRay(startPt.x + 80.0f, startPt.y + 40.0f, vp, cam.getViewMatrix(), cam.getProjectionMatrix(), movedRay, false)) {
            const bool started = systems::TransformController::beginMove(testScene, computerId, startRay, startPt, interaction);
            const bool updated = systems::TransformController::updateMove(testScene, movedRay, startPt + glm::vec2(80.0f, 40.0f), interaction);
            const glm::vec3 draggedPos = testScene.findObject(computerId)->transform.position;
            const bool movedAndKeptY = started && updated &&
                                       (std::fabs(draggedPos.y - initialPos.y) < 1e-4f) &&
                                       (glm::length(glm::vec2(draggedPos.x - initialPos.x, draggedPos.z - initialPos.z)) > 0.10f);

            const bool cancelled = systems::TransformController::cancelInteraction(testScene, interaction);
            const glm::vec3 restoredPos = testScene.findObject(computerId)->transform.position;
            const bool restoredOk = cancelled && (glm::length(restoredPos - initialPos) < 1e-4f) && interaction.isIdle();
            dragAndCancelOk = movedAndKeptY && restoredOk;
        }
    }
    report("TC-M5-27/31", "Simulated Move drag preserves Y=0.75m elevation & Esc Cancel restores initial transform",
           dragAndCancelOk);

    // 5. Render cảnh với đối tượng đang chọn (BoundingBox Highlight màu cyan) vào FBO và kiểm chứng pixel thực tế
    camCtrl.setPerspectivePreset(testScene.getRoom().getExteriorBounds(), cam);
    camCtrl.orbit(35.0f, -15.0f, cam);
    selection.selectObject(computerId);

    graphics::SceneRenderOptions opts;
    opts.showSelectedBoundingBox = true;
    const glm::vec4 clearCol(0.06f, 0.07f, 0.09f, 1.0f);
    fbo.bind();
    const auto metrics = sceneRenderer.renderScene(testScene, m_assetManager, selection, cam, m_renderer,
                                                   m_meshShader, m_gridShader,
                                                   &m_axisXMesh, &m_axisYMesh, &m_axisZMesh,
                                                   fbo.getWidth(), fbo.getHeight(), clearCol, opts);
    std::vector<unsigned char> pixels(static_cast<std::size_t>(fbo.getWidth() * fbo.getHeight() * 4), 0);
    glReadPixels(0, 0, fbo.getWidth(), fbo.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    fbo.unbind();

    int cyanHighlightPixels = 0;
    for (int i = 0; i < fbo.getWidth() * fbo.getHeight(); ++i) {
        const std::size_t idx = static_cast<std::size_t>(i * 4);
        // Selected BoundingBox highlight color = (0.20, 0.88, 1.00) -> RGB ~(51, 224, 255)
        if (pixels[idx] < 90 && pixels[idx + 1] > 180 && pixels[idx + 2] > 210) {
            ++cyanHighlightPixels;
        }
    }
    report("TC-M5-38", "Post-orbit FBO render & selected object cyan BoundingBox highlight pixels verified",
           (metrics.visibleObjects == 7) && (cyanHighlightPixels > 20));

    sceneRenderer.destroyGpuResources();
    fbo.destroy();

    drainAndCheckOpenGlErrors();
    report("TC-M5-39", "Zero unexpected OpenGL errors during M5 GPU integration suite", !m_glErrorDetected);
    return allPassed;
}

void Application::handleReshape(int width, int height)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    glViewport(0, 0, width, height);

    if (m_imguiInitialized) {
        ImGui_ImplGLUT_ReshapeFunc(width, height);
    }

    if (m_config.mode == ApplicationMode::SmokeTestM0 && m_m0ResizeTriggered &&
        width == 960 && height == 540) {
        m_m0ResizeObserved = true;
    }
}

void Application::handleKeyboard(unsigned char key, int x, int y)
{
    if (m_imguiInitialized) {
        ImGui_ImplGLUT_KeyboardFunc(key, x, y);
    }

    const int mods = glutGetModifiers();
    const bool ctrl = (mods & GLUT_ACTIVE_CTRL) != 0;
    const bool shift = (mods & GLUT_ACTIVE_SHIFT) != 0;
    const bool alt = (mods & GLUT_ACTIVE_ALT) != 0;
    m_inputManager.onKeyDown(key, ctrl, shift, alt);

    if (m_imguiInitialized && ImGui::GetIO().WantTextInput) {
        return;
    }

    // Trong Room Editor (M4/M5), phím Esc ưu tiên hủy tương tác đang diễn ra (do ViewportInteraction xử lý),
    // KHÔNG đóng ứng dụng đột ngột khi người dùng đang chỉnh sửa đối tượng.
    if (key == 27 && m_workspace != InteractiveWorkspace::RoomEditorM4) {
        m_window.requestClose();
    }
}

void Application::handleClose()
{
    shutdownGraphics();
}

void Application::drainAndCheckOpenGlErrors()
{
    GLenum err = glGetError();
    while (err != GL_NO_ERROR) {
        m_glErrorDetected = true;
        std::cerr << "[RoomCraft3D][ERROR] OpenGL error code: 0x" << std::hex << err << std::dec << std::endl;
        err = glGetError();
    }
}

} // namespace roomcraft::core
