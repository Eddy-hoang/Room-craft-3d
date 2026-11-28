#include "roomcraft/core/Window.hpp"

// Luôn include <glad/glad.h> trước <GL/freeglut.h> để tránh xung đột với <GL/gl.h>
#include <glad/glad.h>
#include <GL/freeglut.h>

#include <iostream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace roomcraft::core {

Window* Window::s_activeInstance = nullptr;

#ifdef _WIN32
namespace {

LRESULT CALLBACK roomcraftWindowCloseSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC origProc = reinterpret_cast<WNDPROC>(GetPropW(hwnd, L"RoomCraftPrevWndProc"));
    if (msg == WM_CLOSE) {
        auto* fnPtr = reinterpret_cast<std::function<bool()>*>(GetPropW(hwnd, L"RoomCraftCloseReqFn"));
        if (fnPtr != nullptr && *fnPtr) {
            if (!(*fnPtr)()) {
                // Chặn đóng cửa sổ hệ điều hành để hiển thị hộp thoại Save / Discard / Cancel
                return 0;
            }
        }
    }
    if (origProc != nullptr) {
        return CallWindowProcW(origProc, hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace
#endif

Window::Window(WindowConfig config)
    : m_config(std::move(config))
    , m_width(m_config.width)
    , m_height(m_config.height)
{
}

Window::~Window()
{
#ifdef _WIN32
    if (m_nativeHwnd != nullptr && m_prevWndProc != nullptr) {
        HWND hwnd = static_cast<HWND>(m_nativeHwnd);
        if (IsWindow(hwnd)) {
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_prevWndProc));
            RemovePropW(hwnd, L"RoomCraftPrevWndProc");
            RemovePropW(hwnd, L"RoomCraftCloseReqFn");
        }
        m_nativeHwnd = nullptr;
        m_prevWndProc = nullptr;
    }
#endif
    if (s_activeInstance == this) {
        s_activeInstance = nullptr;
    }
    m_initialized = false;
}

bool Window::initialize(int& argc, char** argv)
{
    if (m_initialized) {
        return true;
    }
    if (s_activeInstance != nullptr && s_activeInstance != this) {
        std::cerr << "[RoomCraft3D][ERROR] Multiple Window instances are not supported." << std::endl;
        return false;
    }

    s_activeInstance = this;

    glutInit(&argc, argv);

    // Yêu cầu tường minh OpenGL 3.3 Core Profile (không dùng fixed-function pipeline cũ)
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    // Cho phép glutMainLoop() trả về thay vì gọi exit() đột ngột khi đóng cửa sổ
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(m_config.width, m_config.height);
    glutInitWindowPosition(100, 100);

    m_windowId = glutCreateWindow(m_config.title.c_str());
    if (m_windowId <= 0) {
        std::cerr << "[RoomCraft3D][ERROR] Failed to create FreeGLUT window." << std::endl;
        s_activeInstance = nullptr;
        return false;
    }

    std::cout << "[RoomCraft3D] FreeGLUT window created ("
              << m_config.width << "x" << m_config.height << ")." << std::endl;

    if (!initGladAndQueryGpu()) {
        s_activeInstance = nullptr;
        return false;
    }

    // Đăng ký hệ thống callback FreeGLUT
    glutDisplayFunc(&Window::displayCallback);
    glutReshapeFunc(&Window::reshapeCallback);
    glutCloseFunc(&Window::closeCallback);
    glutKeyboardFunc(&Window::keyboardCallback);
    glutKeyboardUpFunc(&Window::keyboardUpCallback);
    glutSpecialFunc(&Window::specialCallback);
    glutSpecialUpFunc(&Window::specialUpCallback);
    glutMouseFunc(&Window::mouseCallback);
    glutMouseWheelFunc(&Window::mouseWheelCallback);
    glutMotionFunc(&Window::motionCallback);
    glutPassiveMotionFunc(&Window::passiveMotionCallback);

    m_currentTitle = m_config.title;
#ifdef _WIN32
    HWND hwnd = WindowFromDC(wglGetCurrentDC());
    if (hwnd == nullptr) {
        hwnd = GetActiveWindow();
    }
    if (hwnd != nullptr) {
        WNDPROC prev = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&roomcraftWindowCloseSubclassProc)));
        if (prev != nullptr) {
            SetPropW(hwnd, L"RoomCraftPrevWndProc", reinterpret_cast<HANDLE>(prev));
            SetPropW(hwnd, L"RoomCraftCloseReqFn", reinterpret_cast<HANDLE>(&m_callbacks.onCloseRequest));
            m_nativeHwnd = hwnd;
            m_prevWndProc = reinterpret_cast<void*>(prev);
        }
    }
#endif

    m_initialized = true;
    return true;
}

bool Window::initGladAndQueryGpu()
{
    if (!gladLoadGL()) {
        std::cerr << "[RoomCraft3D][ERROR] Failed to initialize GLAD OpenGL loader." << std::endl;
        return false;
    }

    glGetIntegerv(GL_MAJOR_VERSION, &m_gpuInfo.majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &m_gpuInfo.minorVersion);

    GLint profileMask = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profileMask);
    if ((profileMask & GL_CONTEXT_CORE_PROFILE_BIT) != 0) {
        m_gpuInfo.profile = "Core Profile";
    } else if ((profileMask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) != 0) {
        m_gpuInfo.profile = "Compatibility Profile";
    } else {
        m_gpuInfo.profile = "Unknown Profile";
    }

    const auto toString = [](const GLubyte* raw) -> std::string {
        return raw != nullptr ? reinterpret_cast<const char*>(raw) : "Unknown";
    };

    m_gpuInfo.vendor = toString(glGetString(GL_VENDOR));
    m_gpuInfo.renderer = toString(glGetString(GL_RENDERER));
    m_gpuInfo.glVersion = toString(glGetString(GL_VERSION));
    m_gpuInfo.glslVersion = toString(glGetString(GL_SHADING_LANGUAGE_VERSION));

    std::cout << "================ OpenGL Environment Info ================" << std::endl;
    std::cout << "GPU Vendor     : " << m_gpuInfo.vendor << std::endl;
    std::cout << "GPU Renderer   : " << m_gpuInfo.renderer << std::endl;
    std::cout << "OpenGL Version : " << m_gpuInfo.glVersion
              << " (Parsed: " << m_gpuInfo.majorVersion << "." << m_gpuInfo.minorVersion << ")" << std::endl;
    std::cout << "OpenGL Profile : " << m_gpuInfo.profile << std::endl;
    std::cout << "GLSL Version   : " << m_gpuInfo.glslVersion << std::endl;
    std::cout << "=========================================================" << std::endl;

    if (m_gpuInfo.majorVersion < 3 || (m_gpuInfo.majorVersion == 3 && m_gpuInfo.minorVersion < 3)) {
        std::cerr << "[RoomCraft3D][ERROR] OpenGL 3.3 or higher is required, got "
                  << m_gpuInfo.majorVersion << "." << m_gpuInfo.minorVersion << std::endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, m_width, m_height);
    return true;
}

void Window::setCallbacks(WindowCallbacks callbacks)
{
    m_callbacks = std::move(callbacks);
}

void Window::runMainLoop()
{
    if (!m_initialized) {
        return;
    }
    std::cout << "[RoomCraft3D] Entering FreeGLUT main loop..." << std::endl;
    glutMainLoop();
    std::cout << "[RoomCraft3D] Exited FreeGLUT main loop cleanly." << std::endl;
}

void Window::requestClose()
{
    if (m_initialized) {
        glutLeaveMainLoop();
    }
}

void Window::setTitle(const std::string& title)
{
    if (m_initialized && title != m_currentTitle) {
        m_currentTitle = title;
        glutSetWindowTitle(title.c_str());
    }
}

void Window::swapBuffers() const
{
    glutSwapBuffers();
}

void Window::postRedisplay() const
{
    glutPostRedisplay();
}

void Window::resize(int width, int height)
{
    if (m_initialized && width > 0 && height > 0) {
        glutReshapeWindow(width, height);
    }
}

int Window::getElapsedTimeMs() const
{
    return glutGet(GLUT_ELAPSED_TIME);
}

void Window::displayCallback()
{
    if (s_activeInstance != nullptr && s_activeInstance->m_callbacks.onDisplay) {
        s_activeInstance->m_callbacks.onDisplay();
    }
}

void Window::reshapeCallback(int width, int height)
{
    if (s_activeInstance == nullptr) {
        return;
    }
    if (width > 0 && height > 0) {
        s_activeInstance->m_width = width;
        s_activeInstance->m_height = height;
    }
    if (s_activeInstance->m_callbacks.onReshape) {
        s_activeInstance->m_callbacks.onReshape(width, height);
    }
}

void Window::closeCallback()
{
    if (s_activeInstance != nullptr && s_activeInstance->m_callbacks.onClose) {
        s_activeInstance->m_callbacks.onClose();
    }
}

void Window::keyboardCallback(unsigned char key, int x, int y)
{
    if (s_activeInstance != nullptr && s_activeInstance->m_callbacks.onKeyboard) {
        s_activeInstance->m_callbacks.onKeyboard(key, x, y);
    }
}

void Window::keyboardUpCallback(unsigned char key, int x, int y)
{
    if (s_activeInstance != nullptr && s_activeInstance->m_callbacks.onKeyboardUp) {
        s_activeInstance->m_callbacks.onKeyboardUp(key, x, y);
    }
}

void Window::specialCallback(int key, int x, int y)
{
    if (s_activeInstance != nullptr && s_activeInstance->m_callbacks.onSpecial) {
        s_activeInstance->m_callbacks.onSpecial(key, x, y);
    }
}

void Window::specialUpCallback(int key, int x, int y)
{
    if (s_activeInstance != nullptr && s_activeInstance->m_callbacks.onSpecialUp) {
        s_activeInstance->m_callbacks.onSpecialUp(key, x, y);
    }
}

void Window::mouseCallback(int button, int state, int x, int y)
{
    if (s_activeInstance != nullptr && s_activeInstance->m_callbacks.onMouse) {
        s_activeInstance->m_callbacks.onMouse(button, state, x, y);
    }
}

void Window::mouseWheelCallback(int wheel, int direction, int x, int y)
{
    if (s_activeInstance != nullptr && s_activeInstance->m_callbacks.onMouseWheel) {
        s_activeInstance->m_callbacks.onMouseWheel(wheel, direction, x, y);
    }
}

void Window::motionCallback(int x, int y)
{
    if (s_activeInstance != nullptr && s_activeInstance->m_callbacks.onMotion) {
        s_activeInstance->m_callbacks.onMotion(x, y);
    }
}

void Window::passiveMotionCallback(int x, int y)
{
    if (s_activeInstance != nullptr && s_activeInstance->m_callbacks.onPassiveMotion) {
        s_activeInstance->m_callbacks.onPassiveMotion(x, y);
    }
}

} // namespace roomcraft::core
