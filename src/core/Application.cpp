#include "roomcraft/core/Application.hpp"

// Lưu ý thứ tự include bắt buộc:
// Luôn include <glad/glad.h> TRƯỚC <GL/freeglut.h> để tránh xung đột với <GL/gl.h> của hệ thống.
#include <glad/glad.h>
#include <GL/freeglut.h>

#include <imgui.h>
#include <imgui_impl_glut.h>
#include <imgui_impl_opengl3.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace roomcraft::core {

Application* Application::s_instance = nullptr;

Application::Application(ApplicationConfig config)
    : m_config(std::move(config))
    , m_viewportWidth(m_config.width)
    , m_viewportHeight(m_config.height)
{
}

Application::~Application()
{
    shutdownGraphics();
    if (s_instance == this) {
        s_instance = nullptr;
    }
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

    s_instance = this;

    if (!initWindowAndContext(argc, argv)) {
        return false;
    }

    if (!initGladAndLogGpuInfo()) {
        shutdownGraphics();
        return false;
    }

    if (!initTrianglePipeline()) {
        shutdownGraphics();
        return false;
    }

    if (!initImGui()) {
        shutdownGraphics();
        return false;
    }

    // Đăng ký các callback FreeGLUT sau khi toàn bộ hệ thống đã khởi tạo
    glutDisplayFunc(&Application::displayCallback);
    glutReshapeFunc(&Application::reshapeCallback);
    glutCloseFunc(&Application::closeCallback);
    glutKeyboardFunc(&Application::keyboardCallback);
    glutKeyboardUpFunc(&Application::keyboardUpCallback);
    glutSpecialFunc(&Application::specialCallback);
    glutSpecialUpFunc(&Application::specialUpCallback);
    glutMouseFunc(&Application::mouseCallback);
    glutMouseWheelFunc(&Application::mouseWheelCallback);
    glutMotionFunc(&Application::motionCallback);
    glutPassiveMotionFunc(&Application::passiveMotionCallback);

    m_lastFpsTimeMs = glutGet(GLUT_ELAPSED_TIME);
    m_graphicsInitialized = true;

    std::cout << "[RoomCraft3D] M0 initialization completed successfully." << std::endl;
    return true;
}

int Application::run()
{
    if (!m_graphicsInitialized) {
        std::cerr << "[RoomCraft3D][ERROR] Cannot run Application before initialize() succeeds." << std::endl;
        return 1;
    }

    std::cout << "[RoomCraft3D] Entering FreeGLUT main loop..." << std::endl;
    glutMainLoop();
    std::cout << "[RoomCraft3D] Exited FreeGLUT main loop cleanly." << std::endl;

    // Phòng hờ nếu nền tảng không gọi glutCloseFunc trước khi trả về
    shutdownGraphics();

    if (m_config.smokeTestMode) {
        std::cout << "[SMOKE] Frames rendered: " << m_frameCount << std::endl;
        std::cout << "[SMOKE] Triangle pixel verified: "
                  << (m_smokeTrianglePixelVerified ? "YES" : "NO")
                  << " (Center RGB="
                  << static_cast<int>(m_smokeSampledPixel[0]) << ","
                  << static_cast<int>(m_smokeSampledPixel[1]) << ","
                  << static_cast<int>(m_smokeSampledPixel[2]) << ")" << std::endl;
        std::cout << "[SMOKE] Resize verified: " << (m_smokeResizeObserved ? "YES" : "NO")
                  << " (" << m_viewportWidth << "x" << m_viewportHeight << ")" << std::endl;
        std::cout << "[SMOKE] OpenGL errors: " << (m_glErrorDetected ? "YES" : "NONE") << std::endl;

        const bool passed = (m_frameCount >= m_config.smokeTestFrames) &&
                            m_smokeTrianglePixelVerified &&
                            m_smokeResizeObserved &&
                            !m_glErrorDetected;
        std::cout << "[SMOKE] Result: " << (passed ? "PASS" : "FAIL") << std::endl;
        return passed ? 0 : 1;
    }

    return 0;
}

bool Application::initWindowAndContext(int& argc, char** argv)
{
    glutInit(&argc, argv);

    // Yêu cầu rõ ràng OpenGL 3.3 Core Profile (không sử dụng fixed-function pipeline cũ)
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    // Quan trọng: cho phép glutMainLoop() trả về thay vì gọi exit() đột ngột khi đóng cửa sổ,
    // giúp destructor và quy trình dọn dẹp tài nguyên chạy đầy đủ.
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(m_config.width, m_config.height);
    glutInitWindowPosition(100, 100);

    m_windowId = glutCreateWindow(m_config.title.c_str());
    if (m_windowId <= 0) {
        std::cerr << "[RoomCraft3D][ERROR] Failed to create FreeGLUT window." << std::endl;
        return false;
    }

    std::cout << "[RoomCraft3D] FreeGLUT window created ("
              << m_config.width << "x" << m_config.height << ")." << std::endl;
    return true;
}

bool Application::initGladAndLogGpuInfo()
{
    if (!gladLoadGL()) {
        std::cerr << "[RoomCraft3D][ERROR] Failed to initialize GLAD OpenGL function loader." << std::endl;
        return false;
    }

    glGetIntegerv(GL_MAJOR_VERSION, &m_glMajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &m_glMinorVersion);

    GLint profileMask = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profileMask);
    if ((profileMask & GL_CONTEXT_CORE_PROFILE_BIT) != 0) {
        m_glProfile = "Core Profile";
    } else if ((profileMask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) != 0) {
        m_glProfile = "Compatibility Profile";
    } else {
        m_glProfile = "Unknown Profile";
    }

    const auto toString = [](const GLubyte* raw) -> std::string {
        return raw != nullptr ? reinterpret_cast<const char*>(raw) : "Unknown";
    };

    m_gpuVendor = toString(glGetString(GL_VENDOR));
    m_gpuRenderer = toString(glGetString(GL_RENDERER));
    m_glVersion = toString(glGetString(GL_VERSION));
    m_glslVersion = toString(glGetString(GL_SHADING_LANGUAGE_VERSION));

    std::cout << "================ OpenGL Environment Info ================" << std::endl;
    std::cout << "GPU Vendor     : " << m_gpuVendor << std::endl;
    std::cout << "GPU Renderer   : " << m_gpuRenderer << std::endl;
    std::cout << "OpenGL Version : " << m_glVersion
              << " (Parsed: " << m_glMajorVersion << "." << m_glMinorVersion << ")" << std::endl;
    std::cout << "OpenGL Profile : " << m_glProfile << std::endl;
    std::cout << "GLSL Version   : " << m_glslVersion << std::endl;
    std::cout << "=========================================================" << std::endl;

    if (m_glMajorVersion < 3 || (m_glMajorVersion == 3 && m_glMinorVersion < 3)) {
        std::cerr << "[RoomCraft3D][ERROR] OpenGL 3.3 or higher is required, got "
                  << m_glMajorVersion << "." << m_glMinorVersion << std::endl;
        return false;
    }

    // Bật kiểm tra độ sâu (Depth Test) sẵn cho các hình học 3D về sau
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    return true;
}

bool Application::initTrianglePipeline()
{
    // 1. Đọc và biên dịch Vertex Shader + Fragment Shader từ thư mục assets/shaders
    const auto vertPath = resolveAssetPath("assets/shaders/basic.vert");
    const auto fragPath = resolveAssetPath("assets/shaders/basic.frag");

    std::string vertSource;
    std::string fragSource;
    if (!loadTextFile(vertPath, vertSource) || !loadTextFile(fragPath, fragSource)) {
        return false;
    }

    const unsigned int vertShader = compileShader(GL_VERTEX_SHADER, vertSource, vertPath.string());
    if (vertShader == 0) {
        return false;
    }

    const unsigned int fragShader = compileShader(GL_FRAGMENT_SHADER, fragSource, fragPath.string());
    if (fragShader == 0) {
        glDeleteShader(vertShader);
        return false;
    }

    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertShader);
    glAttachShader(m_shaderProgram, fragShader);
    glLinkProgram(m_shaderProgram);

    GLint linked = GL_FALSE;
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &linked);

    // Sau khi link, đối tượng shader trung gian có thể giải phóng ngay
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    if (linked != GL_TRUE) {
        GLint logLen = 0;
        glGetProgramiv(m_shaderProgram, GL_INFO_LOG_LENGTH, &logLen);
        std::string infoLog(static_cast<size_t>(logLen > 0 ? logLen : 1), '\0');
        glGetProgramInfoLog(m_shaderProgram, logLen, nullptr, infoLog.data());
        std::cerr << "[RoomCraft3D][ERROR] Shader program link failed:\n" << infoLog << std::endl;
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
        return false;
    }

    // 2. Dữ liệu đỉnh tam giác theo định dạng xen kẽ (interleaved):
    //    [x, y, z (NDC), r, g, b (màu đỉnh)]
    //    Thứ tự ngược chiều kim đồng hồ (CCW) chuẩn OpenGL.
    const float triangleVertices[] = {
        // Position (XYZ)        // Color (RGB)
        -0.55f, -0.45f, 0.0f,    0.92f, 0.34f, 0.34f, // Đỉnh trái dưới - Đỏ dịu
         0.55f, -0.45f, 0.0f,    0.34f, 0.80f, 0.56f, // Đỉnh phải dưới - Xanh lá dịu
         0.00f,  0.55f, 0.0f,    0.45f, 0.58f, 0.84f  // Đỉnh trên      - Xanh nhấn #7495D6
    };

    // 3. Khởi tạo VAO (lưu cấu hình thuộc tính đỉnh) và VBO (bộ nhớ đỉnh trên GPU)
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

    constexpr GLsizei stride = static_cast<GLsizei>(6 * sizeof(float));

    // Thuộc tính location = 0: vị trí vec3 (offset 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    // Thuộc tính location = 1: màu sắc vec3 (offset 3 * sizeof(float))
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "[RoomCraft3D] Basic GLSL shader & triangle VAO/VBO initialized." << std::endl;
    return true;
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

    // Áp dụng giao diện nền tối nhất quán với định hướng RoomCraft Design System
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.133f, 0.145f, 0.176f, 0.94f); // #22252D
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.173f, 0.188f, 0.227f, 1.0f); // #2C303A

    // Quan trọng: Kết hợp backend imgui_impl_glut (sự kiện FreeGLUT)
    // với imgui_impl_opengl3 (renderer Modern OpenGL 3.3 Core, KHÔNG dùng OpenGL2)
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

    // Đồng bộ kích thước viewport ban đầu cho ImGui
    ImGui_ImplGLUT_ReshapeFunc(m_viewportWidth, m_viewportHeight);

    m_imguiInitialized = true;
    std::cout << "[RoomCraft3D] Dear ImGui initialized (imgui_impl_glut + imgui_impl_opengl3 #version 330 core)."
              << std::endl;
    return true;
}

void Application::shutdownGraphics()
{
    if (!m_graphicsInitialized && !m_imguiInitialized && m_vao == 0 && m_vbo == 0 && m_shaderProgram == 0) {
        return;
    }

    // Giải phóng tài nguyên ImGui trước khi hủy OpenGL context
    if (m_imguiInitialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGLUT_Shutdown();
        ImGui::DestroyContext();
        m_imguiInitialized = false;
    }

    // Giải phóng VAO, VBO và Shader Program an toàn (chống double-delete bằng cách đặt về 0)
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_shaderProgram != 0) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }

    m_graphicsInitialized = false;
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

bool Application::loadTextFile(const std::filesystem::path& filePath, std::string& outContent) const
{
    std::ifstream file(filePath, std::ios::in);
    if (!file.is_open()) {
        std::cerr << "[RoomCraft3D][ERROR] Failed to open file: " << filePath.string() << std::endl;
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    outContent = buffer.str();
    return true;
}

unsigned int Application::compileShader(unsigned int shaderType,
                                        const std::string& source,
                                        const std::string& debugName) const
{
    const unsigned int shader = glCreateShader(shaderType);
    const char* srcPtr = source.c_str();
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string infoLog(static_cast<size_t>(logLen > 0 ? logLen : 1), '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, infoLog.data());
        std::cerr << "[RoomCraft3D][ERROR] Shader compilation failed (" << debugName << "):\n"
                  << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

void Application::renderFrame()
{
    if (!m_graphicsInitialized) {
        return;
    }

    // Cập nhật thống kê FPS mỗi 500ms
    ++m_frameCount;
    ++m_framesSinceLastFps;
    const int nowMs = glutGet(GLUT_ELAPSED_TIME);
    const int elapsedMs = nowMs - m_lastFpsTimeMs;
    if (elapsedMs >= 500) {
        m_currentFps = (static_cast<float>(m_framesSinceLastFps) * 1000.0f) / static_cast<float>(elapsedMs);
        m_framesSinceLastFps = 0;
        m_lastFpsTimeMs = nowMs;
    }

    // 1. Bắt đầu frame mới cho Dear ImGui (OpenGL3 + FreeGLUT)
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();

    // Cửa sổ kiểm thử tích hợp Dear ImGui cho Milestone M0
    ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(390.0f, 250.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("RoomCraft 3D - M0 Foundation Diagnostics")) {
        ImGui::Text("Status: OpenGL 3.3 Core + FreeGLUT + Dear ImGui");
        ImGui::Separator();
        ImGui::Text("GPU Vendor   : %s", m_gpuVendor.c_str());
        ImGui::Text("GPU Renderer : %s", m_gpuRenderer.c_str());
        ImGui::Text("OpenGL       : %s", m_glVersion.c_str());
        ImGui::Text("Profile      : %s", m_glProfile.c_str());
        ImGui::Text("GLSL         : %s", m_glslVersion.c_str());
        ImGui::Separator();
        ImGui::Text("Viewport     : %d x %d", m_viewportWidth, m_viewportHeight);
        ImGui::Text("FPS          : %.1f (Frame #%d)", m_currentFps, m_frameCount);
        ImGui::Checkbox("Render M0 Triangle (VAO/VBO/GLSL)", &m_showTriangle);
        ImGui::ColorEdit3("Clear Color", m_clearColor.data());
        ImGui::TextDisabled("Press ESC or close window to exit cleanly.");
    }
    ImGui::End();

    ImGui::Render();

    // 2. Xóa color buffer và depth buffer
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    glClearColor(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 3. Vẽ tam giác bằng Modern OpenGL 3.3 Core (GLSL Shader + VAO/VBO)
    if (m_showTriangle) {
        glUseProgram(m_shaderProgram);
        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    // 4. Vẽ giao diện Dear ImGui đè lên framebuffer
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Trong chế độ --smoke-test: đọc pixel tại tâm viewport (nằm bên trong tam giác)
    // để xác nhận GPU thực sự đã rasterize tam giác với màu khác màu nền
    if (m_config.smokeTestMode && !m_smokeTrianglePixelVerified && m_frameCount == 2) {
        glReadPixels(m_viewportWidth / 2, m_viewportHeight / 2, 1, 1,
                     GL_RGBA, GL_UNSIGNED_BYTE, m_smokeSampledPixel.data());
        const int bgR = static_cast<int>(m_clearColor[0] * 255.0f);
        const int bgG = static_cast<int>(m_clearColor[1] * 255.0f);
        const int bgB = static_cast<int>(m_clearColor[2] * 255.0f);
        const int diff = std::abs(static_cast<int>(m_smokeSampledPixel[0]) - bgR) +
                         std::abs(static_cast<int>(m_smokeSampledPixel[1]) - bgG) +
                         std::abs(static_cast<int>(m_smokeSampledPixel[2]) - bgB);
        m_smokeTrianglePixelVerified = (diff > 30);
    }

    // Kiểm tra lỗi OpenGL trong quá trình render
    GLenum err = glGetError();
    while (err != GL_NO_ERROR) {
        m_glErrorDetected = true;
        std::cerr << "[RoomCraft3D][ERROR] OpenGL error code: 0x" << std::hex << err << std::dec << std::endl;
        err = glGetError();
    }

    glutSwapBuffers();

    // Nếu đang ở chế độ --smoke-test: kiểm tra resize thực tế rồi thoát an toàn
    if (m_config.smokeTestMode) {
        if (!m_smokeResizeTriggered && m_frameCount >= 3) {
            m_smokeResizeTriggered = true;
            glutReshapeWindow(960, 540);
        }
        if (m_frameCount >= m_config.smokeTestFrames && m_smokeResizeObserved) {
            glutLeaveMainLoop();
            return;
        }
        // Giới hạn tối đa tránh treo nếu WM không cấp phát resize đúng kích thước
        if (m_frameCount >= m_config.smokeTestFrames + 20) {
            glutLeaveMainLoop();
            return;
        }
    }

    glutPostRedisplay();
}

void Application::handleReshape(int width, int height)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    m_viewportWidth = width;
    m_viewportHeight = height;
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);

    if (m_imguiInitialized) {
        ImGui_ImplGLUT_ReshapeFunc(width, height);
    }

    if (m_config.smokeTestMode && m_smokeResizeTriggered && width == 960 && height == 540) {
        m_smokeResizeObserved = true;
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
        glutLeaveMainLoop();
    }
}

void Application::handleClose()
{
    // Callback này chạy TRƯỚC khi FreeGLUT hủy OpenGL context -> giải phóng tài nguyên GPU tại đây
    shutdownGraphics();
}

void Application::displayCallback()
{
    if (s_instance != nullptr) {
        s_instance->renderFrame();
    }
}

void Application::reshapeCallback(int width, int height)
{
    if (s_instance != nullptr) {
        s_instance->handleReshape(width, height);
    }
}

void Application::closeCallback()
{
    if (s_instance != nullptr) {
        s_instance->handleClose();
    }
}

void Application::keyboardCallback(unsigned char key, int x, int y)
{
    if (s_instance != nullptr) {
        s_instance->handleKeyboard(key, x, y);
    }
}

void Application::keyboardUpCallback(unsigned char key, int x, int y)
{
    if (s_instance != nullptr && s_instance->m_imguiInitialized) {
        ImGui_ImplGLUT_KeyboardUpFunc(key, x, y);
    }
}

void Application::specialCallback(int key, int x, int y)
{
    if (s_instance != nullptr && s_instance->m_imguiInitialized) {
        ImGui_ImplGLUT_SpecialFunc(key, x, y);
    }
}

void Application::specialUpCallback(int key, int x, int y)
{
    if (s_instance != nullptr && s_instance->m_imguiInitialized) {
        ImGui_ImplGLUT_SpecialUpFunc(key, x, y);
    }
}

void Application::mouseCallback(int button, int state, int x, int y)
{
    if (s_instance != nullptr && s_instance->m_imguiInitialized) {
        ImGui_ImplGLUT_MouseFunc(button, state, x, y);
    }
}

void Application::mouseWheelCallback(int wheel, int direction, int x, int y)
{
    if (s_instance != nullptr && s_instance->m_imguiInitialized) {
        ImGui_ImplGLUT_MouseWheelFunc(wheel, direction, x, y);
    }
}

void Application::motionCallback(int x, int y)
{
    if (s_instance != nullptr && s_instance->m_imguiInitialized) {
        ImGui_ImplGLUT_MotionFunc(x, y);
    }
}

void Application::passiveMotionCallback(int x, int y)
{
    if (s_instance != nullptr && s_instance->m_imguiInitialized) {
        ImGui_ImplGLUT_MotionFunc(x, y);
    }
}

} // namespace roomcraft::core
