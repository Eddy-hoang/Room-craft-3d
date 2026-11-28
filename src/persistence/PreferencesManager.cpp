#include "roomcraft/persistence/PreferencesManager.hpp"

#include "roomcraft/editor/EditorLayout.hpp"
#include "roomcraft/persistence/Json.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace roomcraft::persistence {

namespace {

std::atomic<std::uint64_t> g_prefTempSeq{1};

float sanitizeFloat(float v, float fallback, float minV, float maxV)
{
    if (!std::isfinite(v)) {
        return fallback;
    }
    return std::clamp(v, minV, maxV);
}

} // namespace

void UserPreferences::clampToValidRanges()
{
    version = 1;
    leftPanelWidth = sanitizeFloat(leftPanelWidth, 272.0f,
                                   editor::EditorLayout::kMinLeftWidth,
                                   editor::EditorLayout::kMaxLeftWidth);
    rightPanelWidth = sanitizeFloat(rightPanelWidth, 316.0f,
                                    editor::EditorLayout::kMinRightWidth,
                                    editor::EditorLayout::kMaxRightWidth);
    hierarchyHeightRatio = sanitizeFloat(hierarchyHeightRatio, 0.36f,
                                         editor::EditorLayout::kMinHierarchyRatio,
                                         editor::EditorLayout::kMaxHierarchyRatio);
    uiScale = sanitizeFloat(uiScale, 1.0f, 1.0f, 1.50f);

    // Chuẩn hóa danh sách Recent Projects: loại rỗng, loại trùng, giới hạn tối đa 8 mục
    std::vector<std::filesystem::path> cleaned;
    cleaned.reserve(std::min(recentProjects.size(), PreferencesManager::kMaxRecentProjects));
    for (const auto& p : recentProjects) {
        if (p.empty()) {
            continue;
        }
        if (std::find(cleaned.begin(), cleaned.end(), p) == cleaned.end()) {
            cleaned.push_back(p);
            if (cleaned.size() >= PreferencesManager::kMaxRecentProjects) {
                break;
            }
        }
    }
    recentProjects = std::move(cleaned);
}

std::filesystem::path PreferencesManager::getDefaultPreferencesPath()
{
#ifdef _WIN32
    std::wstring buf(1024, L'\0');
    DWORD len = GetEnvironmentVariableW(L"APPDATA", buf.data(), static_cast<DWORD>(buf.size()));
    if (len > 0 && len < buf.size()) {
        buf.resize(len);
        return std::filesystem::path(buf) / L"RoomCraft3D" / L"preferences.json";
    }
    len = GetEnvironmentVariableW(L"LOCALAPPDATA", buf.data(), static_cast<DWORD>(buf.size()));
    if (len > 0 && len < buf.size()) {
        buf.resize(len);
        return std::filesystem::path(buf) / L"RoomCraft3D" / L"preferences.json";
    }
#else
    if (const char* home = std::getenv("HOME")) {
        return std::filesystem::path(home) / ".config" / "RoomCraft3D" / "preferences.json";
    }
#endif
    std::error_code ec;
    const auto tmp = std::filesystem::temp_directory_path(ec);
    return (ec ? std::filesystem::path(".") : tmp) / "RoomCraft3D" / "preferences.json";
}

bool PreferencesManager::loadFromFile(const std::filesystem::path& path,
                                      UserPreferences& outPrefs,
                                      std::string* outError)
{
    outPrefs = UserPreferences{};
    outPrefs.clampToValidRanges();

    std::error_code ec;
    if (path.empty() || !std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec)) {
        if (outError != nullptr) {
            *outError = "Preferences file does not exist; using defaults.";
        }
        return false;
    }

    const auto fsize = std::filesystem::file_size(path, ec);
    if (ec || fsize == 0 || fsize > 1024u * 1024u) {
        if (outError != nullptr) {
            *outError = "Preferences file is empty or exceeds 1 MB limit; using defaults.";
        }
        return false;
    }

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        if (outError != nullptr) {
            *outError = "Unable to open preferences file for reading.";
        }
        return false;
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();

    JsonValue root;
    std::string parseErr;
    if (!parseJson(oss.str(), root, &parseErr, 1024u * 1024u, 16) || !root.isObject()) {
        if (outError != nullptr) {
            *outError = "Corrupted preferences file (" + parseErr + "); falling back to defaults.";
        }
        return false;
    }

    auto readFloat = [&](const char* key, float& dst) {
        if (const auto* v = root.find(key); v != nullptr && v->isNumber()) {
            dst = static_cast<float>(v->asDouble());
        }
    };
    auto readBool = [&](const char* key, bool& dst) {
        if (const auto* v = root.find(key); v != nullptr && v->isBool()) {
            dst = v->asBool();
        }
    };

    readFloat("leftPanelWidth", outPrefs.leftPanelWidth);
    readFloat("rightPanelWidth", outPrefs.rightPanelWidth);
    readFloat("hierarchyHeightRatio", outPrefs.hierarchyHeightRatio);
    readFloat("uiScale", outPrefs.uiScale);
    readBool("showLeftPanel", outPrefs.showLeftPanel);
    readBool("showRightPanel", outPrefs.showRightPanel);
    readBool("libraryGridView", outPrefs.libraryGridView);

    if (const auto* rec = root.find("recentProjects"); rec != nullptr && rec->isArray()) {
        for (const auto& item : rec->asArray()) {
            if (item.isString() && !item.asString().empty()) {
                outPrefs.recentProjects.push_back(std::filesystem::u8path(item.asString()));
            }
        }
    }

    outPrefs.clampToValidRanges();
    return true;
}

bool PreferencesManager::saveToFile(const std::filesystem::path& path,
                                    const UserPreferences& prefs,
                                    std::string* outError)
{
    if (path.empty()) {
        if (outError != nullptr) *outError = "Preferences path is empty.";
        return false;
    }

    UserPreferences clean = prefs;
    clean.clampToValidRanges();

    std::error_code ec;
    const auto parent = path.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent, ec)) {
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            if (outError != nullptr) *outError = "Failed to create preferences directory: " + ec.message();
            return false;
        }
    }

    JsonValue root = JsonValue::makeObject();
    root.set("version", clean.version);
    root.set("leftPanelWidth", clean.leftPanelWidth);
    root.set("rightPanelWidth", clean.rightPanelWidth);
    root.set("hierarchyHeightRatio", clean.hierarchyHeightRatio);
    root.set("uiScale", clean.uiScale);
    root.set("showLeftPanel", clean.showLeftPanel);
    root.set("showRightPanel", clean.showRightPanel);
    root.set("libraryGridView", clean.libraryGridView);

    JsonValue recArr = JsonValue::makeArray();
    for (const auto& p : clean.recentProjects) {
        const auto u8 = p.u8string();
        recArr.pushBack(JsonValue(std::string(u8.begin(), u8.end())));
    }
    root.set("recentProjects", std::move(recArr));

    const std::string payload = root.dump(2);
    std::filesystem::path tempPath = path;
    tempPath += L".tmp." + std::to_wstring(g_prefTempSeq.fetch_add(1, std::memory_order_relaxed));

    {
        std::ofstream ofs(tempPath, std::ios::binary | std::ios::trunc);
        if (!ofs.is_open()) {
            if (outError != nullptr) *outError = "Unable to open temporary preferences file for writing.";
            return false;
        }
        ofs.write(payload.data(), static_cast<std::streamsize>(payload.size()));
        ofs.flush();
        if (!ofs.good()) {
            ofs.close();
            std::filesystem::remove(tempPath, ec);
            if (outError != nullptr) *outError = "Failed to write temporary preferences file.";
            return false;
        }
    }

#ifdef _WIN32
    if (!MoveFileExW(tempPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(tempPath, ec);
        if (outError != nullptr) *outError = "Failed to replace preferences file.";
        return false;
    }
#else
    std::filesystem::rename(tempPath, path, ec);
    if (ec) {
        std::filesystem::remove(tempPath, ec);
        if (outError != nullptr) *outError = "Failed to replace preferences file: " + ec.message();
        return false;
    }
#endif
    return true;
}

} // namespace roomcraft::persistence
