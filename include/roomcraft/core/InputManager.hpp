#pragma once

#include <glm/vec2.hpp>

#include <array>
#include <cstdint>

namespace roomcraft::core {

enum class MouseButton : int {
    Left = 0,
    Middle = 1,
    Right = 2,
    Count = 3
};

enum class SpecialKey : int {
    None = 0,
    Home = 1,
    End = 2,
    Up = 3,
    Down = 4,
    Left = 5,
    Right = 6,
    Count = 7
};

// Lớp InputManager thuần CPU quản lý toàn bộ trạng thái chuột và bàn phím theo khung hình:
// - Phân biệt rạch ròi sự kiện nhấn một lần (Pressed/Released) với trạng thái đang giữ (Held),
//   ngăn lặp phím Delete / Ctrl+D mỗi frame khi giữ phím.
// - Hoạt động độc lập khỏi FreeGLUT (nhận sự kiện chuyển tiếp từ Window/Application),
//   nên kiểm thử được trực tiếp bằng CPU unit tests.
class InputManager {
public:
    InputManager();

    // Nhận sự kiện từ FreeGLUT / test harness
    void onMouseMove(float x, float y);
    void onMouseButton(MouseButton button, bool down, float x, float y);
    void onMouseWheel(float delta);
    void onKeyDown(unsigned char key, bool ctrlDown, bool shiftDown, bool altDown);
    void onKeyUp(unsigned char key);
    void onSpecialKeyDown(SpecialKey key, bool ctrlDown, bool shiftDown, bool altDown);
    void onSpecialKeyUp(SpecialKey key);
    void setModifiers(bool ctrlDown, bool shiftDown, bool altDown);

    // Gọi vào cuối mỗi khung hình để xóa mouseDelta, wheelDelta và các cờ 1-frame (Pressed/Released)
    void endFrame();

    // Xóa sạch toàn bộ trạng thái khi mất focus cửa sổ hoặc chuyển workspace
    void resetAll();

    // Truy vấn chuột
    glm::vec2 getMousePosition() const { return m_mousePos; }
    glm::vec2 getMouseDelta() const { return m_mouseDelta; }
    float getWheelDelta() const { return m_wheelDelta; }
    bool isMouseHeld(MouseButton button) const;
    bool isMousePressed(MouseButton button) const;
    bool isMouseReleased(MouseButton button) const;

    // Truy vấn bàn phím (chữ cái tự chuẩn hóa về chữ thường)
    bool isKeyHeld(unsigned char key) const;
    bool isKeyPressed(unsigned char key) const;
    bool isKeyReleased(unsigned char key) const;

    bool isSpecialKeyHeld(SpecialKey key) const;
    bool isSpecialKeyPressed(SpecialKey key) const;

    bool isCtrlDown() const { return m_ctrlDown; }
    bool isShiftDown() const { return m_shiftDown; }
    bool isAltDown() const { return m_altDown; }

private:
    static unsigned char normalizeAsciiKey(unsigned char key);

private:
    glm::vec2 m_mousePos{0.0f, 0.0f};
    glm::vec2 m_mouseDelta{0.0f, 0.0f};
    bool m_hasMousePosition = false;
    float m_wheelDelta = 0.0f;

    std::array<bool, 3> m_mouseHeld{};
    std::array<bool, 3> m_mousePressed{};
    std::array<bool, 3> m_mouseReleased{};

    std::array<bool, 256> m_keyHeld{};
    std::array<bool, 256> m_keyPressed{};
    std::array<bool, 256> m_keyReleased{};

    std::array<bool, static_cast<std::size_t>(SpecialKey::Count)> m_specialHeld{};
    std::array<bool, static_cast<std::size_t>(SpecialKey::Count)> m_specialPressed{};

    bool m_ctrlDown = false;
    bool m_shiftDown = false;
    bool m_altDown = false;
};

} // namespace roomcraft::core
