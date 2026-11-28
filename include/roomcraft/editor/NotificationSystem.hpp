#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace roomcraft::editor {

enum class NotificationLevel {
    Info = 0,
    Success = 1,
    Warning = 2,
    Error = 3
};

struct ToastNotification {
    std::uint64_t id = 0;
    NotificationLevel level = NotificationLevel::Info;
    std::string message;
    float remainingSeconds = 3.0f;
    float totalSeconds = 3.0f;
};

// Hệ thống thông báo toast ngắn gọn không chặn thao tác (thuần CPU, dễ kiểm thử).
class NotificationSystem {
public:
    static constexpr std::size_t kMaxVisible = 4;

    void push(NotificationLevel level, const std::string& message, float durationSeconds = 3.2f)
    {
        if (message.empty()) {
            return;
        }
        ToastNotification item;
        item.id = ++m_nextId;
        item.level = level;
        item.message = message;
        item.totalSeconds = std::clamp(durationSeconds, 0.5f, 10.0f);
        item.remainingSeconds = item.totalSeconds;
        m_items.push_back(item);
        while (m_items.size() > kMaxVisible) {
            m_items.erase(m_items.begin());
        }
    }

    void update(float deltaSeconds)
    {
        if (deltaSeconds <= 0.0f) {
            return;
        }
        for (auto& item : m_items) {
            item.remainingSeconds -= deltaSeconds;
        }
        m_items.erase(std::remove_if(m_items.begin(), m_items.end(),
                                     [](const ToastNotification& n) { return n.remainingSeconds <= 0.0f; }),
                      m_items.end());
    }

    void clear() { m_items.clear(); }

    const std::vector<ToastNotification>& getActive() const { return m_items; }
    bool empty() const { return m_items.empty(); }

private:
    std::vector<ToastNotification> m_items;
    std::uint64_t m_nextId = 0;
};

} // namespace roomcraft::editor
