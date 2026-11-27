#include "roomcraft/core/InputManager.hpp"

#include <cctype>
#include <cmath>

namespace roomcraft::core {

InputManager::InputManager()
{
    resetAll();
}

unsigned char InputManager::normalizeAsciiKey(unsigned char key)
{
    if (key >= 'A' && key <= 'Z') {
        return static_cast<unsigned char>(std::tolower(key));
    }
    return key;
}

void InputManager::onMouseMove(float x, float y)
{
    if (!std::isfinite(x) || !std::isfinite(y)) {
        return;
    }
    const glm::vec2 next(x, y);
    if (m_hasMousePosition) {
        m_mouseDelta += (next - m_mousePos);
    }
    m_mousePos = next;
    m_hasMousePosition = true;
}

void InputManager::onMouseButton(MouseButton button, bool down, float x, float y)
{
    onMouseMove(x, y);
    const int idx = static_cast<int>(button);
    if (idx < 0 || idx >= 3) {
        return;
    }
    if (down) {
        if (!m_mouseHeld[static_cast<std::size_t>(idx)]) {
            m_mousePressed[static_cast<std::size_t>(idx)] = true;
        }
        m_mouseHeld[static_cast<std::size_t>(idx)] = true;
    } else {
        if (m_mouseHeld[static_cast<std::size_t>(idx)]) {
            m_mouseReleased[static_cast<std::size_t>(idx)] = true;
        }
        m_mouseHeld[static_cast<std::size_t>(idx)] = false;
    }
}

void InputManager::onMouseWheel(float delta)
{
    if (std::isfinite(delta)) {
        m_wheelDelta += delta;
    }
}

void InputManager::onKeyDown(unsigned char key, bool ctrlDown, bool shiftDown, bool altDown)
{
    setModifiers(ctrlDown, shiftDown, altDown);
    const unsigned char k = normalizeAsciiKey(key);
    // Chỉ đặt cờ Pressed khi phím chuyển từ thả sang nhấn (chống lặp auto-repeat của HĐH)
    if (!m_keyHeld[k]) {
        m_keyPressed[k] = true;
    }
    m_keyHeld[k] = true;
}

void InputManager::onKeyUp(unsigned char key)
{
    const unsigned char k = normalizeAsciiKey(key);
    if (m_keyHeld[k]) {
        m_keyReleased[k] = true;
    }
    m_keyHeld[k] = false;
}

void InputManager::onSpecialKeyDown(SpecialKey key, bool ctrlDown, bool shiftDown, bool altDown)
{
    setModifiers(ctrlDown, shiftDown, altDown);
    const auto idx = static_cast<std::size_t>(key);
    if (idx == 0 || idx >= m_specialHeld.size()) {
        return;
    }
    if (!m_specialHeld[idx]) {
        m_specialPressed[idx] = true;
    }
    m_specialHeld[idx] = true;
}

void InputManager::onSpecialKeyUp(SpecialKey key)
{
    const auto idx = static_cast<std::size_t>(key);
    if (idx == 0 || idx >= m_specialHeld.size()) {
        return;
    }
    m_specialHeld[idx] = false;
}

void InputManager::setModifiers(bool ctrlDown, bool shiftDown, bool altDown)
{
    m_ctrlDown = ctrlDown;
    m_shiftDown = shiftDown;
    m_altDown = altDown;
}

void InputManager::endFrame()
{
    m_mouseDelta = glm::vec2(0.0f, 0.0f);
    m_wheelDelta = 0.0f;
    m_mousePressed.fill(false);
    m_mouseReleased.fill(false);
    m_keyPressed.fill(false);
    m_keyReleased.fill(false);
    m_specialPressed.fill(false);
}

void InputManager::resetAll()
{
    m_mouseDelta = glm::vec2(0.0f, 0.0f);
    m_wheelDelta = 0.0f;
    m_hasMousePosition = false;
    m_mouseHeld.fill(false);
    m_mousePressed.fill(false);
    m_mouseReleased.fill(false);
    m_keyHeld.fill(false);
    m_keyPressed.fill(false);
    m_keyReleased.fill(false);
    m_specialHeld.fill(false);
    m_specialPressed.fill(false);
    m_ctrlDown = false;
    m_shiftDown = false;
    m_altDown = false;
}

bool InputManager::isMouseHeld(MouseButton button) const
{
    const int idx = static_cast<int>(button);
    return (idx >= 0 && idx < 3) ? m_mouseHeld[static_cast<std::size_t>(idx)] : false;
}

bool InputManager::isMousePressed(MouseButton button) const
{
    const int idx = static_cast<int>(button);
    return (idx >= 0 && idx < 3) ? m_mousePressed[static_cast<std::size_t>(idx)] : false;
}

bool InputManager::isMouseReleased(MouseButton button) const
{
    const int idx = static_cast<int>(button);
    return (idx >= 0 && idx < 3) ? m_mouseReleased[static_cast<std::size_t>(idx)] : false;
}

bool InputManager::isKeyHeld(unsigned char key) const
{
    return m_keyHeld[normalizeAsciiKey(key)];
}

bool InputManager::isKeyPressed(unsigned char key) const
{
    return m_keyPressed[normalizeAsciiKey(key)];
}

bool InputManager::isKeyReleased(unsigned char key) const
{
    return m_keyReleased[normalizeAsciiKey(key)];
}

bool InputManager::isSpecialKeyHeld(SpecialKey key) const
{
    const auto idx = static_cast<std::size_t>(key);
    return (idx > 0 && idx < m_specialHeld.size()) ? m_specialHeld[idx] : false;
}

bool InputManager::isSpecialKeyPressed(SpecialKey key) const
{
    const auto idx = static_cast<std::size_t>(key);
    return (idx > 0 && idx < m_specialPressed.size()) ? m_specialPressed[idx] : false;
}

} // namespace roomcraft::core
