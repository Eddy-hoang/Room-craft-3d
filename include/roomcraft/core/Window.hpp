#pragma once

#include <functional>
#include <string>

namespace roomcraft::core {

// Thông tin GPU và OpenGL context đọc trực tiếp từ driver sau khi GLAD nạp thành công
struct GpuInfo {
    std::string vendor;
    std::string renderer;
    std::string glVersion;
    std::string glslVersion;
    std::string profile;
    int majorVersion = 0;
    int minorVersion = 0;
};

// Cấu hình cửa sổ FreeGLUT
struct WindowConfig {
    int width = 1280;
    int height = 720;
    std::string title = "RoomCraft 3D";
};

// Tập callback sự kiện cửa sổ được tách độc lập khỏi Application
struct WindowCallbacks {
    std::function<void()> onDisplay;
    std::function<void(int, int)> onReshape;
    std::function<void()> onClose;
    // Trả về true nếu cho phép đóng cửa sổ ngay; trả về false để chặn nút X của hệ điều hành
    // và hiển thị hộp thoại Save / Discard / Cancel cho dự án có thay đổi chưa lưu (M8.9).
    std::function<bool()> onCloseRequest;
    std::function<void(unsigned char, int, int)> onKeyboard;
    std::function<void(unsigned char, int, int)> onKeyboardUp;
    std::function<void(int, int, int)> onSpecial;
    std::function<void(int, int, int)> onSpecialUp;
    std::function<void(int, int, int, int)> onMouse;
    std::function<void(int, int, int, int)> onMouseWheel;
    std::function<void(int, int)> onMotion;
    std::function<void(int, int)> onPassiveMotion;
};

// Lớp Window bao gói FreeGLUT và khởi tạo OpenGL 3.3 Core Profile + GLAD.
// Lưu ý: Trong M1 chỉ cho phép tối đa một instance Window đang hoạt động tại một thời điểm.
class Window {
public:
    explicit Window(WindowConfig config);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Khởi tạo FreeGLUT, OpenGL 3.3 Core Context và GLAD
    bool initialize(int& argc, char** argv);

    // Đăng ký callback sự kiện trước khi vào vòng lặp chính
    void setCallbacks(WindowCallbacks callbacks);

    // Chạy vòng lặp sự kiện FreeGLUT (trả về an toàn khi cửa sổ đóng)
    void runMainLoop();

    // Điều khiển cửa sổ
    void requestClose();
    void setTitle(const std::string& title);
    void swapBuffers() const;
    void postRedisplay() const;
    void resize(int width, int height);

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getElapsedTimeMs() const;
    bool isInitialized() const { return m_initialized; }
    const GpuInfo& getGpuInfo() const { return m_gpuInfo; }

private:
    bool initGladAndQueryGpu();

    // Các callback tĩnh chuyển tiếp từ FreeGLUT về instance Window duy nhất
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
    static Window* s_activeInstance;

    WindowConfig m_config;
    WindowCallbacks m_callbacks;
    GpuInfo m_gpuInfo;

    int m_windowId = 0;
    int m_width = 1280;
    int m_height = 720;
    bool m_initialized = false;
    void* m_nativeHwnd = nullptr;
    void* m_prevWndProc = nullptr;
    std::string m_currentTitle;
};

} // namespace roomcraft::core
