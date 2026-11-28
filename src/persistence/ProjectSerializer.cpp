#include "roomcraft/persistence/ProjectSerializer.hpp"

#include "roomcraft/persistence/Json.hpp"

#include <charconv>
#include <cmath>
#include <limits>

namespace roomcraft::persistence {

namespace {

JsonValue encodeVec3(const glm::vec3& v)
{
    JsonValue arr = JsonValue::makeArray();
    arr.pushBack(static_cast<double>(v.x));
    arr.pushBack(static_cast<double>(v.y));
    arr.pushBack(static_cast<double>(v.z));
    return arr;
}

JsonValue encodeVec4(const glm::vec4& v)
{
    JsonValue arr = JsonValue::makeArray();
    arr.pushBack(static_cast<double>(v.r));
    arr.pushBack(static_cast<double>(v.g));
    arr.pushBack(static_cast<double>(v.b));
    arr.pushBack(static_cast<double>(v.a));
    return arr;
}

std::string wallModeToJsonString(scene::WallVisibilityMode mode)
{
    switch (mode) {
        case scene::WallVisibilityMode::AllWalls:    return "AllWalls";
        case scene::WallVisibilityMode::HideCeiling: return "HideCeiling";
        case scene::WallVisibilityMode::CutawayView: return "CutawayView";
        case scene::WallVisibilityMode::Manual:      return "Manual";
        default:                                     return "CutawayView";
    }
}

bool wallModeFromJsonString(const std::string& s, scene::WallVisibilityMode& out)
{
    if (s == "AllWalls")    { out = scene::WallVisibilityMode::AllWalls; return true; }
    if (s == "HideCeiling") { out = scene::WallVisibilityMode::HideCeiling; return true; }
    if (s == "CutawayView") { out = scene::WallVisibilityMode::CutawayView; return true; }
    if (s == "Manual")      { out = scene::WallVisibilityMode::Manual; return true; }
    return false;
}

std::string lightPresetToJsonString(int presetIndex)
{
    switch (presetIndex) {
        case 0:  return "Daylight";
        case 1:  return "WarmInterior";
        case 2:  return "NeutralStudio";
        case 3:  return "Evening";
        default: return "Custom";
    }
}

bool lightPresetFromJsonString(const std::string& s, int& out)
{
    if (s == "Daylight")      { out = 0; return true; }
    if (s == "WarmInterior")  { out = 1; return true; }
    if (s == "NeutralStudio") { out = 2; return true; }
    if (s == "Evening")       { out = 3; return true; }
    if (s == "Custom")        { out = -1; return true; }
    return false;
}

std::string placementModeToJsonString(systems::PlacementValidationMode mode)
{
    switch (mode) {
        case systems::PlacementValidationMode::Off:            return "Off";
        case systems::PlacementValidationMode::WarningOnly:    return "WarningOnly";
        case systems::PlacementValidationMode::PreventInvalid: return "PreventInvalid";
        default:                                               return "WarningOnly";
    }
}

bool placementModeFromJsonString(const std::string& s, systems::PlacementValidationMode& out)
{
    if (s == "Off")            { out = systems::PlacementValidationMode::Off; return true; }
    if (s == "WarningOnly")    { out = systems::PlacementValidationMode::WarningOnly; return true; }
    if (s == "PreventInvalid") { out = systems::PlacementValidationMode::PreventInvalid; return true; }
    return false;
}

std::string msaaIndexToJsonString(int idx)
{
    if (idx == 1) return "2x";
    if (idx == 2) return "4x";
    return "Off";
}

bool msaaIndexFromJsonString(const std::string& s, int& out)
{
    if (s == "Off") { out = 0; return true; }
    if (s == "2x")  { out = 1; return true; }
    if (s == "4x")  { out = 2; return true; }
    return false;
}

bool readRequiredString(const JsonValue& obj, const std::string& key, std::string& out, std::string& err)
{
    const JsonValue* v = obj.find(key);
    if (v == nullptr || !v->isString()) {
        err = "Missing or invalid string field '" + key + "'.";
        return false;
    }
    out = v->asString();
    return true;
}

bool readRequiredBool(const JsonValue& obj, const std::string& key, bool& out, std::string& err)
{
    const JsonValue* v = obj.find(key);
    if (v == nullptr || !v->isBool()) {
        err = "Missing or invalid boolean field '" + key + "'.";
        return false;
    }
    out = v->asBool();
    return true;
}

bool readRequiredFloat(const JsonValue& obj, const std::string& key, float& out, std::string& err)
{
    const JsonValue* v = obj.find(key);
    if (v == nullptr || !v->isNumber()) {
        err = "Missing or invalid numeric field '" + key + "'.";
        return false;
    }
    const double d = v->asDouble();
    if (!std::isfinite(d) || std::fabs(d) > static_cast<double>(std::numeric_limits<float>::max())) {
        err = "Numeric field '" + key + "' is non-finite or out of float range.";
        return false;
    }
    out = static_cast<float>(d);
    return true;
}

bool readRequiredInt(const JsonValue& obj, const std::string& key, int& out, std::string& err)
{
    const JsonValue* v = obj.find(key);
    if (v == nullptr || !v->isInt64()) {
        err = "Missing or invalid integer field '" + key + "'.";
        return false;
    }
    const std::int64_t iv = v->asInt64();
    if (iv < std::numeric_limits<int>::min() || iv > std::numeric_limits<int>::max()) {
        err = "Integer field '" + key + "' out of range.";
        return false;
    }
    out = static_cast<int>(iv);
    return true;
}

// Giải mã lossless uint64_t: chấp nhận chuỗi thập phân chuẩn ("123") hoặc số nguyên JSON không âm chính xác
bool readRequiredUInt64Id(const JsonValue& obj, const std::string& key, std::uint64_t& out, std::string& err)
{
    const JsonValue* v = obj.find(key);
    if (v == nullptr) {
        err = "Missing required ID field '" + key + "'.";
        return false;
    }
    if (v->isString()) {
        const std::string& s = v->asString();
        if (s.empty()) {
            err = "Empty ID string for field '" + key + "'.";
            return false;
        }
        for (char c : s) {
            if (c < '0' || c > '9') {
                err = "Non-digit character in ID field '" + key + "': '" + s + "'.";
                return false;
            }
        }
        std::uint64_t parsed = 0;
        const auto res = std::from_chars(s.data(), s.data() + s.size(), parsed);
        if (res.ec != std::errc() || res.ptr != s.data() + s.size()) {
            err = "Overflow or invalid uint64 value in field '" + key + "': '" + s + "'.";
            return false;
        }
        out = parsed;
        return true;
    }
    if (v->isUInt64()) {
        out = v->asUInt64();
        return true;
    }
    if (v->isInt64() && v->asInt64() >= 0) {
        out = static_cast<std::uint64_t>(v->asInt64());
        return true;
    }
    err = "Field '" + key + "' must be a decimal string or non-negative integer.";
    return false;
}

bool readRequiredVec3(const JsonValue& obj, const std::string& key, glm::vec3& out, std::string& err)
{
    const JsonValue* v = obj.find(key);
    if (v == nullptr || !v->isArray() || v->asArray().size() != 3) {
        err = "Missing or invalid 3-element numeric array '" + key + "'.";
        return false;
    }
    const auto& arr = v->asArray();
    for (int i = 0; i < 3; ++i) {
        if (!arr[static_cast<std::size_t>(i)].isNumber()) {
            err = "Element " + std::to_string(i) + " of '" + key + "' must be numeric.";
            return false;
        }
        const double d = arr[static_cast<std::size_t>(i)].asDouble();
        if (!std::isfinite(d)) {
            err = "Non-finite numeric value in '" + key + "'.";
            return false;
        }
        out[i] = static_cast<float>(d);
    }
    return true;
}

bool readRequiredVec4(const JsonValue& obj, const std::string& key, glm::vec4& out, std::string& err)
{
    const JsonValue* v = obj.find(key);
    if (v == nullptr || !v->isArray() || v->asArray().size() != 4) {
        err = "Missing or invalid 4-element numeric array '" + key + "'.";
        return false;
    }
    const auto& arr = v->asArray();
    for (int i = 0; i < 4; ++i) {
        if (!arr[static_cast<std::size_t>(i)].isNumber()) {
            err = "Element " + std::to_string(i) + " of '" + key + "' must be numeric.";
            return false;
        }
        const double d = arr[static_cast<std::size_t>(i)].asDouble();
        if (!std::isfinite(d)) {
            err = "Non-finite numeric value in '" + key + "'.";
            return false;
        }
        out[i] = static_cast<float>(d);
    }
    return true;
}

bool readRequiredObject(const JsonValue& parent, const std::string& key, const JsonValue*& out, std::string& err)
{
    const JsonValue* v = parent.find(key);
    if (v == nullptr || !v->isObject()) {
        err = "Missing or invalid JSON object '" + key + "'.";
        return false;
    }
    out = v;
    return true;
}

} // namespace

bool ProjectSerializer::toJsonString(const ProjectData& data,
                                     std::string& outJson,
                                     std::string* outError)
{
    if (data.format != kProjectFormatId || data.schemaVersion != kCurrentSchemaVersion) {
        if (outError != nullptr) {
            *outError = "Cannot serialize unsupported project format/schema version.";
        }
        return false;
    }

    JsonValue root = JsonValue::makeObject();
    root.set("format", data.format);
    root.set("schemaVersion", data.schemaVersion);
    root.set("projectName", data.projectName);
    root.set("nextObjectId", std::to_string(data.nextObjectId));

    // 1. Room & Materials
    JsonValue room = JsonValue::makeObject();
    room.set("width", data.room.width);
    room.set("length", data.room.length);
    room.set("height", data.room.height);
    room.set("wallThickness", data.room.wallThickness);
    room.set("floorThickness", data.room.floorThickness);
    room.set("gridSpacing", data.room.gridSpacing);
    room.set("floorColor", encodeVec4(data.room.floorColor));
    room.set("wallColor", encodeVec4(data.room.wallColor));
    room.set("ceilingColor", encodeVec4(data.room.ceilingColor));
    room.set("wallVisibilityMode", wallModeToJsonString(data.room.wallVisibilityMode));
    room.set("showNorthWall", data.room.showNorthWall);
    room.set("showSouthWall", data.room.showSouthWall);
    room.set("showEastWall", data.room.showEastWall);
    room.set("showWestWall", data.room.showWestWall);
    room.set("showCeiling", data.room.showCeiling);
    room.set("showGrid", data.room.showGrid);
    room.set("showWindow", data.room.showWindow);
    root.set("room", std::move(room));

    // 2. Scene Objects (IDs dưới dạng chuỗi thập phân lossless)
    JsonValue objs = JsonValue::makeArray();
    for (const auto& obj : data.objects) {
        JsonValue jo = JsonValue::makeObject();
        jo.set("objectId", std::to_string(obj.id));
        jo.set("assetId", obj.assetId);
        jo.set("name", obj.name);
        jo.set("position", encodeVec3(obj.transform.position));
        jo.set("rotationDegrees", encodeVec3(obj.transform.rotationDegrees));
        jo.set("scale", encodeVec3(obj.transform.scale));
        jo.set("visible", obj.visible);
        jo.set("locked", obj.locked);

        JsonValue matOv = JsonValue::makeObject();
        matOv.set("shininessScale", obj.shininessScale);
        jo.set("materialOverrides", std::move(matOv));
        objs.pushBack(std::move(jo));
    }
    root.set("objects", std::move(objs));

    // 3. Complete Lighting Settings (M7)
    JsonValue light = JsonValue::makeObject();
    light.set("preset", lightPresetToJsonString(data.lightingPreset));
    light.set("ambient", data.lighting.ambient);
    light.set("sunEnabled", data.lighting.sunEnabled);
    light.set("sunDirection", encodeVec3(data.lighting.direction));
    light.set("sunColor", encodeVec3(data.lighting.sunColor));
    light.set("sunIntensity", data.lighting.sunIntensity);
    light.set("pointEnabled", data.lighting.pointEnabled);
    light.set("pointPosition", encodeVec3(data.lighting.position));
    light.set("pointColor", encodeVec3(data.lighting.pointColor));
    light.set("pointIntensity", data.lighting.pointIntensity);
    light.set("pointAttenuation", encodeVec3(data.lighting.attenuation));
    light.set("shadowsEnabled", data.lighting.shadowsEnabled);
    light.set("shadowMapSize", data.lighting.shadowMapSize);
    light.set("shadowBias", data.lighting.shadowBias);
    root.set("lighting", std::move(light));

    // 4. Render Settings
    JsonValue render = JsonValue::makeObject();
    render.set("msaa", msaaIndexToJsonString(data.renderSettings.msaaSampleIndex));
    render.set("wireframe", data.renderSettings.wireframe);
    render.set("showAxes", data.renderSettings.showAxes);
    render.set("showSelectedBBox", data.renderSettings.showSelectedBBox);
    render.set("showMoveGizmo", data.renderSettings.showMoveGizmo);
    root.set("renderSettings", std::move(render));

    // 5. Camera State
    JsonValue cam = JsonValue::makeObject();
    cam.set("projectionMode",
            data.camera.projectionMode == graphics::ProjectionMode::Orthographic ? "Orthographic" : "Perspective");
    cam.set("position", encodeVec3(data.camera.position));
    cam.set("target", encodeVec3(data.camera.target));
    cam.set("up", encodeVec3(data.camera.up));
    cam.set("fovDegrees", data.camera.fovDegrees);
    cam.set("nearPlane", data.camera.nearPlane);
    cam.set("farPlane", data.camera.farPlane);
    cam.set("orthoHalfHeight", data.camera.orthoHalfHeight);
    root.set("camera", std::move(cam));

    // 6. Editor & Collision Preferences
    JsonValue editor = JsonValue::makeObject();
    editor.set("showGrid", data.editorSettings.showGrid);
    editor.set("gridSnapEnabled", data.editorSettings.gridSnapEnabled);
    editor.set("placementValidationMode", placementModeToJsonString(data.editorSettings.placementMode));
    editor.set("allowRugOverlap", data.editorSettings.allowRugOverlap);
    root.set("editorSettings", std::move(editor));

    outJson = root.dump(2);
    return true;
}

bool ProjectSerializer::fromJsonString(const std::string& jsonText,
                                       ProjectData& outData,
                                       std::string* outError)
{
    JsonValue root;
    std::string err;
    if (!parseJson(jsonText, root, &err)) {
        if (outError != nullptr) *outError = err;
        return false;
    }
    if (!root.isObject()) {
        if (outError != nullptr) *outError = "Project root must be a JSON object.";
        return false;
    }

    ProjectData parsed;
    if (!readRequiredString(root, "format", parsed.format, err) ||
        !readRequiredInt(root, "schemaVersion", parsed.schemaVersion, err)) {
        if (outError != nullptr) *outError = err;
        return false;
    }

    if (parsed.format != kProjectFormatId) {
        if (outError != nullptr) {
            *outError = "Invalid project format '" + parsed.format + "' (expected 'RoomCraft3D').";
        }
        return false;
    }
    if (parsed.schemaVersion != kCurrentSchemaVersion) {
        if (outError != nullptr) {
            *outError = "Unsupported schemaVersion " + std::to_string(parsed.schemaVersion) +
                        " (only version " + std::to_string(kCurrentSchemaVersion) + " is supported).";
        }
        return false;
    }

    if (!readRequiredString(root, "projectName", parsed.projectName, err) ||
        !readRequiredUInt64Id(root, "nextObjectId", parsed.nextObjectId, err)) {
        if (outError != nullptr) *outError = err;
        return false;
    }

    // 1. Room
    const JsonValue* roomObj = nullptr;
    if (!readRequiredObject(root, "room", roomObj, err)) {
        if (outError != nullptr) *outError = err;
        return false;
    }
    std::string wallModeStr;
    if (!readRequiredFloat(*roomObj, "width", parsed.room.width, err) ||
        !readRequiredFloat(*roomObj, "length", parsed.room.length, err) ||
        !readRequiredFloat(*roomObj, "height", parsed.room.height, err) ||
        !readRequiredFloat(*roomObj, "wallThickness", parsed.room.wallThickness, err) ||
        !readRequiredFloat(*roomObj, "floorThickness", parsed.room.floorThickness, err) ||
        !readRequiredFloat(*roomObj, "gridSpacing", parsed.room.gridSpacing, err) ||
        !readRequiredVec4(*roomObj, "floorColor", parsed.room.floorColor, err) ||
        !readRequiredVec4(*roomObj, "wallColor", parsed.room.wallColor, err) ||
        !readRequiredVec4(*roomObj, "ceilingColor", parsed.room.ceilingColor, err) ||
        !readRequiredString(*roomObj, "wallVisibilityMode", wallModeStr, err) ||
        !readRequiredBool(*roomObj, "showNorthWall", parsed.room.showNorthWall, err) ||
        !readRequiredBool(*roomObj, "showSouthWall", parsed.room.showSouthWall, err) ||
        !readRequiredBool(*roomObj, "showEastWall", parsed.room.showEastWall, err) ||
        !readRequiredBool(*roomObj, "showWestWall", parsed.room.showWestWall, err) ||
        !readRequiredBool(*roomObj, "showCeiling", parsed.room.showCeiling, err) ||
        !readRequiredBool(*roomObj, "showGrid", parsed.room.showGrid, err)) {
        if (outError != nullptr) *outError = "Room section error: " + err;
        return false;
    }
    if (!wallModeFromJsonString(wallModeStr, parsed.room.wallVisibilityMode)) {
        if (outError != nullptr) *outError = "Invalid wallVisibilityMode: '" + wallModeStr + "'.";
        return false;
    }
    // Trường tùy chọn RC2.5: tương thích ngược hoàn toàn với tệp .roomcraft từ M8/RC1
    if (const auto* winVal = roomObj->find("showWindow")) {
        if (!winVal->isBool()) {
            if (outError != nullptr) *outError = "Room section error: invalid 'showWindow'.";
            return false;
        }
        parsed.room.showWindow = winVal->asBool();
    }

    // 2. Objects
    const JsonValue* objsVal = root.find("objects");
    if (objsVal == nullptr || !objsVal->isArray()) {
        if (outError != nullptr) *outError = "Missing or invalid 'objects' array.";
        return false;
    }
    const auto& objArr = objsVal->asArray();
    if (objArr.size() > kMaxProjectObjects) {
        if (outError != nullptr) {
            *outError = "Object count (" + std::to_string(objArr.size()) + ") exceeds limit (" +
                        std::to_string(kMaxProjectObjects) + ").";
        }
        return false;
    }
    parsed.objects.reserve(objArr.size());
    for (std::size_t i = 0; i < objArr.size(); ++i) {
        const auto& jo = objArr[i];
        if (!jo.isObject()) {
            if (outError != nullptr) *outError = "Object entry " + std::to_string(i) + " must be a JSON object.";
            return false;
        }
        scene::SceneObject obj;
        const JsonValue* matOv = nullptr;
        if (!readRequiredUInt64Id(jo, "objectId", obj.id, err) ||
            !readRequiredString(jo, "assetId", obj.assetId, err) ||
            !readRequiredString(jo, "name", obj.name, err) ||
            !readRequiredVec3(jo, "position", obj.transform.position, err) ||
            !readRequiredVec3(jo, "rotationDegrees", obj.transform.rotationDegrees, err) ||
            !readRequiredVec3(jo, "scale", obj.transform.scale, err) ||
            !readRequiredBool(jo, "visible", obj.visible, err) ||
            !readRequiredBool(jo, "locked", obj.locked, err) ||
            !readRequiredObject(jo, "materialOverrides", matOv, err) ||
            !readRequiredFloat(*matOv, "shininessScale", obj.shininessScale, err)) {
            if (outError != nullptr) {
                *outError = "Object entry " + std::to_string(i) + " error: " + err;
            }
            return false;
        }
        parsed.objects.push_back(std::move(obj));
    }

    // 3. Lighting
    const JsonValue* lightObj = nullptr;
    if (!readRequiredObject(root, "lighting", lightObj, err)) {
        if (outError != nullptr) *outError = err;
        return false;
    }
    std::string presetStr;
    if (!readRequiredString(*lightObj, "preset", presetStr, err) ||
        !readRequiredFloat(*lightObj, "ambient", parsed.lighting.ambient, err) ||
        !readRequiredBool(*lightObj, "sunEnabled", parsed.lighting.sunEnabled, err) ||
        !readRequiredVec3(*lightObj, "sunDirection", parsed.lighting.direction, err) ||
        !readRequiredVec3(*lightObj, "sunColor", parsed.lighting.sunColor, err) ||
        !readRequiredFloat(*lightObj, "sunIntensity", parsed.lighting.sunIntensity, err) ||
        !readRequiredBool(*lightObj, "pointEnabled", parsed.lighting.pointEnabled, err) ||
        !readRequiredVec3(*lightObj, "pointPosition", parsed.lighting.position, err) ||
        !readRequiredVec3(*lightObj, "pointColor", parsed.lighting.pointColor, err) ||
        !readRequiredFloat(*lightObj, "pointIntensity", parsed.lighting.pointIntensity, err) ||
        !readRequiredVec3(*lightObj, "pointAttenuation", parsed.lighting.attenuation, err)) {
        if (outError != nullptr) *outError = "Lighting section error: " + err;
        return false;
    }
    if (!lightPresetFromJsonString(presetStr, parsed.lightingPreset)) {
        if (outError != nullptr) *outError = "Invalid lighting preset: '" + presetStr + "'.";
        return false;
    }
    // Các trường mở rộng M9.4 (tương thích ngược hoàn toàn với tệp .roomcraft schema v1 từ M8)
    if (const auto* shEn = lightObj->find("shadowsEnabled")) {
        if (!shEn->isBool()) {
            if (outError != nullptr) *outError = "Lighting section error: invalid 'shadowsEnabled'.";
            return false;
        }
        parsed.lighting.shadowsEnabled = shEn->asBool();
    }
    if (const auto* shSz = lightObj->find("shadowMapSize")) {
        if (!shSz->isInt64()) {
            if (outError != nullptr) *outError = "Lighting section error: invalid 'shadowMapSize'.";
            return false;
        }
        parsed.lighting.shadowMapSize = static_cast<int>(shSz->asInt64());
    }
    if (const auto* shBias = lightObj->find("shadowBias")) {
        if (!shBias->isNumber()) {
            if (outError != nullptr) *outError = "Lighting section error: invalid 'shadowBias'.";
            return false;
        }
        parsed.lighting.shadowBias = static_cast<float>(shBias->asDouble());
    }

    // 4. Render Settings
    const JsonValue* renderObj = nullptr;
    if (!readRequiredObject(root, "renderSettings", renderObj, err)) {
        if (outError != nullptr) *outError = err;
        return false;
    }
    std::string msaaStr;
    if (!readRequiredString(*renderObj, "msaa", msaaStr, err) ||
        !readRequiredBool(*renderObj, "wireframe", parsed.renderSettings.wireframe, err) ||
        !readRequiredBool(*renderObj, "showAxes", parsed.renderSettings.showAxes, err) ||
        !readRequiredBool(*renderObj, "showSelectedBBox", parsed.renderSettings.showSelectedBBox, err) ||
        !readRequiredBool(*renderObj, "showMoveGizmo", parsed.renderSettings.showMoveGizmo, err)) {
        if (outError != nullptr) *outError = "Render settings error: " + err;
        return false;
    }
    if (!msaaIndexFromJsonString(msaaStr, parsed.renderSettings.msaaSampleIndex)) {
        if (outError != nullptr) *outError = "Invalid MSAA mode: '" + msaaStr + "'.";
        return false;
    }

    // 5. Camera
    const JsonValue* camObj = nullptr;
    if (!readRequiredObject(root, "camera", camObj, err)) {
        if (outError != nullptr) *outError = err;
        return false;
    }
    std::string projStr;
    if (!readRequiredString(*camObj, "projectionMode", projStr, err) ||
        !readRequiredVec3(*camObj, "position", parsed.camera.position, err) ||
        !readRequiredVec3(*camObj, "target", parsed.camera.target, err) ||
        !readRequiredVec3(*camObj, "up", parsed.camera.up, err) ||
        !readRequiredFloat(*camObj, "fovDegrees", parsed.camera.fovDegrees, err) ||
        !readRequiredFloat(*camObj, "nearPlane", parsed.camera.nearPlane, err) ||
        !readRequiredFloat(*camObj, "farPlane", parsed.camera.farPlane, err) ||
        !readRequiredFloat(*camObj, "orthoHalfHeight", parsed.camera.orthoHalfHeight, err)) {
        if (outError != nullptr) *outError = "Camera section error: " + err;
        return false;
    }
    if (projStr == "Perspective") {
        parsed.camera.projectionMode = graphics::ProjectionMode::Perspective;
    } else if (projStr == "Orthographic") {
        parsed.camera.projectionMode = graphics::ProjectionMode::Orthographic;
    } else {
        if (outError != nullptr) *outError = "Invalid camera projectionMode: '" + projStr + "'.";
        return false;
    }

    // 6. Editor Settings
    const JsonValue* edObj = nullptr;
    if (!readRequiredObject(root, "editorSettings", edObj, err)) {
        if (outError != nullptr) *outError = err;
        return false;
    }
    std::string modeStr;
    if (!readRequiredBool(*edObj, "showGrid", parsed.editorSettings.showGrid, err) ||
        !readRequiredBool(*edObj, "gridSnapEnabled", parsed.editorSettings.gridSnapEnabled, err) ||
        !readRequiredString(*edObj, "placementValidationMode", modeStr, err) ||
        !readRequiredBool(*edObj, "allowRugOverlap", parsed.editorSettings.allowRugOverlap, err)) {
        if (outError != nullptr) *outError = "Editor settings error: " + err;
        return false;
    }
    if (!placementModeFromJsonString(modeStr, parsed.editorSettings.placementMode)) {
        if (outError != nullptr) *outError = "Invalid placementValidationMode: '" + modeStr + "'.";
        return false;
    }

    outData = std::move(parsed);
    return true;
}

} // namespace roomcraft::persistence
