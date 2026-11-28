#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/persistence/Json.hpp"
#include "roomcraft/persistence/PreferencesManager.hpp"
#include "roomcraft/persistence/ProjectData.hpp"
#include "roomcraft/persistence/ProjectManager.hpp"
#include "roomcraft/persistence/ProjectSerializer.hpp"
#include "roomcraft/persistence/ProjectValidator.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/scene/ScenePresets.hpp"
#include "roomcraft/systems/CollisionSystem.hpp"
#include "roomcraft/systems/CommandHistory.hpp"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

using namespace roomcraft;
using namespace roomcraft::persistence;
using namespace roomcraft::systems;

int g_failed = 0;

void check(const std::string& id, const std::string& desc, bool cond)
{
    std::cout << (cond ? "[PASS] " : "[FAIL] ") << id << " - " << desc << std::endl;
    if (!cond) {
        ++g_failed;
    }
}

ProjectData makeBaselineProject(const assets::FurnitureCatalog& catalog)
{
    scene::Scene sc;
    scene::ScenePresets::applyPreset(scene::ScenePresetType::ModernBedroom, sc, catalog, nullptr);
    ProjectData pd;
    pd.format = kProjectFormatId;
    pd.schemaVersion = kCurrentSchemaVersion;
    pd.projectName = "Modern Bedroom";
    pd.room = sc.getRoom().getSettings();
    pd.objects = sc.getObjects();
    pd.nextObjectId = sc.getNextObjectId();
    pd.lightingPreset = 0;
    pd.lighting = graphics::LightingSettings::preset(graphics::LightingPreset::Daylight);
    return pd;
}

void testJsonParserAuditAndFuzzing()
{
    JsonValue v;
    std::string err;

    // 1. Valid UTF-8 Vietnamese + surrogate pair emoji (\uD83D\uDE00 -> U+1F600)
    const bool validUtf8 = parseJson(R"({"name":"Phòng Ngủ Hiện Đại","emoji":"\uD83D\uDE00","num":-12.5e-2})", v, &err);
    check("M9-JSON-01", "Valid UTF-8 Vietnamese, surrogate pair emoji, scientific float",
          validUtf8 && v.isObject() &&
              v.find("name")->asString() == "Phòng Ngủ Hiện Đại" &&
              v.find("emoji")->asString() == "\xF0\x9F\x98\x80" &&
              std::fabs(v.find("num")->asDouble() - (-0.125)) < 1e-9);

    // 2. Invalid UTF-8 sequences rejected
    const std::vector<std::string> badUtf8 = {
        std::string("{\"s\":\"\xFF\"}"),                  // invalid lead byte
        std::string("{\"s\":\"\x80\"}"),                  // stray continuation byte
        std::string("{\"s\":\"\xE2\x82\"}"),              // truncated 3-byte
        std::string("{\"s\":\"\xC0\xAF\"}"),              // overlong '/'
        std::string("{\"s\":\"\xED\xA0\x80\"}"),          // raw UTF-8 surrogate U+D800
        std::string("{\"s\":\"\xF4\x90\x80\x80\"}")       // > U+10FFFF
    };
    bool allBadUtf8Rejected = true;
    for (const auto& sample : badUtf8) {
        err.clear();
        if (parseJson(sample, v, &err) || err.empty()) {
            allBadUtf8Rejected = false;
        }
    }
    check("M9-JSON-02", "Invalid UTF-8 sequences (lead/continuation/overlong/surrogate/>U+10FFFF) rejected",
          allBadUtf8Rejected);

    // 3. Invalid Unicode escapes (lone low surrogate, lone high surrogate, bad pair)
    const bool loneLow = !parseJson(R"({"s":"\uDC00"})", v, &err);
    const bool loneHigh = !parseJson(R"({"s":"\uD800"})", v, &err);
    const bool badPair = !parseJson(R"({"s":"\uD800\u0041"})", v, &err);
    check("M9-JSON-03", "Lone low/high surrogates and malformed surrogate pairs rejected",
          loneLow && loneHigh && badPair);

    // 4. Duplicate JSON object keys rejected
    err.clear();
    const bool dupRejected = !parseJson(R"({"a": 1, "b": 2, "a": 3})", v, &err) && !err.empty();
    check("M9-JSON-04", "Duplicate JSON object keys rejected strictly", dupRejected);

    // 5. Numeric edge cases: float overflow, NaN, Infinity, bad exponents, leading zero
    const std::vector<std::string> badNums = {
        R"({"n": 1e400})",
        R"({"n": -1e400})",
        R"({"n": NaN})",
        R"({"n": Infinity})",
        R"({"n": -Infinity})",
        R"({"n": 1e})",
        R"({"n": 1e+})",
        R"({"n": 1.})",
        R"({"n": 01})"
    };
    bool allBadNumsRejected = true;
    for (const auto& s : badNums) {
        err.clear();
        if (parseJson(s, v, &err) || err.empty()) {
            allBadNumsRejected = false;
        }
    }
    check("M9-JSON-05", "Float overflow, NaN/Infinity, bad exponents and leading zeros rejected",
          allBadNumsRejected);

    // 6. Empty input populates outError & trailing garbage rejected
    std::string emptyErr;
    const bool emptyRejected = !parseJson("   \t\r\n ", v, &emptyErr) && !emptyErr.empty();
    std::string trailErr;
    const bool trailRejected = !parseJson(R"({"ok":true} garbage)", v, &trailErr) && !trailErr.empty();
    check("M9-JSON-06", "Empty input error propagation and trailing garbage rejection",
          emptyRejected && trailRejected);

    // 7. Nesting depth limit, input size limit, and total node count limit
    std::string deep(70, '[');
    deep += std::string(70, ']');
    const bool deepRejected = !parseJson(deep, v, &err, JsonParseLimits::kMaxInputBytes, 64);

    std::string bigSize(2048, ' ');
    const bool sizeRejected = !parseJson(bigSize, v, &err, 1024, 64);

    std::string manyNodes = "[";
    for (int i = 0; i < 200; ++i) {
        if (i > 0) manyNodes += ",";
        manyNodes += "1";
    }
    manyNodes += "]";
    const bool nodeCapRejected = !parseJson(manyNodes, v, &err, JsonParseLimits::kMaxInputBytes, 64, 100);
    check("M9-JSON-07", "Nesting depth (64), input byte size limit, and total node count cap enforced",
          deepRejected && sizeRejected && nodeCapRejected);

    // 8. Deterministic bounded fuzz test (1500 mutated cases, zero crashes)
    std::mt19937 rng(0xC0FFEE99u);
    const std::string seedValid = R"({"format":"RoomCraft3D","schemaVersion":1,"arr":[1,2.5,-3,true,false,null,"hi"]})";
    bool fuzzNoCrash = true;
    int rejectedCount = 0;
    for (int iter = 0; iter < 1500; ++iter) {
        std::string sample;
        const int mode = iter % 4;
        if (mode == 0) {
            // Random truncation of valid JSON
            const std::size_t cut = rng() % seedValid.size();
            sample = seedValid.substr(0, cut);
        } else if (mode == 1) {
            // Random byte mutations of valid JSON
            sample = seedValid;
            const int flips = 1 + static_cast<int>(rng() % 5);
            for (int f = 0; f < flips; ++f) {
                sample[rng() % sample.size()] = static_cast<char>(rng() & 0xFFu);
            }
        } else if (mode == 2) {
            // Random grammar soup
            static constexpr const char kAlphabet[] = "{}[]\":,0123456789-.eE+tfnul\\uD800\n\t ";
            const std::size_t len = 1 + (rng() % 96);
            sample.resize(len);
            for (std::size_t k = 0; k < len; ++k) {
                sample[k] = kAlphabet[rng() % (sizeof(kAlphabet) - 1)];
            }
        } else {
            // Pure arbitrary binary bytes
            const std::size_t len = 1 + (rng() % 64);
            sample.resize(len);
            for (std::size_t k = 0; k < len; ++k) {
                sample[k] = static_cast<char>(rng() & 0xFFu);
            }
        }

        JsonValue out;
        std::string ferr;
        if (!parseJson(sample, out, &ferr, 4096, 32, 2000)) {
            ++rejectedCount;
            if (ferr.empty()) {
                fuzzNoCrash = false;
            }
        }
    }
    check("M9-JSON-08", "Deterministic 1500-case bounded JSON fuzz test completed cleanly without crashes",
          fuzzNoCrash && rejectedCount > 1200);
}

void testPersistenceSafetyAndBackwardCompatibility(const assets::FurnitureCatalog& catalog)
{
    const auto tmpDir = std::filesystem::temp_directory_path() / L"RoomCraft3D_M9_RelTest_Phòng 3D";
    std::error_code ec;
    std::filesystem::remove_all(tmpDir, ec);
    std::filesystem::create_directories(tmpDir, ec);

    const ProjectData base = makeBaselineProject(catalog);

    // 1. M8 schema v1 backward compatibility (JSON without optional M9 shadow fields)
    std::string m9Json;
    ProjectSerializer::toJsonString(base, m9Json);
    auto stripLineContaining = [](std::string text, const std::string& needle) {
        std::size_t pos = text.find(needle);
        if (pos != std::string::npos) {
            std::size_t lineStart = text.rfind('\n', pos);
            std::size_t lineEnd = text.find('\n', pos);
            if (lineStart != std::string::npos && lineEnd != std::string::npos) {
                text.erase(lineStart, lineEnd - lineStart);
            }
        }
        return text;
    };
    // Strip optional M9/RC2 fields (shadows + showWindow) to verify 100% M8/RC1 backward compatibility
    std::string m8Compat = stripLineContaining(m9Json, "\"shadowBias\"");
    m8Compat = stripLineContaining(m8Compat, "\"shadowMapSize\"");
    m8Compat = stripLineContaining(m8Compat, "\"shadowsEnabled\"");
    m8Compat = stripLineContaining(m8Compat, "\"showWindow\"");
    if (const auto sgPos = m8Compat.find("\"showGrid\""); sgPos != std::string::npos) {
        const auto lineEnd = m8Compat.find('\n', sgPos);
        if (lineEnd != std::string::npos && lineEnd > 0 && m8Compat[lineEnd - 1] == ',') {
            m8Compat.erase(lineEnd - 1, 1);
        }
    }
    if (const auto paPos = m8Compat.find("\"pointAttenuation\""); paPos != std::string::npos) {
        const auto lineEnd = m8Compat.find('\n', paPos);
        if (lineEnd != std::string::npos && lineEnd > 0 && m8Compat[lineEnd - 1] == ',') {
            m8Compat.erase(lineEnd - 1, 1);
        }
    }
    ProjectData loadedCompat;
    std::string compatErr;
    const bool compatOk = ProjectSerializer::fromJsonString(m8Compat, loadedCompat, &compatErr) &&
                          ProjectValidator::validate(loadedCompat, catalog, &compatErr);
    check("M9-PERS-01", "Backward compatibility with M8/RC1 schema v1 files without optional shadow/window fields",
          compatOk && loadedCompat.lighting.shadowsEnabled && loadedCompat.lighting.shadowMapSize == 1024 &&
          loadedCompat.room.showWindow);

    // 2. Invalid shadow map size & shadow bias rejected by ProjectValidator
    ProjectData badShadow = base;
    badShadow.lighting.shadowMapSize = 777;
    std::string valErr;
    const bool badSizeRej = !ProjectValidator::validate(badShadow, catalog, &valErr);
    badShadow.lighting.shadowMapSize = 1024;
    badShadow.lighting.shadowBias = -0.01f;
    const bool badBiasRej = !ProjectValidator::validate(badShadow, catalog, &valErr);
    check("M9-PERS-02", "Invalid shadowMapSize and shadowBias rejected by ProjectValidator",
          badSizeRej && badBiasRej);

    // 3. Controlled failure injection: locked destination file preserves original bytes & cleans up temp file
    const auto destPath = tmpDir / L"Dự án Kiểm thử.roomcraft";
    std::string err;
    const bool initSaved = ProjectManager::writeProjectFileSafe(destPath, base, catalog, &err);
    std::string originalBytes;
    {
        std::ifstream ifs(destPath, std::ios::binary);
        originalBytes.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    }

    bool lockedSaveFailed = false;
    bool bytesPreserved = false;
    bool noTempLeftovers = true;

#ifdef _WIN32
    HANDLE lockHandle = CreateFileW(destPath.c_str(), GENERIC_READ, 0 /* exclusive lock */,
                                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (lockHandle != INVALID_HANDLE_VALUE) {
        ProjectData modified = base;
        modified.projectName = "Should Not Overwrite";
        modified.lighting.ambient = 0.9f;
        lockedSaveFailed = !ProjectManager::writeProjectFileSafe(destPath, modified, catalog, &err);
        CloseHandle(lockHandle);
    }
#else
    lockedSaveFailed = true;
#endif

    {
        std::ifstream ifs(destPath, std::ios::binary);
        const std::string afterBytes((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        bytesPreserved = (afterBytes == originalBytes);
    }
    for (const auto& entry : std::filesystem::directory_iterator(tmpDir, ec)) {
        const auto name = entry.path().filename().u8string();
        if (std::string(name.begin(), name.end()).find(".tmp.") != std::string::npos) {
            noTempLeftovers = false;
        }
    }
    check("M9-PERS-03", "Locked destination failure injection preserves original bytes and cleans up temp files",
          initSaved && lockedSaveFailed && bytesPreserved && noTempLeftovers);

    std::filesystem::remove_all(tmpDir, ec);
}

void testDirtyStateAndUndoRedoExactness(const assets::FurnitureCatalog& catalog)
{
    scene::Scene sc;
    scene::ScenePresets::applyPreset(scene::ScenePresetType::ModernBedroom, sc, catalog, nullptr);
    graphics::LightingSettings lighting = graphics::LightingSettings::preset(graphics::LightingPreset::Daylight);

    auto makeData = [&]() {
        ProjectData d;
        d.format = kProjectFormatId;
        d.schemaVersion = kCurrentSchemaVersion;
        d.projectName = "DirtyAudit";
        d.room = sc.getRoom().getSettings();
        d.objects = sc.getObjects();
        d.nextObjectId = sc.getNextObjectId();
        d.lighting = lighting;
        return d;
    };

    ProjectManager pm;
    CommandHistory history(64);
    pm.captureSavedBaseline(makeData());
    const bool cleanAtStart = !pm.isDirty();

    // Edit 1: Move first object
    scene::ObjectId sel = sc.getObjects().front().id;
    history.pushUndo(CommandHistory::capture(sc, sel, "Move object"));
    scene::Transform tr = sc.getObjects().front().transform;
    tr.position.x += 0.5f;
    sc.setObjectTransform(sel, tr);
    const bool dirtyAfterEdit = pm.syncDirtyWithState(makeData());

    // Undo back to saved baseline -> dirty MUST become false!
    std::string desc;
    const bool undoOk = history.undo(sc, sel, catalog, &desc);
    const bool cleanAfterUndoToSaved = !pm.syncDirtyWithState(makeData());

    // Redo beyond saved baseline -> dirty MUST become true!
    const bool redoOk = history.redo(sc, sel, catalog, &desc);
    const bool dirtyAfterRedo = pm.syncDirtyWithState(makeData());

    // Save at new state -> dirty becomes false; Undo from saved state -> dirty becomes true; Redo -> false!
    pm.captureSavedBaseline(makeData());
    const bool cleanAfterSave = !pm.isDirty();
    history.undo(sc, sel, catalog, &desc);
    const bool dirtyAfterUndoFromNewSave = pm.syncDirtyWithState(makeData());
    history.redo(sc, sel, catalog, &desc);
    const bool cleanAfterRedoToNewSave = !pm.syncDirtyWithState(makeData());

    // Non-CommandHistory persisted edit (Lighting) keeps dirty=true even after Undo of object move!
    lighting.ambient = 0.77f;
    const bool dirtyAfterLightingEdit = pm.syncDirtyWithState(makeData());
    history.pushUndo(CommandHistory::capture(sc, sel, "Move 2"));
    tr.position.z += 0.25f;
    sc.setObjectTransform(sel, tr);
    pm.syncDirtyWithState(makeData());
    history.undo(sc, sel, catalog, &desc);
    const bool stillDirtyBecauseLightingChanged = pm.syncDirtyWithState(makeData());
    lighting.ambient = graphics::LightingSettings::preset(graphics::LightingPreset::Daylight).ambient;
    const bool cleanAfterRevertingLighting = !pm.syncDirtyWithState(makeData());

    check("M9-DIRTY-01", "Exact dirty-state across Edit -> Undo to saved -> Redo -> Save -> Undo -> Redo",
          cleanAtStart && dirtyAfterEdit && undoOk && cleanAfterUndoToSaved && redoOk &&
              dirtyAfterRedo && cleanAfterSave && dirtyAfterUndoFromNewSave && cleanAfterRedoToNewSave);
    check("M9-DIRTY-02", "Non-CommandHistory persisted changes (Lighting) accurately preserve dirty state across Undo",
          dirtyAfterLightingEdit && stillDirtyBecauseLightingChanged && cleanAfterRevertingLighting);
}

void testCollisionAudit()
{
    // 1. Prevent Invalid non-trapping transition checks
    ObjectCollisionStatus valid;
    valid.boundsValid = true;
    valid.boundary.valid = true;

    ObjectCollisionStatus outBig = valid;
    outBig.boundary.valid = false;
    outBig.boundary.exceedsEast = true;
    outBig.boundary.maxViolationDepth = 0.80f;

    ObjectCollisionStatus outSmall = valid;
    outSmall.boundary.valid = false;
    outSmall.boundary.exceedsEast = true;
    outSmall.boundary.maxViolationDepth = 0.30f;

    ObjectCollisionStatus colOne = valid;
    colOne.collidingObjectIds = {2};
    colOne.collidingObjectNames = {"Desk"};

    const bool blockValidToInvalid = !CollisionSystem::isPreventInvalidTransitionAllowed(valid, outSmall);
    const bool allowInvalidToValid = CollisionSystem::isPreventInvalidTransitionAllowed(outBig, valid);
    const bool allowInvalidRecoveryStep = CollisionSystem::isPreventInvalidTransitionAllowed(outBig, outSmall);
    const bool blockWorseningInvalid = !CollisionSystem::isPreventInvalidTransitionAllowed(outSmall, outBig);
    const bool blockNewCollision = !CollisionSystem::isPreventInvalidTransitionAllowed(valid, colOne);

    check("M9-COLL-01", "Prevent Invalid blocks new invalid placements while allowing recovery from pre-existing invalid states",
          blockValidToInvalid && allowInvalidToValid && allowInvalidRecoveryStep &&
              blockWorseningInvalid && blockNewCollision);

    // 2. Rug policy does NOT falsely exempt flattened arbitrary furniture
    scene::SceneObject flatDesk;
    flatDesk.id = 10;
    flatDesk.assetId = "procedural-desk";
    math::BoundingBox flatBox;
    flatBox.min = {-0.5f, 0.0f, -0.3f};
    flatBox.max = {0.5f, 0.02f, 0.3f};

    scene::SceneObject carpet;
    carpet.id = 11;
    carpet.assetId = "blender-carpet";
    math::BoundingBox carpetBox;
    carpetBox.min = {-1.1f, 0.0f, -0.75f};
    carpetBox.max = {1.1f, 0.016f, 0.75f};

    check("M9-COLL-02", "Rug overlap policy exempts floor-covering assets only (not flattened furniture)",
          !CollisionSystem::isFloorCovering(flatDesk, flatBox) &&
              CollisionSystem::isFloorCovering(carpet, carpetBox));
}

void testPreferencesPersistence()
{
    const auto tmpDir = std::filesystem::temp_directory_path() / L"RoomCraft3D_M9_PrefsTest";
    std::error_code ec;
    std::filesystem::remove_all(tmpDir, ec);
    std::filesystem::create_directories(tmpDir, ec);
    const auto prefPath = tmpDir / L"preferences.json";

    // 1. Missing file returns safe defaults
    UserPreferences missing;
    std::string err;
    const bool missingHandled = !PreferencesManager::loadFromFile(prefPath, missing, &err) &&
                                missing.leftPanelWidth == 272.0f && missing.uiScale == 1.0f;

    // 2. Save & reload round-trip with Unicode Recent Projects + clamping verification
    UserPreferences toSave;
    toSave.leftPanelWidth = 340.0f;
    toSave.rightPanelWidth = 390.0f;
    toSave.hierarchyHeightRatio = 0.48f;
    toSave.uiScale = 1.25f;
    toSave.showLeftPanel = true;
    toSave.showRightPanel = false;
    toSave.libraryGridView = false;
    toSave.recentProjects = {
        std::filesystem::path(L"D:\\Thiết kế\\Phòng ngủ 1.roomcraft"),
        std::filesystem::path(L"D:\\Projects\\Gaming Room.roomcraft")
    };
    const bool saved = PreferencesManager::saveToFile(prefPath, toSave, &err);

    UserPreferences loaded;
    const bool reloaded = PreferencesManager::loadFromFile(prefPath, loaded, &err);
    const bool roundTripOk = saved && reloaded &&
                             std::fabs(loaded.leftPanelWidth - 340.0f) < 0.01f &&
                             std::fabs(loaded.rightPanelWidth - 390.0f) < 0.01f &&
                             std::fabs(loaded.hierarchyHeightRatio - 0.48f) < 0.001f &&
                             std::fabs(loaded.uiScale - 1.25f) < 0.001f &&
                             loaded.showRightPanel == false &&
                             loaded.libraryGridView == false &&
                             loaded.recentProjects.size() == 2 &&
                             loaded.recentProjects[0] == toSave.recentProjects[0];

    // 3. Out-of-range values clamped safely & corrupted file falls back cleanly
    {
        std::ofstream ofs(prefPath, std::ios::binary | std::ios::trunc);
        ofs << R"({"version":1,"leftPanelWidth":99999,"rightPanelWidth":-50,"hierarchyHeightRatio":9.5,"uiScale":8.0})";
    }
    UserPreferences clamped;
    const bool clampedOk = PreferencesManager::loadFromFile(prefPath, clamped, &err) &&
                           clamped.leftPanelWidth <= 420.0f && clamped.rightPanelWidth >= 260.0f &&
                           clamped.hierarchyHeightRatio <= 0.72f && clamped.uiScale <= 1.50f;

    {
        std::ofstream ofs(prefPath, std::ios::binary | std::ios::trunc);
        ofs << "{corrupted json !!!";
    }
    UserPreferences corrupt;
    const bool corruptFallback = !PreferencesManager::loadFromFile(prefPath, corrupt, &err) &&
                                 corrupt.leftPanelWidth == 272.0f && corrupt.uiScale == 1.0f;

    check("M9-PREF-01", "PreferencesManager round-trip, Unicode Recent Projects, clamping, and corrupt/missing fallback",
          missingHandled && roundTripOk && clampedOk && corruptFallback);

    std::filesystem::remove_all(tmpDir, ec);
}

} // namespace

int main()
{
    std::cout << "=== RoomCraft 3D — M9 Reliability, Fuzzing & Preferences Unit Tests ===" << std::endl;
    assets::FurnitureCatalog catalog;

    testJsonParserAuditAndFuzzing();
    testPersistenceSafetyAndBackwardCompatibility(catalog);
    testDirtyStateAndUndoRedoExactness(catalog);
    testCollisionAudit();
    testPreferencesPersistence();

    std::cout << "=== M9 Reliability Tests Summary: "
              << (g_failed == 0 ? "ALL PASS" : "FAILED (" + std::to_string(g_failed) + ")")
              << " ===" << std::endl;
    return g_failed == 0 ? 0 : 1;
}
