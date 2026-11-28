#include "roomcraft/persistence/ProjectManager.hpp"

#include "roomcraft/persistence/Json.hpp"
#include "roomcraft/persistence/ProjectSerializer.hpp"
#include "roomcraft/persistence/ProjectValidator.hpp"

#include <algorithm>
#include <atomic>
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

std::atomic<std::uint64_t> g_tempCounter{1};

std::filesystem::path makeTempPathInSameDirectory(const std::filesystem::path& destPath)
{
    const std::uint64_t seq = g_tempCounter.fetch_add(1, std::memory_order_relaxed);
    std::filesystem::path temp = destPath;
    temp += L".tmp." + std::to_wstring(seq);
    return temp;
}

#ifdef _WIN32
std::string formatWin32Error(DWORD code)
{
    if (code == ERROR_ACCESS_DENIED) {
        return "Access denied (file or directory may be read-only or protected).";
    }
    if (code == ERROR_SHARING_VIOLATION || code == ERROR_LOCK_VIOLATION) {
        return "File is currently locked by another process.";
    }
    if (code == ERROR_DISK_FULL || code == ERROR_HANDLE_DISK_FULL) {
        return "Insufficient disk space.";
    }
    if (code == ERROR_PATH_NOT_FOUND || code == ERROR_FILE_NOT_FOUND) {
        return "Destination path does not exist.";
    }
    return "Windows I/O error code " + std::to_string(static_cast<unsigned long>(code)) + ".";
}
#endif

} // namespace

std::string ProjectManager::deriveProjectNameFromPath(const std::filesystem::path& path)
{
    if (path.empty()) {
        return "Untitled Room";
    }
    try {
        const auto stem = path.stem().u8string();
        std::string name(stem.begin(), stem.end());
        name = scene::Scene::normalizeObjectName(name);
        return name.empty() ? "Untitled Room" : name;
    } catch (...) {
        return "Untitled Room";
    }
}

bool ProjectManager::writeProjectFileSafe(const std::filesystem::path& destinationPath,
                                          const ProjectData& data,
                                          const assets::FurnitureCatalog& catalog,
                                          std::string* outError)
{
    auto fail = [&](const std::string& msg) {
        if (outError != nullptr) *outError = msg;
        return false;
    };

    if (destinationPath.empty()) {
        return fail("Destination file path is empty.");
    }

    // 1. Xác thực toàn bộ dữ liệu trước khi chạm tới hệ thống tệp
    std::string valErr;
    if (!ProjectValidator::validate(data, catalog, &valErr)) {
        return fail("Project validation failed before save: " + valErr);
    }

    // 2. Serialize JSON trong bộ nhớ trước
    std::string jsonText;
    std::string serErr;
    if (!ProjectSerializer::toJsonString(data, jsonText, &serErr)) {
        return fail("Serialization failed: " + serErr);
    }

    // 3. Kiểm tra thư mục đích tồn tại và tệp đích không phải thư mục / read-only
    std::error_code ec;
    const std::filesystem::path parentDir = destinationPath.parent_path();
    if (!parentDir.empty() && !std::filesystem::exists(parentDir, ec)) {
        return fail("Destination directory does not exist.");
    }
    if (std::filesystem::is_directory(destinationPath, ec)) {
        return fail("Destination path refers to a directory, not a file.");
    }

#ifdef _WIN32
    const DWORD existingAttrs = GetFileAttributesW(destinationPath.c_str());
    if (existingAttrs != INVALID_FILE_ATTRIBUTES && (existingAttrs & FILE_ATTRIBUTE_READONLY) != 0) {
        return fail("Destination file is read-only and cannot be overwritten.");
    }
#endif

    // 4. Ghi ra tệp tạm trong CÙNG thư mục đích (không cắt cụt hay xóa tệp đích cũ)
    const std::filesystem::path tempPath = makeTempPathInSameDirectory(destinationPath);
    {
        std::ofstream ofs(tempPath, std::ios::binary | std::ios::trunc);
        if (!ofs.is_open()) {
            return fail("Unable to create temporary file in destination directory (permission denied or invalid path).");
        }
        ofs.write(jsonText.data(), static_cast<std::streamsize>(jsonText.size()));
        ofs.flush();
        if (!ofs.good()) {
            ofs.close();
            std::filesystem::remove(tempPath, ec);
            return fail("Failed while writing project data to temporary file.");
        }
        ofs.close();
        if (ofs.fail()) {
            std::filesystem::remove(tempPath, ec);
            return fail("Failed while closing temporary project file.");
        }
    }

    // Kiểm tra kích thước tệp tạm khớp chính xác số byte đã ghi
    const auto writtenSize = std::filesystem::file_size(tempPath, ec);
    if (ec || writtenSize != jsonText.size()) {
        std::filesystem::remove(tempPath, ec);
        return fail("Temporary file size verification failed.");
    }

    // 5. Thay thế tệp đích nguyên tử trên Windows (MoveFileExW) hoặc POSIX rename
#ifdef _WIN32
    if (!MoveFileExW(tempPath.c_str(),
                     destinationPath.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        const DWORD lastErr = GetLastError();
        std::filesystem::remove(tempPath, ec);
        return fail("Failed to replace destination file: " + formatWin32Error(lastErr));
    }
#else
    std::filesystem::rename(tempPath, destinationPath, ec);
    if (ec) {
        std::filesystem::remove(tempPath, ec);
        return fail("Failed to replace destination file: " + ec.message());
    }
#endif

    return true;
}

bool ProjectManager::loadAndPrepareProject(const std::filesystem::path& sourcePath,
                                           const assets::FurnitureCatalog& catalog,
                                           ProjectData& outData,
                                           scene::Scene& outCandidateScene,
                                           graphics::Camera& outCandidateCamera,
                                           std::string* outError)
{
    auto fail = [&](const std::string& msg) {
        if (outError != nullptr) *outError = msg;
        return false;
    };

    if (sourcePath.empty()) {
        return fail("Project file path is empty.");
    }

    std::error_code ec;
    if (!std::filesystem::exists(sourcePath, ec) || ec) {
        return fail("Project file does not exist.");
    }
    if (!std::filesystem::is_regular_file(sourcePath, ec) || ec) {
        return fail("Project path is not a regular file.");
    }

    const auto fileSize = std::filesystem::file_size(sourcePath, ec);
    if (ec) {
        return fail("Unable to query project file size.");
    }
    if (fileSize == 0) {
        return fail("Project file is empty.");
    }
    if (fileSize > JsonParseLimits::kMaxInputBytes) {
        return fail("Project file size (" + std::to_string(fileSize) +
                    " bytes) exceeds maximum limit (" + std::to_string(JsonParseLimits::kMaxInputBytes) + " bytes).");
    }

    // 1. Đọc toàn bộ tệp vào bộ nhớ
    std::ifstream ifs(sourcePath, std::ios::binary);
    if (!ifs.is_open()) {
        return fail("Unable to open project file for reading.");
    }
    std::ostringstream buffer;
    buffer << ifs.rdbuf();
    if (ifs.bad()) {
        return fail("I/O error while reading project file.");
    }
    const std::string rawJson = buffer.str();

    // 2. Parse JSON vào ProjectData tạm thời
    ProjectData candidateData;
    std::string err;
    if (!ProjectSerializer::fromJsonString(rawJson, candidateData, &err)) {
        return fail("Failed to parse project file: " + err);
    }

    // 3. Xác thực toàn bộ dữ liệu dự án (schema, IDs, assets, lighting, camera, room)
    if (!ProjectValidator::validate(candidateData, catalog, &err)) {
        return fail("Invalid project data: " + err);
    }

    // 4. Khởi dựng Scene ứng viên (giao dịch) và Camera ứng viên
    scene::Scene candidateScene;
    if (!candidateScene.restoreFromSnapshot(candidateData.room,
                                            candidateData.objects,
                                            candidateData.nextObjectId,
                                            catalog,
                                            &err)) {
        return fail("Failed to construct scene from project data: " + err);
    }

    graphics::Camera candidateCam;
    if (!candidateData.camera.applyToCamera(candidateCam)) {
        return fail("Failed to restore camera state from project data.");
    }

    outData = std::move(candidateData);
    outCandidateScene = std::move(candidateScene);
    outCandidateCamera = candidateCam;
    return true;
}

bool ProjectManager::saveCurrentOrAs(const std::filesystem::path& path,
                                     const ProjectData& data,
                                     const assets::FurnitureCatalog& catalog,
                                     std::string* outError)
{
    if (!writeProjectFileSafe(path, data, catalog, outError)) {
        // Giữ nguyên dirty state và đường dẫn cũ nếu lưu thất bại
        return false;
    }

    m_currentFilePath = path;
    m_projectName = data.projectName;
    m_dirty = false;
    recordRecentProject(path);
    return true;
}

bool ProjectManager::openTransactional(const std::filesystem::path& path,
                                       const assets::FurnitureCatalog& catalog,
                                       scene::Scene& activeScene,
                                       graphics::Camera& activeCamera,
                                       ProjectData& outLoadedData,
                                       std::string* outError)
{
    ProjectData candidateData;
    scene::Scene candidateScene;
    graphics::Camera candidateCamera;
    if (!loadAndPrepareProject(path, catalog, candidateData, candidateScene, candidateCamera, outError)) {
        // Thất bại -> KHÔNG chạm vào activeScene, activeCamera, m_dirty hay m_currentFilePath
        return false;
    }

    // Commit duy nhất sau khi toàn bộ xác thực thành công
    activeScene = std::move(candidateScene);
    activeCamera = candidateCamera;
    outLoadedData = std::move(candidateData);
    m_currentFilePath = path;
    m_projectName = outLoadedData.projectName;
    m_dirty = false;
    recordRecentProject(path);
    return true;
}

void ProjectManager::resetForNewProject(const std::string& projectName, bool dirty)
{
    m_currentFilePath.clear();
    m_projectName = projectName.empty() ? "Untitled Room" : projectName;
    m_dirty = dirty;
}

void ProjectManager::recordRecentProject(const std::filesystem::path& path)
{
    if (path.empty()) {
        return;
    }
    m_recentProjects.erase(
        std::remove(m_recentProjects.begin(), m_recentProjects.end(), path),
        m_recentProjects.end());
    m_recentProjects.insert(m_recentProjects.begin(), path);
    if (m_recentProjects.size() > kMaxRecentProjects) {
        m_recentProjects.resize(kMaxRecentProjects);
    }
}

} // namespace roomcraft::persistence
