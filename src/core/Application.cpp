#include "roomcraft/core/Application.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glut.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
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

    if (!m_window.initialize(argc, argv)) {
        return false;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM0) {
        if (!initM0Resources()) {
            shutdownGraphics();
            return false;
        }
    } else {
        if (!initM1Resources()) {
            shutdownGraphics();
            return false;
        }
    }

    if (!initImGui()) {
        shutdownGraphics();
        return false;
    }

    // Kết nối các sự kiện cửa sổ từ Window tới Application và Dear ImGui
    WindowCallbacks callbacks;
    callbacks.onDisplay = [this]() { renderFrame(); };
    callbacks.onReshape = [this](int w, int h) { handleReshape(w, h); };
    callbacks.onClose = [this]() { handleClose(); };
    callbacks.onKeyboard = [this](unsigned char key, int x, int y) { handleKeyboard(key, x, y); };
    callbacks.onKeyboardUp = [this](unsigned char key, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_KeyboardUpFunc(key, x, y);
        }
    };
    callbacks.onSpecial = [this](int key, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_SpecialFunc(key, x, y);
        }
    };
    callbacks.onSpecialUp = [this](int key, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_SpecialUpFunc(key, x, y);
        }
    };
    callbacks.onMouse = [this](int button, int state, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_MouseFunc(button, state, x, y);
        }
    };
    callbacks.onMouseWheel = [this](int wheel, int direction, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_MouseWheelFunc(wheel, direction, x, y);
        }
    };
    callbacks.onMotion = [this](int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_MotionFunc(x, y);
        }
    };
    callbacks.onPassiveMotion = [this](int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_MotionFunc(x, y);
        }
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
                            m_m0PixelVerified &&
                            m_m0ResizeObserved &&
                            !m_glErrorDetected;
        std::cout << "[SMOKE] Result: " << (passed ? "PASS" : "FAIL") << std::endl;
        return passed ? 0 : 1;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM1) {
        const auto& stats = m_renderer.getLastFrameStats();
        std::cout << "[M1-SMOKE] Frames rendered       : " << m_frameCount << std::endl;
        std::cout << "[M1-SMOKE] Last frame draw calls : " << stats.drawCalls
                  << " (vertices=" << stats.vertexCount
                  << ", indices=" << stats.indexCount << ")" << std::endl;
        std::cout << "[M1-SMOKE] Integration suite     : " << (m_m1SuitePassed ? "PASS" : "FAIL") << std::endl;
        std::cout << "[M1-SMOKE] Multi-cube pixels     : " << (m_m1MultiCubePixelsVerified ? "PASS" : "FAIL") << std::endl;
        std::cout << "[M1-SMOKE] OpenGL errors         : " << (m_glErrorDetected ? "YES" : "NONE") << std::endl;

        const bool passed = (m_frameCount >= m_config.smokeTestFrames) &&
                            m_m1SuiteExecuted &&
                            m_m1SuitePassed &&
                            m_m1MultiCubePixelsVerified &&
                            !m_glErrorDetected;
        std::cout << "[M1-SMOKE] Final Result          : " << (passed ? "PASS" : "FAIL") << std::endl;
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

    // Bảng màu tối theo định hướng RoomCraft Design System
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

bool Application::initM1Resources()
{
    // 1. Nạp Shader 3D (mesh.vert/frag) và Shader lưới/trục (grid.vert/frag)
    if (!m_meshShader.loadFromFiles(resolveAssetPath("assets/shaders/mesh.vert"),
                                    resolveAssetPath("assets/shaders/mesh.frag"))) {
        return false;
    }

    if (!m_gridShader.loadFromFiles(resolveAssetPath("assets/shaders/grid.vert"),
                                    resolveAssetPath("assets/shaders/grid.frag"))) {
        return false;
    }

    // 2. Tạo Indexed Cube Mesh (fixture kiểm thử Graphics Engine M1)
    std::vector<graphics::Vertex> cubeVerts;
    std::vector<std::uint32_t> cubeIndices;
    graphics::Mesh::buildUnitCubeFixture(cubeVerts, cubeIndices);
    if (!m_cubeMesh.create(cubeVerts, cubeIndices, graphics::PrimitiveTopology::Triangles)) {
        return false;
    }

    // 3. Tạo Grid trên mặt phẳng XZ và 3 trục tọa độ X, Y, Z bằng GL_LINES (Modern OpenGL)
    std::vector<graphics::Vertex> gridVerts;
    std::vector<std::uint32_t> gridIndices;
    graphics::Mesh::buildGridFixture(6, 1.0f, gridVerts, gridIndices);
    if (!m_gridMesh.create(gridVerts, gridIndices, graphics::PrimitiveTopology::Lines)) {
        return false;
    }

    std::vector<graphics::Vertex> axisVerts;
    std::vector<std::uint32_t> axisIndices;
    graphics::Mesh::buildAxisLineFixture({0.0f, 0.005f, 0.0f}, {2.5f, 0.005f, 0.0f}, axisVerts, axisIndices);
    if (!m_axisXMesh.create(axisVerts, axisIndices, graphics::PrimitiveTopology::Lines)) {
        return false;
    }
    graphics::Mesh::buildAxisLineFixture({0.0f, 0.0f, 0.0f}, {0.0f, 2.5f, 0.0f}, axisVerts, axisIndices);
    if (!m_axisYMesh.create(axisVerts, axisIndices, graphics::PrimitiveTopology::Lines)) {
        return false;
    }
    graphics::Mesh::buildAxisLineFixture({0.0f, 0.005f, 0.0f}, {0.0f, 0.005f, 2.5f}, axisVerts, axisIndices);
    if (!m_axisZMesh.create(axisVerts, axisIndices, graphics::PrimitiveTopology::Lines)) {
        return false;
    }

    // 4. Khởi tạo Off-screen Framebuffer (mặc định 860x640, tự resize theo viewport panel)
    if (!m_framebuffer.create(860, 640)) {
        return false;
    }

    // 5. Thiết lập scene thử nghiệm gồm nhiều khối lập phương 3D có che khuất nhau
    setupDefaultDemoScene();
    m_camera.reset();
    m_camera.setAspectRatio(static_cast<float>(m_framebuffer.getWidth()) /
                            static_cast<float>(m_framebuffer.getHeight()));

    std::cout << "[RoomCraft3D] M1 Core Graphics Engine resources initialized (Shaders, Indexed Cube, Grid/Axes, Framebuffer)."
              << std::endl;
    return true;
}

void Application::setupDefaultDemoScene()
{
    m_demoCubes.clear();

    // Cube A (gần camera hơn, màu đỏ dịu) — chồng hình chiếu lên Cube B để quan sát trực tiếp Depth Testing
    DemoCubeObject cubeA;
    cubeA.name = "Cube A (Front Red)";
    cubeA.transform = scene::Transform({0.25f, 0.50f, 1.10f}, {0.0f, 18.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    cubeA.defaultTransform = cubeA.transform;
    cubeA.color = {0.88f, 0.34f, 0.34f, 1.0f};
    m_demoCubes.push_back(cubeA);

    // Cube B (xa camera hơn, vẽ sau Cube A, màu xanh dương) — nếu tắt Depth Test sẽ đè lên Cube A
    DemoCubeObject cubeB;
    cubeB.name = "Cube B (Back Blue)";
    cubeB.transform = scene::Transform({-0.45f, 0.65f, -0.55f}, {0.0f, -24.0f, 0.0f}, {1.35f, 1.30f, 1.35f});
    cubeB.defaultTransform = cubeB.transform;
    cubeB.color = {0.35f, 0.56f, 0.88f, 1.0f};
    m_demoCubes.push_back(cubeB);

    // Cube C (bên phải, non-uniform scaling, màu xanh lá)
    DemoCubeObject cubeC;
    cubeC.name = "Cube C (Scaled Green)";
    cubeC.transform = scene::Transform({1.95f, 0.40f, -0.35f}, {12.0f, 35.0f, 0.0f}, {0.85f, 0.80f, 1.45f});
    cubeC.defaultTransform = cubeC.transform;
    cubeC.color = {0.30f, 0.78f, 0.52f, 1.0f};
    m_demoCubes.push_back(cubeC);
}

void Application::shutdownGraphics()
{
    if (!m_initialized && !m_imguiInitialized &&
        !m_meshShader.isValid() && !m_gridShader.isValid() && !m_m0BasicShader.isValid() &&
        !m_cubeMesh.isValid() && !m_framebuffer.isComplete() && m_m0Vao == 0 && m_m0Vbo == 0) {
        return;
    }

    // 1. Giải phóng Dear ImGui trước khi hủy OpenGL context
    if (m_imguiInitialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGLUT_Shutdown();
        ImGui::DestroyContext();
        m_imguiInitialized = false;
    }

    // 2. Giải phóng toàn bộ tài nguyên M1 trước khi OpenGL context bị hủy
    m_framebuffer.destroy();
    m_cubeMesh.destroy();
    m_gridMesh.destroy();
    m_axisXMesh.destroy();
    m_axisYMesh.destroy();
    m_axisZMesh.destroy();
    m_meshShader.destroy();
    m_gridShader.destroy();

    // 3. Giải phóng tài nguyên M0 regression (nếu có)
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
    std::cout << "[RoomCraft3D] GPU & Dear ImGui resources released cleanly." << std::endl;
}

std::filesystem::path Application::resolveAssetPath(const std::filesystem::path& relativePath) const
{
    const auto candidateExe = m_executableDir / relativePath;
    if (std::filesystem::exists(candidateExe)) {
        return candidateExe;
    }

    const auto candidateCwd = std::filesystem::current_path() / relativePath;
    if (std::filesystem::exists(candidateCwd)) {
        return candidateCwd;
    }

    return candidateExe;
}

void Application::renderFrame()
{
    if (!m_initialized) {
        return;
    }

    // Cập nhật FPS và frame time (ms) từ đồng hồ thực tế của FreeGLUT
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

    // Chạy bộ kiểm thử GPU tích hợp một lần ở khung hình đầu tiên khi ở chế độ --m1-smoke-test
    if (m_config.mode == ApplicationMode::SmokeTestM1 && !m_m1SuiteExecuted) {
        m_m1SuiteExecuted = true;
        m_m1SuitePassed = runM1IntegrationSuiteOnce();
        // Khôi phục camera & kích thước FBO chuẩn cho vòng render 10 frames tiếp theo
        m_framebuffer.resize(800, 600);
        m_camera.reset();
        m_camera.setAspectRatio(static_cast<float>(m_framebuffer.getWidth()) /
                                static_cast<float>(m_framebuffer.getHeight()));
    }

    renderM1InteractiveFrame();

    if (m_config.mode == ApplicationMode::SmokeTestM1) {
        if (m_frameCount == 3 && !m_m1MultiCubePixelsVerified) {
            m_m1MultiCubePixelsVerified = verifyM1MultiCubeRenderedPixels();
        }
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

void Application::renderM1DemoSceneToFramebuffer(float faceContrast)
{
    if (!m_framebuffer.isComplete()) {
        return;
    }

    m_framebuffer.bind();
    m_renderer.beginFrame(m_framebuffer.getWidth(),
                          m_framebuffer.getHeight(),
                          m_clearColor,
                          m_depthTestEnabled);

    // 1. Vẽ lưới căn chỉnh (XZ) và hệ trục tọa độ XYZ bằng GL_LINES
    if (m_showGridAndAxes) {
        const glm::mat4 identity(1.0f);
        m_renderer.drawLines(m_gridMesh, m_gridShader, identity, m_camera, {0.30f, 0.33f, 0.40f, 1.0f});
        m_renderer.drawLines(m_axisXMesh, m_gridShader, identity, m_camera, {0.92f, 0.30f, 0.30f, 1.0f}); // +X Đỏ
        m_renderer.drawLines(m_axisYMesh, m_gridShader, identity, m_camera, {0.30f, 0.88f, 0.42f, 1.0f}); // +Y Xanh lá
        m_renderer.drawLines(m_axisZMesh, m_gridShader, identity, m_camera, {0.34f, 0.56f, 0.96f, 1.0f}); // +Z Xanh dương
    }

    // 2. Vẽ các khối lập phương 3D thử nghiệm (Cube A, Cube B, Cube C) dùng chung 1 Indexed Cube Mesh
    for (const auto& cube : m_demoCubes) {
        m_renderer.drawMesh(m_cubeMesh, m_meshShader, cube.transform, m_camera, cube.color, faceContrast);
    }

    m_renderer.endFrame();
    graphics::Framebuffer::unbind();
}

void Application::renderM1InteractiveFrame()
{
    // 1. Render scene 3D vào off-screen Framebuffer
    renderM1DemoSceneToFramebuffer(0.45f);

    // 2. Bắt đầu frame Dear ImGui trên cửa sổ chính
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();

    renderM1DiagnosticsUi();
    renderM1ViewportUi();

    ImGui::Render();

    // 3. Xóa Default Framebuffer và vẽ toàn bộ giao diện ImGui (bao gồm texture của 3D Viewport)
    glViewport(0, 0, m_window.getWidth(), m_window.getHeight());
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.070f, 0.075f, 0.095f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    drainAndCheckOpenGlErrors();
    m_window.swapBuffers();
}

void Application::renderM1DiagnosticsUi()
{
    const auto& gpu = m_window.getGpuInfo();
    const auto& stats = m_renderer.getLastFrameStats();

    ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(370.0f, 696.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("RoomCraft 3D - M1 Diagnostics")) {
        ImGui::TextUnformatted("Core Graphics Engine (OpenGL 3.3 Core)");
        ImGui::Separator();

        ImGui::Text("GPU Renderer : %s", gpu.renderer.c_str());
        ImGui::Text("OpenGL       : %s", gpu.glVersion.c_str());
        ImGui::Text("Profile      : %s", gpu.profile.c_str());
        ImGui::Text("GLSL         : %s", gpu.glslVersion.c_str());

        ImGui::Separator();
        ImGui::Text("FPS          : %.1f", m_currentFps);
        ImGui::Text("Frame Time   : %.2f ms", m_frameTimeMs);
        ImGui::Text("Draw Calls   : %u", stats.drawCalls);
        ImGui::Text("Vertex Count : %u", stats.vertexCount);
        ImGui::Text("Index Count  : %u", stats.indexCount);
        ImGui::Text("Window Size  : %d x %d", m_window.getWidth(), m_window.getHeight());
        ImGui::Text("FBO Viewport : %d x %d (%s)",
                    m_framebuffer.getWidth(),
                    m_framebuffer.getHeight(),
                    m_framebuffer.isComplete() ? "Complete" : "Incomplete");

        ImGui::Separator();
        ImGui::TextUnformatted("Render & Depth Controls");
        ImGui::Checkbox("Enable Depth Testing (GL_DEPTH_TEST)", &m_depthTestEnabled);
        ImGui::Checkbox("Show Grid (XZ) & XYZ Axes", &m_showGridAndAxes);
        ImGui::ColorEdit3("Clear Color", &m_clearColor.r);

        ImGui::Separator();
        ImGui::TextUnformatted("Camera & Projection");
        int projIdx = (m_camera.getProjectionMode() == graphics::ProjectionMode::Perspective) ? 0 : 1;
        if (ImGui::RadioButton("Perspective", &projIdx, 0)) {
            m_camera.setProjectionMode(graphics::ProjectionMode::Perspective);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Orthographic", &projIdx, 1)) {
            m_camera.setProjectionMode(graphics::ProjectionMode::Orthographic);
        }

        if (ImGui::Button("Default 3D")) {
            m_camera.setPerspectivePreset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Top Ortho")) {
            m_camera.setTopOrthoPreset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Front Ortho")) {
            m_camera.setFrontOrthoPreset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Camera")) {
            m_camera.reset();
        }

        float fov = m_camera.getFieldOfView();
        if (m_camera.getProjectionMode() == graphics::ProjectionMode::Perspective) {
            if (ImGui::SliderFloat("FOV (deg)", &fov, 20.0f, 90.0f)) {
                m_camera.setFieldOfView(fov);
            }
        } else {
            float orthoHalf = m_camera.getOrthoHalfHeight();
            if (ImGui::SliderFloat("Ortho Size", &orthoHalf, 1.0f, 10.0f)) {
                m_camera.setOrthoHalfHeight(orthoHalf);
            }
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Demo Object Transform");
        if (!m_demoCubes.empty()) {
            m_selectedCubeIndex = std::clamp(m_selectedCubeIndex, 0, static_cast<int>(m_demoCubes.size()) - 1);
            if (ImGui::BeginCombo("Target Object", m_demoCubes[static_cast<std::size_t>(m_selectedCubeIndex)].name.c_str())) {
                for (int i = 0; i < static_cast<int>(m_demoCubes.size()); ++i) {
                    const bool isSelected = (i == m_selectedCubeIndex);
                    if (ImGui::Selectable(m_demoCubes[static_cast<std::size_t>(i)].name.c_str(), isSelected)) {
                        m_selectedCubeIndex = i;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            auto& obj = m_demoCubes[static_cast<std::size_t>(m_selectedCubeIndex)];
            ImGui::DragFloat3("Position (m)", &obj.transform.position.x, 0.05f, -10.0f, 10.0f);
            ImGui::DragFloat3("Rotation (deg)", &obj.transform.rotationDegrees.x, 1.0f, -360.0f, 360.0f);

            glm::vec3 editScale = obj.transform.scale;
            if (ImGui::DragFloat3("Scale", &editScale.x, 0.05f, 0.1f, 5.0f)) {
                obj.transform.setScaleSafe(editScale, 0.05f);
            }

            if (ImGui::Button("Reset Object Transform")) {
                obj.transform = obj.defaultTransform;
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("Press ESC or close window to exit cleanly.");
    }
    ImGui::End();
}

void Application::renderM1ViewportUi()
{
    ImGui::SetNextWindowPos(ImVec2(394.0f, 12.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(874.0f, 696.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("3D Viewport (Off-screen Framebuffer)")) {
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const int targetW = static_cast<int>(avail.x);
        const int targetH = static_cast<int>(avail.y);

        // Chỉ cập nhật kích thước Framebuffer khi kích thước khả dụng > 0 và thực sự thay đổi
        if (targetW > 0 && targetH > 0 && m_config.mode == ApplicationMode::InteractiveM1) {
            if (targetW != m_framebuffer.getWidth() || targetH != m_framebuffer.getHeight()) {
                m_framebuffer.resize(targetW, targetH);
            }
            m_camera.setAspectRatio(static_cast<float>(m_framebuffer.getWidth()) /
                                    static_cast<float>(m_framebuffer.getHeight()));
        }

        if (m_framebuffer.isComplete() && targetW > 0 && targetH > 0) {
            // Quan trọng: Texture OpenGL có gốc (0, 0) ở góc dưới-trái còn ImGui có gốc ở góc trên-trái,
            // nên phải đảo trục V: uv0 = (0, 1), uv1 = (1, 0) để hình ảnh 3D không bị lộn ngược.
            const auto texId = static_cast<ImTextureID>(static_cast<std::uintptr_t>(m_framebuffer.getColorTextureId()));
            ImGui::Image(texId, avail, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        } else {
            ImGui::TextDisabled("Framebuffer inactive (viewport size is 0).");
        }
    }
    ImGui::End();
}

bool Application::runM1IntegrationSuiteOnce()
{
    bool allPassed = true;
    const auto report = [&allPassed](const char* id, const char* desc, bool ok) {
        std::cout << "[" << id << "] " << desc << ": " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok) {
            allPassed = false;
        }
    };

    // TC-M1-02: Shader hợp lệ đã biên dịch & link thành công
    report("TC-M1-02", "Valid 3D & Grid shaders compiled and linked",
           m_meshShader.isValid() && m_gridShader.isValid());

    // TC-M1-03: Shader sai cú pháp phải trả về false kèm thông báo lỗi rõ ràng
    {
        graphics::Shader badShader;
        std::string errMsg;
        const bool compiled = badShader.loadFromSource(
            "#version 330 core\nvoid main() { INVALID_SYNTAX_TOKEN }",
            "#version 330 core\nout vec4 FragColor; void main() { FragColor = vec4(1.0); }",
            "invalid_test_shader",
            &errMsg);
        const bool rejectedCleanly = (!compiled) && (!badShader.isValid()) && (!errMsg.empty());
        report("TC-M1-03", "Invalid GLSL syntax rejected with diagnostic log", rejectedCleanly);
    }

    // TC-M1-04: Indexed Cube Mesh hợp lệ (24 đỉnh, 36 chỉ mục)
    report("TC-M1-04", "Indexed cube mesh uploaded (24 vertices, 36 indices)",
           m_cubeMesh.isValid() && m_cubeMesh.getVertexCount() == 24 && m_cubeMesh.getIndexCount() == 36);

    // TC-M1-05: Từ chối dữ liệu chỉ mục vượt biên trên GPU Mesh::create
    {
        graphics::Mesh badMesh;
        std::vector<graphics::Vertex> verts(3);
        std::vector<std::uint32_t> badIndices = {0, 1, 99};
        std::string errMsg;
        const bool created = badMesh.create(verts, badIndices, graphics::PrimitiveTopology::Triangles, &errMsg);
        report("TC-M1-05", "Out-of-bounds mesh indices rejected cleanly",
               (!created) && (!badMesh.isValid()) && (!errMsg.empty()));
    }

    // TC-M1-13 & TC-M1-14: Framebuffer completeness, no-op resize, 0x0 safety, actual resize
    report("TC-M1-13", "Off-screen Framebuffer complete with valid color texture",
           m_framebuffer.isComplete() && m_framebuffer.getColorTextureId() != 0);
    report("TC-M1-14", "Framebuffer resize, no-op caching, and 0x0 safety",
           verifyM1FramebufferResizeAndZeroSafety());

    // TC-M1-11: Chứng minh Depth Testing độc lập thứ tự draw + đối chứng khi tắt Depth Test
    report("TC-M1-11", "Depth testing order-independent proof + depth-off control",
           verifyM1DepthTestingOrderIndependence());

    // TC-M1-15: Kiểm thử RAII & move semantics không double-delete
    report("TC-M1-15", "RAII move ownership & safe resource destruction",
           verifyM1RaiiMoveLifecycle());

    drainAndCheckOpenGlErrors();
    report("TC-M1-16", "Zero unexpected OpenGL errors during integration suite", !m_glErrorDetected);

    return allPassed;
}

bool Application::verifyM1FramebufferResizeAndZeroSafety()
{
    graphics::Framebuffer fbo;
    if (!fbo.create(640, 360) || !fbo.isComplete()) {
        return false;
    }
    const unsigned int initialFboId = fbo.getFboId();
    const unsigned int initialTexId = fbo.getColorTextureId();

    // Resize về đúng kích thước cũ -> không được cấp phát lại
    if (!fbo.resize(640, 360) || fbo.getFboId() != initialFboId || fbo.getColorTextureId() != initialTexId) {
        return false;
    }

    // Resize về 0x0 (mô phỏng cửa sổ thu nhỏ) -> giữ nguyên FBO hợp lệ, không phát sinh lỗi GL
    if (!fbo.resize(0, 0) || !fbo.isComplete() || fbo.getWidth() != 640 || fbo.getHeight() != 360) {
        return false;
    }

    // Resize sang kích thước mới 800x600 -> cập nhật thành công và vẫn complete
    if (!fbo.resize(800, 600) || !fbo.isComplete() || fbo.getWidth() != 800 || fbo.getHeight() != 600) {
        return false;
    }

    fbo.destroy();
    return !fbo.isComplete() && fbo.getFboId() == 0 && fbo.getColorTextureId() == 0;
}

bool Application::verifyM1DepthTestingOrderIndependence()
{
    graphics::Framebuffer fbo;
    if (!fbo.create(256, 256)) {
        return false;
    }

    graphics::Camera testCam;
    testCam.setProjectionMode(graphics::ProjectionMode::Perspective);
    testCam.setAspectRatio(1.0f);
    testCam.setFieldOfView(45.0f);
    testCam.setClipPlanes(0.1f, 50.0f);
    testCam.lookAt({0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

    // Hai khối phủ kín tâm màn hình dọc theo tia nhìn -Z:
    // - nearRed ở z = +1.5 (gần camera hơn)
    // - farBlue ở z = -1.5 (xa camera hơn)
    const scene::Transform nearRed({0.0f, 0.0f, 1.5f}, {0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 0.4f});
    const scene::Transform farBlue({0.0f, 0.0f, -1.5f}, {0.0f, 0.0f, 0.0f}, {2.5f, 2.5f, 0.4f});
    const glm::vec4 redColor{1.0f, 0.0f, 0.0f, 1.0f};
    const glm::vec4 blueColor{0.0f, 0.0f, 1.0f, 1.0f};

    const auto readCenterPixel = [&]() -> std::array<unsigned char, 4> {
        std::array<unsigned char, 4> px{0, 0, 0, 0};
        glReadPixels(128, 128, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
        return px;
    };

    // Pass 1: Depth ON, vẽ Xa (Blue) trước -> Gần (Red) sau => tâm phải là ĐỎ
    fbo.bind();
    m_renderer.beginFrame(256, 256, {0.0f, 0.0f, 0.0f, 1.0f}, true);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, farBlue, testCam, blueColor, 0.0f);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, nearRed, testCam, redColor, 0.0f);
    m_renderer.endFrame();
    const auto pxOrderFarFirst = readCenterPixel();

    // Pass 2: Depth ON, ĐẢO THỨ TỰ VẼ: vẽ Gần (Red) trước -> Xa (Blue) sau => tâm VẪN PHẢI LÀ ĐỎ
    m_renderer.beginFrame(256, 256, {0.0f, 0.0f, 0.0f, 1.0f}, true);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, nearRed, testCam, redColor, 0.0f);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, farBlue, testCam, blueColor, 0.0f);
    m_renderer.endFrame();
    const auto pxOrderNearFirst = readCenterPixel();

    // Pass 3 (Đối chứng): Depth OFF, vẽ Gần (Red) trước -> Xa (Blue) sau => Blue phải đè lên thành XANH DƯƠNG
    m_renderer.beginFrame(256, 256, {0.0f, 0.0f, 0.0f, 1.0f}, false);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, nearRed, testCam, redColor, 0.0f);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, farBlue, testCam, blueColor, 0.0f);
    m_renderer.endFrame();
    const auto pxDepthDisabled = readCenterPixel();

    graphics::Framebuffer::unbind();
    fbo.destroy();

    const bool pass1IsRed = (pxOrderFarFirst[0] > 240 && pxOrderFarFirst[1] < 15 && pxOrderFarFirst[2] < 15);
    const bool pass2IsRed = (pxOrderNearFirst[0] > 240 && pxOrderNearFirst[1] < 15 && pxOrderNearFirst[2] < 15);
    const bool pass3IsBlue = (pxDepthDisabled[2] > 240 && pxDepthDisabled[0] < 15 && pxDepthDisabled[1] < 15);

    std::cout << "  [DepthTest] Far->Near (Depth ON)  RGB="
              << static_cast<int>(pxOrderFarFirst[0]) << ","
              << static_cast<int>(pxOrderFarFirst[1]) << ","
              << static_cast<int>(pxOrderFarFirst[2]) << std::endl;
    std::cout << "  [DepthTest] Near->Far (Depth ON)  RGB="
              << static_cast<int>(pxOrderNearFirst[0]) << ","
              << static_cast<int>(pxOrderNearFirst[1]) << ","
              << static_cast<int>(pxOrderNearFirst[2]) << std::endl;
    std::cout << "  [DepthTest] Near->Far (Depth OFF) RGB="
              << static_cast<int>(pxDepthDisabled[0]) << ","
              << static_cast<int>(pxDepthDisabled[1]) << ","
              << static_cast<int>(pxDepthDisabled[2]) << std::endl;

    return pass1IsRed && pass2IsRed && pass3IsBlue;
}

bool Application::verifyM1RaiiMoveLifecycle()
{
    // Kiểm thử move semantics cho Shader
    graphics::Shader s1;
    if (!s1.loadFromFiles(resolveAssetPath("assets/shaders/grid.vert"),
                          resolveAssetPath("assets/shaders/grid.frag"))) {
        return false;
    }
    const unsigned int shaderHandle = s1.getProgramId();
    graphics::Shader s2(std::move(s1));
    if (s1.isValid() || s1.getProgramId() != 0 || s2.getProgramId() != shaderHandle) {
        return false;
    }
    s2.destroy();
    s2.destroy(); // Chống double-delete

    // Kiểm thử move semantics cho Mesh
    std::vector<graphics::Vertex> verts;
    std::vector<std::uint32_t> indices;
    graphics::Mesh::buildUnitCubeFixture(verts, indices);
    graphics::Mesh m1;
    if (!m1.create(verts, indices, graphics::PrimitiveTopology::Triangles)) {
        return false;
    }
    const unsigned int vaoHandle = m1.getVaoId();
    graphics::Mesh m2(std::move(m1));
    if (m1.isValid() || m1.getVaoId() != 0 || m2.getVaoId() != vaoHandle) {
        return false;
    }
    m2.destroy();
    m2.destroy();

    // Kiểm thử move semantics cho Framebuffer
    graphics::Framebuffer f1;
    if (!f1.create(128, 128)) {
        return false;
    }
    const unsigned int fboHandle = f1.getFboId();
    graphics::Framebuffer f2(std::move(f1));
    if (f1.isComplete() || f1.getFboId() != 0 || f2.getFboId() != fboHandle) {
        return false;
    }
    f2.destroy();
    f2.destroy();

    return true;
}

bool Application::verifyM1MultiCubeRenderedPixels()
{
    if (!m_framebuffer.isComplete() || m_demoCubes.size() < 3) {
        return false;
    }

    // Chiếu vị trí tâm từng Cube lên tọa độ pixel trong Framebuffer để đọc và xác minh màu thực tế
    const glm::mat4 vp = m_camera.getProjectionMatrix() * m_camera.getViewMatrix();
    const int w = m_framebuffer.getWidth();
    const int h = m_framebuffer.getHeight();

    m_framebuffer.bind();

    const auto sampleWorldPoint = [&](const glm::vec3& worldPos) -> std::array<unsigned char, 4> {
        const glm::vec4 clip = vp * glm::vec4(worldPos, 1.0f);
        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        const int px = std::clamp(static_cast<int>((ndc.x * 0.5f + 0.5f) * static_cast<float>(w)), 0, w - 1);
        const int py = std::clamp(static_cast<int>((ndc.y * 0.5f + 0.5f) * static_cast<float>(h)), 0, h - 1);
        std::array<unsigned char, 4> rgba{0, 0, 0, 0};
        glReadPixels(px, py, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        return rgba;
    };

    // Cube A (Đỏ), phần lộ phía trên của Cube B (Xanh dương), Cube C (Xanh lá)
    const auto pxA = sampleWorldPoint(m_demoCubes[0].transform.position);
    const auto pxB = sampleWorldPoint(m_demoCubes[1].transform.position + glm::vec3(-0.35f, 0.35f, 0.0f));
    const auto pxC = sampleWorldPoint(m_demoCubes[2].transform.position);

    graphics::Framebuffer::unbind();

    const bool cubeAIsRedDominant = (pxA[0] > pxA[1] + 40 && pxA[0] > pxA[2] + 40);
    const bool cubeBIsBlueDominant = (pxB[2] > pxB[0] + 40 && pxB[2] > pxB[1] + 20);
    const bool cubeCIsGreenDominant = (pxC[1] > pxC[0] + 40 && pxC[1] > pxC[2] + 20);

    std::cout << "[TC-M1-12] Multi-cube framebuffer samples: "
              << "CubeA=(" << static_cast<int>(pxA[0]) << "," << static_cast<int>(pxA[1]) << "," << static_cast<int>(pxA[2]) << ") "
              << "CubeB=(" << static_cast<int>(pxB[0]) << "," << static_cast<int>(pxB[1]) << "," << static_cast<int>(pxB[2]) << ") "
              << "CubeC=(" << static_cast<int>(pxC[0]) << "," << static_cast<int>(pxC[1]) << "," << static_cast<int>(pxC[2]) << ")"
              << std::endl;

    const auto& stats = m_renderer.getLastFrameStats();
    const bool statsValid = (stats.drawCalls >= 3 && stats.vertexCount >= 72 && stats.indexCount >= 108);

    return cubeAIsRedDominant && cubeBIsBlueDominant && cubeCIsGreenDominant && statsValid;
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
        if (ImGui::GetIO().WantCaptureKeyboard) {
            return;
        }
    }

    // Phím ESC để thoát ứng dụng an toàn
    if (key == 27) {
        m_window.requestClose();
    }
}

void Application::handleClose()
{
    // Giải phóng toàn bộ tài nguyên GPU & ImGui TRƯỚC khi FreeGLUT hủy OpenGL context
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
