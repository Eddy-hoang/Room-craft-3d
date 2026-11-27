#include "roomcraft/scene/Room.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

namespace {

bool approx(float a, float b, float eps = 1e-4f)
{
    return std::abs(a - b) <= eps;
}

bool check(bool cond, const char* label, int& failures)
{
    std::cout << "  [Room] " << label << ": " << (cond ? "PASS" : "FAIL") << std::endl;
    if (!cond) ++failures;
    return cond;
}

} // namespace

int runRoomTests()
{
    int failures = 0;
    roomcraft::scene::Room room;
    const auto& s = room.getSettings();

    // TC-M4-02: Default room dimensions (5 x 6 x 3 m, wall 0.15 m, floor 0.10 m, grid 0.5 m)
    check(approx(s.width, 5.0f) && approx(s.length, 6.0f) && approx(s.height, 3.0f) &&
          approx(s.wallThickness, 0.15f) && approx(s.floorThickness, 0.10f) &&
          approx(s.gridSpacing, 0.50f) && approx(s.getFloorArea(), 30.0f) && approx(s.getVolume(), 90.0f),
          "TC-M4-02 Default room dimensions (5x6x3m, area=30, vol=90)", failures);

    // TC-M4-03: Room interior bounds X in [-2.5, 2.5], Y in [0, 3], Z in [-3, 3]
    const auto ib = room.getInteriorBounds();
    check(ib.isValid() && approx(ib.min.x, -2.5f) && approx(ib.max.x, 2.5f) &&
          approx(ib.min.y, 0.0f) && approx(ib.max.y, 3.0f) &&
          approx(ib.min.z, -3.0f) && approx(ib.max.z, 3.0f),
          "TC-M4-03 Room interior bounds match [-W/2, W/2] x [0, H] x [-L/2, L/2]", failures);

    // TC-M4-04: Floor geometry top at y = 0, thickness downward to y = -0.10
    const auto& fb = room.getFloorMesh().boundingBox;
    check(room.getFloorMesh().validate() &&
          approx(fb.max.y, 0.0f) && approx(fb.min.y, -0.10f) &&
          approx(fb.max.x - fb.min.x, 5.30f) && approx(fb.max.z - fb.min.z, 6.30f),
          "TC-M4-04 Floor geometry top at y=0 and extends downward by floorThickness", failures);

    // TC-M4-05: 4 walls built OUTSIDE interior bounds with exact height & zero corner gaps
    const auto& nb = room.getNorthWallMesh().boundingBox;
    const auto& sb = room.getSouthWallMesh().boundingBox;
    const auto& wb = room.getWestWallMesh().boundingBox;
    const auto& eb = room.getEastWallMesh().boundingBox;
    const bool wallsOk =
        room.getNorthWallMesh().validate() && room.getSouthWallMesh().validate() &&
        room.getWestWallMesh().validate() && room.getEastWallMesh().validate() &&
        approx(nb.min.y, 0.0f) && approx(nb.max.y, 3.0f) &&
        approx(nb.max.z, -3.0f) && approx(nb.min.z, -3.15f) && approx(nb.max.x - nb.min.x, 5.30f) &&
        approx(sb.min.z, 3.0f) && approx(sb.max.z, 3.15f) && approx(sb.max.x - sb.min.x, 5.30f) &&
        approx(wb.max.x, -2.5f) && approx(wb.min.x, -2.65f) && approx(wb.max.z - wb.min.z, 6.0f) &&
        approx(eb.min.x, 2.5f) && approx(eb.max.x, 2.65f) && approx(eb.max.z - eb.min.z, 6.0f);
    check(wallsOk, "TC-M4-05 Four walls outside interior bounds with exact dimensions & sealed corners", failures);

    // TC-M4-06: Room resize updates geometryRevision; color/visibility change does NOT rebuild mesh
    const auto revBefore = room.getGeometryRevision();
    auto colorOnly = room.getSettings();
    colorOnly.floorColor = glm::vec4(0.3f, 0.4f, 0.5f, 1.0f);
    colorOnly.wallVisibilityMode = roomcraft::scene::WallVisibilityMode::AllWalls;
    const bool colorApplied = room.applySettings(colorOnly);
    const bool noRebuildOnColor = colorApplied && (room.getGeometryRevision() == revBefore);

    auto resized = room.getSettings();
    resized.width = 7.0f;
    resized.length = 8.0f;
    resized.height = 3.5f;
    const bool resizeOk = room.applySettings(resized);
    const auto ib2 = room.getInteriorBounds();
    check(noRebuildOnColor && resizeOk && (room.getGeometryRevision() == revBefore + 1) &&
          approx(ib2.min.x, -3.5f) && approx(ib2.max.x, 3.5f) &&
          approx(ib2.min.z, -4.0f) && approx(ib2.max.z, 4.0f) && approx(ib2.max.y, 3.5f),
          "TC-M4-06 Room resize rebuilds geometry; color/visibility changes skip rebuild", failures);

    // TC-M4-07: Invalid room parameters rejected transactionally (prior settings preserved)
    std::string err;
    auto badNeg = room.getSettings();
    badNeg.width = -3.0f;
    auto badZero = room.getSettings();
    badZero.height = 0.0f;
    auto badNan = room.getSettings();
    badNan.length = std::numeric_limits<float>::quiet_NaN();
    const bool rejected = !room.applySettings(badNeg, &err) &&
                          !room.applySettings(badZero, &err) &&
                          !room.applySettings(badNan, &err) &&
                          approx(room.getSettings().width, 7.0f) &&
                          approx(room.getSettings().length, 8.0f);
    check(rejected, "TC-M4-07 Invalid room parameters (negative, zero, NaN) rejected & state preserved", failures);

    // TC-M4-08: Wall visibility modes & manual toggles do not alter interior bounds
    auto vis = room.getSettings();
    vis.wallVisibilityMode = roomcraft::scene::WallVisibilityMode::CutawayView;
    room.applySettings(vis);
    const bool cutawayOk = room.getSettings().isNorthWallVisible() && room.getSettings().isWestWallVisible() &&
                           !room.getSettings().isSouthWallVisible() && !room.getSettings().isEastWallVisible() &&
                           !room.getSettings().isCeilingVisible();
    vis.wallVisibilityMode = roomcraft::scene::WallVisibilityMode::Manual;
    vis.showNorthWall = false;
    vis.showCeiling = true;
    room.applySettings(vis);
    const bool manualOk = !room.getSettings().isNorthWallVisible() && room.getSettings().isCeilingVisible() &&
                          approx(room.getInteriorBounds().max.x, 3.5f);
    check(cutawayOk && manualOk, "TC-M4-08 Wall visibility modes & manual toggles preserve interior bounds", failures);

    return failures;
}
