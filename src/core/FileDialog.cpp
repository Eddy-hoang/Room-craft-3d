#include "roomcraft/core/FileDialog.hpp"

#include <array>
#include <cwchar>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#endif

namespace roomcraft::core {

bool FileDialog::openProjectFile(std::filesystem::path& outPath,
                                 const std::filesystem::path& initialDir)
{
#ifdef _WIN32
    std::array<wchar_t, 4096> fileBuf{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = L"RoomCraft 3D Project (*.roomcraft)\0*.roomcraft\0JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileBuf.data();
    ofn.nMaxFile = static_cast<DWORD>(fileBuf.size());
    ofn.lpstrDefExt = L"roomcraft";
    ofn.lpstrTitle = L"Open RoomCraft 3D Project";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;

    std::wstring initDirW;
    if (!initialDir.empty()) {
        initDirW = initialDir.wstring();
        ofn.lpstrInitialDir = initDirW.c_str();
    }

    if (GetOpenFileNameW(&ofn) != FALSE) {
        outPath = std::filesystem::path(fileBuf.data());
        return true;
    }
    return false;
#else
    (void)outPath;
    (void)initialDir;
    return false;
#endif
}

bool FileDialog::saveProjectFile(std::filesystem::path& outPath,
                                 const std::string& defaultFileName,
                                 const std::filesystem::path& initialDir)
{
#ifdef _WIN32
    std::array<wchar_t, 4096> fileBuf{};
    if (!defaultFileName.empty()) {
        const std::filesystem::path defPath = std::filesystem::u8path(defaultFileName);
        const std::wstring defW = defPath.wstring();
        const std::size_t copyLen = std::min(defW.size(), fileBuf.size() - 1);
        std::wcsncpy(fileBuf.data(), defW.c_str(), copyLen);
        fileBuf[copyLen] = L'\0';
    }

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = L"RoomCraft 3D Project (*.roomcraft)\0*.roomcraft\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileBuf.data();
    ofn.nMaxFile = static_cast<DWORD>(fileBuf.size());
    ofn.lpstrDefExt = L"roomcraft";
    ofn.lpstrTitle = L"Save RoomCraft 3D Project";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;

    std::wstring initDirW;
    if (!initialDir.empty()) {
        initDirW = initialDir.wstring();
        ofn.lpstrInitialDir = initDirW.c_str();
    }

    if (GetSaveFileNameW(&ofn) != FALSE) {
        std::filesystem::path chosen(fileBuf.data());
        if (!chosen.has_extension()) {
            chosen += L".roomcraft";
        }
        outPath = std::move(chosen);
        return true;
    }
    return false;
#else
    (void)outPath;
    (void)defaultFileName;
    (void)initialDir;
    return false;
#endif
}

} // namespace roomcraft::core
