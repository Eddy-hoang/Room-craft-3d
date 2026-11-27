#pragma once

#include <array>
#include <filesystem>
#include <string>

namespace roomcraft::core {

// Cấu hình khởi tạo cho ứng dụng RoomCraft 3D (Milestone M0)
struct ApplicationConfig {
    int width = 1280;
    int height = 720;
    std::string title = "RoomCraft 3D - M0 Foundation (OpenGL 3.3 Core)";
    bool smokeTestMode = false;
    int smokeTestFrames = 10;
};

// Lớp Application quản lý vòng đời cửa sổ FreeGLUT, OpenGL 3.3 Core Context,
// bộ nạp GLAD, pipeline render tam giác cơ bản (VAO/VBO/GLSL) và tích hợp Dear ImGui.
class Application {
public:
    explicit Application(ApplicationConfig config);
    ~Application();

    // Không cho phép sao chép vì lớp sở hữu tài nguyên GPU (VAO, VBO, Shader) và ImGui context
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Khởi tạo cửa sổ, OpenGL 3.3 Core, GLAD, shader/mesh M0 và Dear ImGui
    bool initialize(int& argc, char** argv);

    // Chạy vòng lặp sự kiện FreeGLUT; trả về mã thoát (0 nếu thành công)
    int run();

private:
    // Các bước khởi tạo con
    bool initWindowAndContext(int& argc, char** argv);
    bool initGladAndLogGpuInfo();
    bool initTrianglePipeline();
    bool initImGui();

    // Giải phóng an toàn tài nguyên GPU & ImGui trước khi OpenGL context bị hủy
    void shutdownGraphics();

    // Tiện ích đọc và biên dịch shader GLSL
    std::filesystem::path resolveAssetPath(const std::filesystem::path& relativePath) const;
    bool loadTextFile(const std::filesystem::path& filePath, std::string& outContent) const;
    unsigned int compileShader(unsigned int shaderType, const std::string& source, const std::string& debugName) const;

    // Vòng lặp render và xử lý sự kiện của instance hiện tại
    void renderFrame();
    void handleReshape(int width, int height);
    void handleKeyboard(unsigned char key, int x, int y);
    void handleClose();

    // Các callback tĩnh cho FreeGLUT chuyển tiếp về instance đang hoạt động
    static void displayCallback();
    static void reshapeCallback(int width, int height);
    static void closeCallback();
    static void keyboardCallback(unsigned char key, int x, int y);
    static void keyboardUpCallback(unsigned char key, int x, int y);
    static void specialCallback(int key, int x, int y);
    static void specialUpCallback(int key, int x, int y);
    static void mouseCallback(int button, int state, int x, int y);
    static void mouseWheelCallback(int wheel, int direction, int x, int y);
    static void motionCallback(int x, int y);
    static void passiveMotionCallback(int x, int y);

private:
    static Application* s_instance;

    ApplicationConfig m_config;
    std::filesystem::path m_executableDir;

    int m_windowId = 0;
    int m_viewportWidth = 1280;
    int m_viewportHeight = 720;

    // Thông tin GPU và OpenGL lấy từ driver sau khi GLAD nạp xong
    std::string m_gpuVendor;
    std::string m_gpuRenderer;
    std::string m_glVersion;
    std::string m_glslVersion;
    std::string m_glProfile;
    int m_glMajorVersion = 0;
    int m_glMinorVersion = 0;

    // Tài nguyên OpenGL của tam giác kiểm chứng M0
    unsigned int m_shaderProgram = 0;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;

    // Trạng thái vòng đời và UI thử nghiệm M0
    bool m_graphicsInitialized = false;
    bool m_imguiInitialized = false;
    bool m_showTriangle = true;
    std::array<float, 4> m_clearColor = {0.094f, 0.102f, 0.125f, 1.0f}; // #181A20 theo Design System

    // Bộ đếm phục vụ thống kê FPS và smoke test tự động
    int m_frameCount = 0;
    int m_lastFpsTimeMs = 0;
    int m_framesSinceLastFps = 0;
    float m_currentFps = 0.0f;
    bool m_smokeResizeTriggered = false;
    bool m_smokeResizeObserved = false;
    bool m_smokeTrianglePixelVerified = false;
    std::array<unsigned char, 4> m_smokeSampledPixel = {0, 0, 0, 0};
    bool m_glErrorDetected = false;
};

} // namespace roomcraft::core
