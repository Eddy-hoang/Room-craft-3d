#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/persistence/Json.hpp"
#include "roomcraft/persistence/ProjectData.hpp"
#include "roomcraft/persistence/ProjectManager.hpp"
#include "roomcraft/persistence/ProjectSerializer.hpp"
#include "roomcraft/persistence/ProjectValidator.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/systems/CommandHistory.hpp"

#include <glm/geometric.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace roomcraft;

namespace {

int g_failures = 0;

void check(const char* name, bool condition)
{
    std::cout << "  [M8-Persistence] " << name << ": " << (condition ? "PASS" : "FAIL") << std::endl;
    if (!condition) {
        ++g_failures;
    }
}

bool nearEq(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) <= eps;
}

bool nearVec3(const glm::vec3& a, const glm::vec3& b, float eps = 1e-4f)
{
    return glm::length(a - b) <= eps;
}

bool nearVec4(const glm::vec4& a, const glm::vec4& b, float eps = 1e-4f)
{
    return std::fabs(a.r - b.r) <= eps && std::fabs(a.g - b.g) <= eps &&
           std::fabs(a.b - b.b) <= eps && std::fabs(a.a - b.a) <= eps;
}

std::string readWholeFile(const std::filesystem::path& p)
{
    std::ifstream ifs(p, std::ios::binary);
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

void writeWholeFile(const std::filesystem::path& p, const std::string& content)
{
    std::ofstream ofs(p, std::ios::binary | std::ios::trunc);
    ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
}

persistence::ProjectData makeRichProject(const assets::FurnitureCatalog& catalog)
{
    persistence::ProjectData data;
    data.format = persistence::kProjectFormatId;
    data.schemaVersion = persistence::kCurrentSchemaVersion;
    data.projectName = "Phòng Ngủ Hiện Đại 2026";

    data.room.width = 7.5f;
    data.room.length = 8.25f;
    data.room.height = 3.4f;
    data.room.wallThickness = 0.20f;
    data.room.floorThickness = 0.12f;
    data.room.gridSpacing = 0.25f;
    data.room.floorColor = {0.55f, 0.42f, 0.31f, 1.0f};
    data.room.wallColor = {0.91f, 0.89f, 0.84f, 1.0f};
    data.room.ceilingColor = {0.95f, 0.95f, 0.93f, 1.0f};
    data.room.wallVisibilityMode = scene::WallVisibilityMode::Manual;
    data.room.showNorthWall = true;
    data.room.showSouthWall = false;
    data.room.showEastWall = true;
    data.room.showWestWall = false;
    data.room.showCeiling = true;
    data.room.showGrid = true;

    // Đặt cả 14 asset (bao gồm ID lớn > 2^53 để xác minh lossless uint64_t!)
    const auto& entries = catalog.getAllEntries();
    std::uint64_t baseId = 9007199254740993ULL; // 2^53 + 1 (double sẽ mất độ chính xác nếu không lưu lossless)
    for (std::size_t i = 0; i < entries.size(); ++i) {
        scene::SceneObject obj;
        obj.id = baseId + static_cast<std::uint64_t>(i * 3);
        obj.assetId = entries[i].assetId;
        obj.name = entries[i].displayName + " #" + std::to_string(i + 1);
        obj.transform.position = glm::vec3(-2.5f + 0.35f * static_cast<float>(i), 0.12f * static_cast<float>(i % 3), -1.5f + 0.25f * static_cast<float>(i));
        obj.transform.rotationDegrees = glm::vec3(2.5f * static_cast<float>(i % 4), 15.0f * static_cast<float>(i), -1.5f * static_cast<float>(i % 3));
        obj.transform.scale = glm::vec3(0.85f + 0.1f * static_cast<float>(i % 4), 1.0f + 0.05f * static_cast<float>(i % 3), 1.15f);
        obj.visible = (i % 5 != 0);
        obj.locked = (i % 4 == 0);
        obj.shininessScale = 0.35f + 0.25f * static_cast<float>(i % 6);
        data.objects.push_back(std::move(obj));
    }
    data.nextObjectId = baseId + static_cast<std::uint64_t>(entries.size() * 3) + 10ULL;

    data.lightingPreset = -1; // Custom
    data.lighting.ambient = 0.29f;
    data.lighting.sunEnabled = true;
    data.lighting.direction = {-0.35f, -0.85f, -0.40f};
    data.lighting.sunColor = {0.98f, 0.90f, 0.78f};
    data.lighting.sunIntensity = 0.88f;
    data.lighting.pointEnabled = true;
    data.lighting.position = {1.25f, 2.45f, -0.85f};
    data.lighting.pointColor = {1.0f, 0.72f, 0.48f};
    data.lighting.pointIntensity = 2.35f;
    data.lighting.attenuation = {1.0f, 0.14f, 0.07f};

    data.renderSettings.msaaSampleIndex = 2; // 4x
    data.renderSettings.wireframe = true;
    data.renderSettings.showAxes = false;
    data.renderSettings.showSelectedBBox = true;
    data.renderSettings.showMoveGizmo = false;

    data.camera.projectionMode = graphics::ProjectionMode::Orthographic;
    data.camera.position = {6.2f, 5.4f, 7.1f};
    data.camera.target = {0.3f, 0.6f, -0.4f};
    data.camera.up = {0.0f, 1.0f, 0.0f};
    data.camera.fovDegrees = 52.0f;
    data.camera.nearPlane = 0.1f;
    data.camera.farPlane = 150.0f;
    data.camera.orthoHalfHeight = 4.75f;

    data.editorSettings.showGrid = false;
    data.editorSettings.gridSnapEnabled = true;
    data.editorSettings.placementMode = systems::PlacementValidationMode::PreventInvalid;
    data.editorSettings.allowRugOverlap = false;

    return data;
}

} // namespace

int main()
{
    std::cout << "================ RoomCraft 3D - M8 Persistence & Undo Unit Tests (CPU) ================" << std::endl;

    assets::FurnitureCatalog catalog;
    const std::filesystem::path tmpDir = std::filesystem::temp_directory_path() / L"roomcraft3d_m8_persist test_DựÁn";
    std::error_code ec;
    std::filesystem::remove_all(tmpDir, ec);
    std::filesystem::create_directories(tmpDir, ec);

    // M8-T10..18: Complete JSON Serialization, Deserialization, Lossless uint64 IDs, All 14 Assets,
    //             Transforms, Material Overrides, Lighting, Room & Camera Round-trip (with Unicode + spaces path!)
    const auto original = makeRichProject(catalog);
    const std::filesystem::path unicodePath = tmpDir / L"Dự Án Phòng Ngủ (Final).roomcraft";
    {
        std::string jsonText;
        std::string err;
        const bool serOk = persistence::ProjectSerializer::toJsonString(original, jsonText, &err);

        // Kiểm tra JSON không chứa handle GPU hay đường dẫn tuyệt đối
        const bool noGpuLeaks = (jsonText.find("VAO") == std::string::npos) &&
                                (jsonText.find("D:\\") == std::string::npos) &&
                                (jsonText.find("\"9007199254740993\"") != std::string::npos);

        persistence::ProjectData parsed;
        const bool deserOk = persistence::ProjectSerializer::fromJsonString(jsonText, parsed, &err);
        const bool validOk = persistence::ProjectValidator::validate(parsed, catalog, &err);

        check("M8-T11/12 Project JSON serialization & deserialization (lossless uint64 string IDs, no GPU handles)",
              serOk && noGpuLeaks && deserOk && validOk);

        // Round-trip qua hệ thống tệp thật có khoảng trắng & tên Unicode
        persistence::ProjectManager pm;
        const bool savedOk = pm.saveCurrentOrAs(unicodePath, original, catalog, &err);

        scene::Scene loadedScene;
        graphics::Camera loadedCam;
        persistence::ProjectData loadedData;
        const bool openedOk = pm.openTransactional(unicodePath, catalog, loadedScene, loadedCam, loadedData, &err);

        bool all14Match = openedOk && (loadedScene.getObjectCount() == 14) &&
                          (loadedScene.getNextObjectId() == original.nextObjectId);
        for (std::size_t i = 0; i < 14 && all14Match; ++i) {
            const auto& a = original.objects[i];
            const auto* b = loadedScene.findObject(a.id);
            all14Match = (b != nullptr) &&
                         (b->id == a.id) && (b->assetId == a.assetId) && (b->name == a.name) &&
                         (b->visible == a.visible) && (b->locked == a.locked) &&
                         nearEq(b->shininessScale, a.shininessScale) &&
                         nearVec3(b->transform.position, a.transform.position) &&
                         nearVec3(b->transform.rotationDegrees, a.transform.rotationDegrees) &&
                         nearVec3(b->transform.scale, a.transform.scale);
        }

        // Thêm đối tượng mới sau khi load -> ID mới phải >= original.nextObjectId (không bao giờ tái sử dụng ID cũ!)
        const auto newAddedId = loadedScene.addObject("procedural-desk", "New Desk", scene::Transform{}, catalog, &err);
        const bool allocatorOk = (newAddedId == original.nextObjectId) &&
                                 (loadedScene.getNextObjectId() == original.nextObjectId + 1ULL);

        const bool lightOk = (loadedData.lightingPreset == original.lightingPreset) &&
                             nearEq(loadedData.lighting.ambient, original.lighting.ambient) &&
                             (loadedData.lighting.sunEnabled == original.lighting.sunEnabled) &&
                             nearVec3(loadedData.lighting.direction, original.lighting.direction) &&
                             nearVec3(loadedData.lighting.sunColor, original.lighting.sunColor) &&
                             nearEq(loadedData.lighting.sunIntensity, original.lighting.sunIntensity) &&
                             (loadedData.lighting.pointEnabled == original.lighting.pointEnabled) &&
                             nearVec3(loadedData.lighting.position, original.lighting.position) &&
                             nearVec3(loadedData.lighting.pointColor, original.lighting.pointColor) &&
                             nearEq(loadedData.lighting.pointIntensity, original.lighting.pointIntensity) &&
                             nearVec3(loadedData.lighting.attenuation, original.lighting.attenuation);

        const auto& lr = loadedScene.getRoom().getSettings();
        const bool roomCamOk =
            nearEq(lr.width, original.room.width) && nearEq(lr.length, original.room.length) &&
            nearEq(lr.height, original.room.height) && nearEq(lr.gridSpacing, original.room.gridSpacing) &&
            nearVec4(lr.floorColor, original.room.floorColor) &&
            nearVec4(lr.wallColor, original.room.wallColor) &&
            (lr.wallVisibilityMode == original.room.wallVisibilityMode) &&
            (lr.showSouthWall == original.room.showSouthWall) &&
            (lr.showCeiling == original.room.showCeiling) &&
            (loadedCam.getProjectionMode() == original.camera.projectionMode) &&
            nearVec3(loadedCam.getPosition(), original.camera.position) &&
            nearVec3(loadedCam.getTarget(), original.camera.target) &&
            nearEq(loadedCam.getOrthoHalfHeight(), original.camera.orthoHalfHeight) &&
            (loadedData.renderSettings.msaaSampleIndex == 2) &&
            (loadedData.editorSettings.placementMode == systems::PlacementValidationMode::PreventInvalid) &&
            (loadedData.editorSettings.gridSnapEnabled == true);

        check("M8-T10/13..18 Full 14-asset save/load round-trip (IDs, allocator, transforms, material overrides, lighting, room, camera)",
              savedOk && openedOk && all14Match && allocatorOk && lightOk && roomCamOk);
    }

    // M8-T19: Invalid JSON rejection (syntax error, trailing garbage, empty, NaN token)
    {
        persistence::ProjectData tmp;
        std::string err;
        const bool r1 = !persistence::ProjectSerializer::fromJsonString("{broken json", tmp, &err);
        const bool r2 = !persistence::ProjectSerializer::fromJsonString("{\"format\":\"RoomCraft3D\"} trailing", tmp, &err);
        const bool r3 = !persistence::ProjectSerializer::fromJsonString("", tmp, &err);
        const bool r4 = !persistence::ProjectSerializer::fromJsonString("{\"a\": NaN}", tmp, &err);
        check("M8-T19 Invalid JSON syntax, trailing tokens, empty input & non-standard NaN rejected", r1 && r2 && r3 && r4);
    }

    // M8-T20: Unsupported schema version & format rejection
    {
        std::string validJson;
        persistence::ProjectSerializer::toJsonString(original, validJson);

        std::string badSchema = validJson;
        const auto pos = badSchema.find("\"schemaVersion\": 1");
        if (pos != std::string::npos) badSchema.replace(pos, 18, "\"schemaVersion\": 99");

        std::string badFormat = validJson;
        const auto fpos = badFormat.find("\"RoomCraft3D\"");
        if (fpos != std::string::npos) badFormat.replace(fpos, 13, "\"OtherApp\"");

        persistence::ProjectData tmp;
        std::string err1, err2;
        const bool rejectSchema = !persistence::ProjectSerializer::fromJsonString(badSchema, tmp, &err1);
        const bool rejectFormat = !persistence::ProjectSerializer::fromJsonString(badFormat, tmp, &err2);
        check("M8-T20 Unsupported schemaVersion (99) & foreign format rejected safely",
              rejectSchema && !err1.empty() && rejectFormat && !err2.empty());
    }

    // M8-T21: Duplicate ObjectId, invalid ObjectId=0 & nextObjectId <= maxId rejection
    {
        auto dup = original;
        dup.objects[1].id = dup.objects[0].id;
        std::string errDup;
        const bool rejDup = !persistence::ProjectValidator::validate(dup, catalog, &errDup);

        auto zeroId = original;
        zeroId.objects[0].id = 0;
        std::string errZero;
        const bool rejZero = !persistence::ProjectValidator::validate(zeroId, catalog, &errZero);

        auto badAlloc = original;
        badAlloc.nextObjectId = badAlloc.objects.back().id; // phải > maxId!
        std::string errAlloc;
        const bool rejAlloc = !persistence::ProjectValidator::validate(badAlloc, catalog, &errAlloc);

        check("M8-T21 Duplicate ObjectId, zero ObjectId & invalid nextObjectId rejected",
              rejDup && rejZero && rejAlloc);
    }

    // M8-T22: Invalid numeric & transform rejection (negative/zero scale, NaN, out-of-range room, bad lighting)
    {
        auto badScale = original;
        badScale.objects[0].transform.scale.x = -0.5f;
        const bool rejScale = !persistence::ProjectValidator::validate(badScale, catalog);

        auto badNan = original;
        badNan.objects[0].transform.position.y = std::numeric_limits<float>::quiet_NaN();
        const bool rejNan = !persistence::ProjectValidator::validate(badNan, catalog);

        auto badRoom = original;
        badRoom.room.width = 0.5f; // < kMinWidth (2.0m)
        const bool rejRoom = !persistence::ProjectValidator::validate(badRoom, catalog);

        auto badAtten = original;
        badAtten.lighting.attenuation.x = 0.0f; // constant must be > 0
        const bool rejAtten = !persistence::ProjectValidator::validate(badAtten, catalog);

        check("M8-T22 Invalid negative scale, NaN transform, room bounds & lighting attenuation rejected",
              rejScale && rejNan && rejRoom && rejAtten);
    }

    // M8-T23: Missing asset reference rejection with actionable error message
    {
        auto missing = original;
        missing.objects[0].assetId = "nonexistent-vintage-piano";
        std::string err;
        const bool rejected = !persistence::ProjectValidator::validate(missing, catalog, &err);
        check("M8-T23 Missing asset reference rejected with actionable assetId diagnostic",
              rejected && err.find("nonexistent-vintage-piano") != std::string::npos);
    }

    // M8-T24: Transactional load failure preserves active scene, camera, dirty state & current path
    {
        persistence::ProjectManager pm;
        scene::Scene activeScene;
        graphics::Camera activeCam;
        activeCam.lookAt({3.0f, 2.0f, 4.0f}, {0.0f, 0.5f, 0.0f});
        const auto keepId = activeScene.addObject("blender-modern-sofa", "Keep Sofa", scene::Transform({1.0f, 0.0f, 0.5f}), catalog);
        pm.resetForNewProject("Active Design", true);

        const std::filesystem::path corruptPath = tmpDir / "corrupt.roomcraft";
        writeWholeFile(corruptPath, "{ \"format\": \"RoomCraft3D\", \"schemaVersion\": 1, broken");

        persistence::ProjectData dummy;
        std::string err;
        const bool failed = !pm.openTransactional(corruptPath, catalog, activeScene, activeCam, dummy, &err);

        const auto* stillThere = activeScene.findObject(keepId);
        check("M8-T24 Transactional load failure preserves active scene, camera, dirty state & project name",
              failed && activeScene.getObjectCount() == 1 && stillThere != nullptr &&
              stillThere->name == "Keep Sofa" &&
              nearVec3(activeCam.getPosition(), {3.0f, 2.0f, 4.0f}) &&
              pm.isDirty() && pm.getProjectName() == "Active Design");
    }

    // M8-T25: Safe save failure preserves existing valid project file intact (read-only target & invalid dir)
    {
        const std::filesystem::path protectedPath = tmpDir / "readonly_keep.roomcraft";
        std::string err;
        const bool initialSave = persistence::ProjectManager::writeProjectFileSafe(protectedPath, original, catalog, &err);
        const std::string beforeBytes = readWholeFile(protectedPath);

#ifdef _WIN32
        SetFileAttributesW(protectedPath.c_str(), FILE_ATTRIBUTE_READONLY);
#endif
        auto modified = original;
        modified.projectName = "Should Not Overwrite";
        const bool failedRoSave = !persistence::ProjectManager::writeProjectFileSafe(protectedPath, modified, catalog, &err);
        const std::string afterBytes = readWholeFile(protectedPath);

#ifdef _WIN32
        SetFileAttributesW(protectedPath.c_str(), FILE_ATTRIBUTE_NORMAL);
#endif
        const std::filesystem::path badDirPath = tmpDir / "nonexistent_subdir_xyz" / "fail.roomcraft";
        const bool failedBadDir = !persistence::ProjectManager::writeProjectFileSafe(badDirPath, modified, catalog, &err);

        check("M8-T25 Safe save failure (read-only file / missing directory) preserves previous valid project byte-for-byte",
              initialSave && failedRoSave && (beforeBytes == afterBytes) && !beforeBytes.empty() && failedBadDir);
    }

    // M8-T26 & M8-T27: Dirty-state tracking, failed save retains dirty, successful save clears dirty, Recent Projects
    {
        persistence::ProjectManager pm;
        pm.resetForNewProject("Dirty Flow Test", false);
        const bool cleanInit = !pm.isDirty();
        pm.markDirty();
        const bool dirtyAfterEdit = pm.isDirty();

        // Save thất bại (thư mục không tồn tại) -> dirty PHẢI giữ true!
        std::string err;
        const bool failSave = !pm.saveCurrentOrAs(tmpDir / "no_such_dir" / "a.roomcraft", original, catalog, &err);
        const bool stillDirtyAfterFail = pm.isDirty();

        // Save thành công -> dirty trở về false & ghi nhận Recent Projects
        const std::filesystem::path okPath = tmpDir / "clean_save.roomcraft";
        const bool okSave = pm.saveCurrentOrAs(okPath, original, catalog, &err);
        const bool cleanAfterSave = !pm.isDirty() && pm.getCurrentFilePath() == okPath &&
                                    !pm.getRecentProjects().empty() && pm.getRecentProjects().front() == okPath;

        check("M8-T26/27 Dirty-state tracking (stays dirty on failed save, clears on success) & Recent Projects",
              cleanInit && dirtyAfterEdit && failSave && stillDirtyAfterFail && okSave && cleanAfterSave);
    }

    // M8-T31: Optional Undo/Redo (CommandHistory P2)
    {
        scene::Scene s;
        systems::CommandHistory hist(3); // Giới hạn 3 mục để kiểm thử bounded cap
        scene::ObjectId sel = scene::kInvalidObjectId;

        // 1. Add object
        hist.pushUndo(systems::CommandHistory::capture(s, sel, "Add Chair"));
        const auto id1 = s.addObject("procedural-chair", "Chair 1", scene::Transform({0.0f, 0.0f, 0.0f}), catalog);
        sel = id1;

        // 2. Move object (1 completed drag = 1 entry)
        hist.pushUndo(systems::CommandHistory::capture(s, sel, "Move Chair"));
        s.setObjectTransform(id1, scene::Transform({1.5f, 0.0f, -1.0f}));

        // 3. Rotate object
        hist.pushUndo(systems::CommandHistory::capture(s, sel, "Rotate Chair"));
        s.setObjectTransform(id1, scene::Transform({1.5f, 0.0f, -1.0f}, {0.0f, 90.0f, 0.0f}));

        // Undo Rotate -> back to 0 deg
        std::string desc;
        const bool u1 = hist.undo(s, sel, catalog, &desc);
        const bool afterU1 = u1 && nearEq(s.findObject(id1)->transform.rotationDegrees.y, 0.0f) && hist.canRedo();

        // Redo Rotate -> back to 90 deg
        const bool r1 = hist.redo(s, sel, catalog, &desc);
        const bool afterR1 = r1 && nearEq(s.findObject(id1)->transform.rotationDegrees.y, 90.0f) && !hist.canRedo();

        // Undo Rotate -> new edit (Scale) invalidates Redo stack!
        const bool u2 = hist.undo(s, sel, catalog, &desc);
        hist.pushUndo(systems::CommandHistory::capture(s, sel, "Scale Chair"));
        s.setObjectTransform(id1, scene::Transform({1.5f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {1.5f, 1.5f, 1.5f}));
        const bool redoCleared = u2 && !hist.canRedo();

        // Push extra edit to verify bounded cap (max 3)
        hist.pushUndo(systems::CommandHistory::capture(s, sel, "Delete Chair"));
        s.removeObject(id1);
        const bool capped = (hist.getUndoCount() == 3);

        // Undo Delete restores object with exact same ObjectId!
        const bool uDel = hist.undo(s, sel, catalog, &desc);
        const bool restoredId = uDel && (s.findObject(id1) != nullptr) &&
                                nearEq(s.findObject(id1)->transform.scale.x, 1.5f);

        check("M8-T31 Bounded CommandHistory Undo/Redo (Add/Move/Rotate/Scale/Delete, redo invalidation, cap)",
              afterU1 && afterR1 && redoCleared && capped && restoredId);
    }

    std::filesystem::remove_all(tmpDir, ec);

    std::cout << "=======================================================================================" << std::endl;
    if (g_failures == 0) {
        std::cout << "[M8-PERSISTENCE-TESTS] ALL PASSED (0 failures)." << std::endl;
        return 0;
    }
    std::cout << "[M8-PERSISTENCE-TESTS] FAILED (" << g_failures << " failures)." << std::endl;
    return 1;
}
