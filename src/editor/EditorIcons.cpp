#include "roomcraft/editor/EditorIcons.hpp"

#include "roomcraft/editor/EditorTheme.hpp"

#include <cmath>

namespace roomcraft::editor {

void EditorIcons::draw(ImDrawList* dl,
                       EditorIcon icon,
                       ImVec2 c,
                       float size,
                       ImU32 col)
{
    if (dl == nullptr || size <= 2.0f) {
        return;
    }

    const float r = size * 0.44f;
    const float stroke = std::max(1.35f, size * 0.095f);

    switch (icon) {
        case EditorIcon::AppLogo: {
            // Huy hiệu khối lập phương isometric đặc trưng của RoomCraft 3D
            const ImVec2 bgMin(c.x - size * 0.52f, c.y - size * 0.52f);
            const ImVec2 bgMax(c.x + size * 0.52f, c.y + size * 0.52f);
            dl->AddRectFilled(bgMin, bgMax, EditorTheme::toU32(tokens::kPrimary), 4.0f);
            const ImU32 white = IM_COL32(240, 245, 255, 255);
            const ImVec2 top(c.x, c.y - r * 0.85f);
            const ImVec2 left(c.x - r * 0.78f, c.y - r * 0.35f);
            const ImVec2 right(c.x + r * 0.78f, c.y - r * 0.35f);
            const ImVec2 botL(c.x - r * 0.78f, c.y + r * 0.45f);
            const ImVec2 botR(c.x + r * 0.78f, c.y + r * 0.45f);
            const ImVec2 bot(c.x, c.y + r * 0.88f);
            dl->AddQuad(top, right, c, left, white, stroke);
            dl->AddLine(left, botL, white, stroke);
            dl->AddLine(right, botR, white, stroke);
            dl->AddLine(c, bot, white, stroke);
            dl->AddLine(botL, bot, white, stroke);
            dl->AddLine(botR, bot, white, stroke);
            break;
        }
        case EditorIcon::NewRoom: {
            dl->AddRect(ImVec2(c.x - r * 0.75f, c.y - r * 0.85f),
                        ImVec2(c.x + r * 0.75f, c.y + r * 0.85f), col, 2.0f, 0, stroke);
            dl->AddLine(ImVec2(c.x - r * 0.35f, c.y), ImVec2(c.x + r * 0.35f, c.y), col, stroke);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.35f), ImVec2(c.x, c.y + r * 0.35f), col, stroke);
            break;
        }
        case EditorIcon::Select: {
            const ImVec2 p0(c.x - r * 0.55f, c.y - r * 0.80f);
            const ImVec2 p1(c.x - r * 0.55f, c.y + r * 0.75f);
            const ImVec2 p2(c.x - r * 0.05f, c.y + r * 0.32f);
            const ImVec2 p3(c.x + r * 0.68f, c.y + r * 0.25f);
            dl->AddTriangle(p0, p1, p2, col, stroke);
            dl->AddLine(p0, p3, col, stroke);
            dl->AddLine(p3, p2, col, stroke);
            break;
        }
        case EditorIcon::Move: {
            dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x + r, c.y), col, stroke);
            dl->AddLine(ImVec2(c.x, c.y - r), ImVec2(c.x, c.y + r), col, stroke);
            const float a = r * 0.35f;
            dl->AddTriangleFilled(ImVec2(c.x, c.y - r - 1.0f), ImVec2(c.x - a, c.y - r + a), ImVec2(c.x + a, c.y - r + a), col);
            dl->AddTriangleFilled(ImVec2(c.x, c.y + r + 1.0f), ImVec2(c.x - a, c.y + r - a), ImVec2(c.x + a, c.y + r - a), col);
            dl->AddTriangleFilled(ImVec2(c.x - r - 1.0f, c.y), ImVec2(c.x - r + a, c.y - a), ImVec2(c.x - r + a, c.y + a), col);
            dl->AddTriangleFilled(ImVec2(c.x + r + 1.0f, c.y), ImVec2(c.x + r - a, c.y - a), ImVec2(c.x + r - a, c.y + a), col);
            break;
        }
        case EditorIcon::Rotate: {
            dl->PathArcTo(c, r * 0.82f, -2.5f, 1.85f, 16);
            dl->PathStroke(col, 0, stroke);
            const float tipX = c.x + r * 0.82f * std::cos(-2.5f);
            const float tipY = c.y + r * 0.82f * std::sin(-2.5f);
            dl->AddTriangleFilled(ImVec2(tipX - 2.0f, tipY - 3.5f),
                                  ImVec2(tipX + 4.5f, tipY - 1.5f),
                                  ImVec2(tipX + 0.5f, tipY + 3.5f), col);
            break;
        }
        case EditorIcon::Scale: {
            dl->AddLine(ImVec2(c.x - r * 0.78f, c.y + r * 0.78f), ImVec2(c.x + r * 0.78f, c.y - r * 0.78f), col, stroke);
            dl->AddRect(ImVec2(c.x - r * 0.88f, c.y + r * 0.25f), ImVec2(c.x - r * 0.25f, c.y + r * 0.88f), col, 1.0f, 0, stroke);
            dl->AddRectFilled(ImVec2(c.x + r * 0.22f, c.y - r * 0.88f), ImVec2(c.x + r * 0.88f, c.y - r * 0.22f), col, 1.0f);
            break;
        }
        case EditorIcon::Snap: {
            // Biểu tượng nam châm hình chữ U
            dl->PathArcTo(ImVec2(c.x, c.y - r * 0.15f), r * 0.65f, 3.14159f, 6.28318f, 12);
            dl->PathStroke(col, 0, stroke * 1.25f);
            dl->AddLine(ImVec2(c.x - r * 0.65f, c.y - r * 0.15f), ImVec2(c.x - r * 0.65f, c.y + r * 0.78f), col, stroke * 1.25f);
            dl->AddLine(ImVec2(c.x + r * 0.65f, c.y - r * 0.15f), ImVec2(c.x + r * 0.65f, c.y + r * 0.78f), col, stroke * 1.25f);
            break;
        }
        case EditorIcon::Focus: {
            dl->AddCircle(c, r * 0.58f, col, 16, stroke);
            dl->AddCircleFilled(c, r * 0.18f, col);
            dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x - r * 0.60f, c.y), col, stroke);
            dl->AddLine(ImVec2(c.x + r * 0.60f, c.y), ImVec2(c.x + r, c.y), col, stroke);
            dl->AddLine(ImVec2(c.x, c.y - r), ImVec2(c.x, c.y - r * 0.60f), col, stroke);
            dl->AddLine(ImVec2(c.x, c.y + r * 0.60f), ImVec2(c.x, c.y + r), col, stroke);
            break;
        }
        case EditorIcon::Duplicate: {
            dl->AddRect(ImVec2(c.x - r * 0.80f, c.y - r * 0.80f), ImVec2(c.x + r * 0.25f, c.y + r * 0.25f), col, 1.5f, 0, stroke);
            dl->AddRect(ImVec2(c.x - r * 0.20f, c.y - r * 0.20f), ImVec2(c.x + r * 0.85f, c.y + r * 0.85f), col, 1.5f, 0, stroke);
            break;
        }
        case EditorIcon::Delete: {
            dl->AddLine(ImVec2(c.x - r * 0.80f, c.y - r * 0.55f), ImVec2(c.x + r * 0.80f, c.y - r * 0.55f), col, stroke);
            dl->AddLine(ImVec2(c.x - r * 0.30f, c.y - r * 0.82f), ImVec2(c.x + r * 0.30f, c.y - r * 0.82f), col, stroke);
            dl->AddRect(ImVec2(c.x - r * 0.58f, c.y - r * 0.55f), ImVec2(c.x + r * 0.58f, c.y + r * 0.82f), col, 1.5f, 0, stroke);
            dl->AddLine(ImVec2(c.x - r * 0.18f, c.y - r * 0.25f), ImVec2(c.x - r * 0.18f, c.y + r * 0.52f), col, stroke * 0.85f);
            dl->AddLine(ImVec2(c.x + r * 0.18f, c.y - r * 0.25f), ImVec2(c.x + r * 0.18f, c.y + r * 0.52f), col, stroke * 0.85f);
            break;
        }
        case EditorIcon::EyeOpen: {
            dl->AddCircle(c, r * 0.28f, col, 12, stroke);
            dl->AddBezierQuadratic(ImVec2(c.x - r * 0.92f, c.y), ImVec2(c.x, c.y - r * 0.95f), ImVec2(c.x + r * 0.92f, c.y), col, stroke);
            dl->AddBezierQuadratic(ImVec2(c.x - r * 0.92f, c.y), ImVec2(c.x, c.y + r * 0.95f), ImVec2(c.x + r * 0.92f, c.y), col, stroke);
            break;
        }
        case EditorIcon::EyeClosed: {
            dl->AddBezierQuadratic(ImVec2(c.x - r * 0.85f, c.y), ImVec2(c.x, c.y + r * 0.65f), ImVec2(c.x + r * 0.85f, c.y), col, stroke);
            dl->AddLine(ImVec2(c.x - r * 0.78f, c.y - r * 0.70f), ImVec2(c.x + r * 0.78f, c.y + r * 0.70f), col, stroke);
            break;
        }
        case EditorIcon::LockClosed: {
            dl->PathArcTo(ImVec2(c.x, c.y - r * 0.25f), r * 0.42f, 3.14159f, 6.28318f, 10);
            dl->PathStroke(col, 0, stroke);
            dl->AddRectFilled(ImVec2(c.x - r * 0.64f, c.y - r * 0.12f), ImVec2(c.x + r * 0.64f, c.y + r * 0.82f), col, 2.0f);
            break;
        }
        case EditorIcon::LockOpen: {
            dl->PathArcTo(ImVec2(c.x + r * 0.12f, c.y - r * 0.35f), r * 0.40f, 3.14159f, 5.75f, 10);
            dl->PathStroke(col, 0, stroke);
            dl->AddRect(ImVec2(c.x - r * 0.62f, c.y - r * 0.08f), ImVec2(c.x + r * 0.62f, c.y + r * 0.80f), col, 2.0f, 0, stroke);
            break;
        }
        case EditorIcon::Search: {
            const ImVec2 lens(c.x - r * 0.18f, c.y - r * 0.18f);
            dl->AddCircle(lens, r * 0.56f, col, 14, stroke);
            dl->AddLine(ImVec2(lens.x + r * 0.38f, lens.y + r * 0.38f), ImVec2(c.x + r * 0.85f, c.y + r * 0.85f), col, stroke * 1.2f);
            break;
        }
        case EditorIcon::Settings: {
            dl->AddCircle(c, r * 0.48f, col, 14, stroke);
            for (int i = 0; i < 6; ++i) {
                const float ang = static_cast<float>(i) * 1.0472f;
                dl->AddLine(ImVec2(c.x + std::cos(ang) * r * 0.52f, c.y + std::sin(ang) * r * 0.52f),
                            ImVec2(c.x + std::cos(ang) * r * 0.90f, c.y + std::sin(ang) * r * 0.90f), col, stroke);
            }
            break;
        }
        case EditorIcon::Camera: {
            // Khối lập phương 3D tinh gọn cho chế độ Perspective 3D
            const ImVec2 top(c.x, c.y - r * 0.88f);
            const ImVec2 left(c.x - r * 0.80f, c.y - r * 0.36f);
            const ImVec2 right(c.x + r * 0.80f, c.y - r * 0.36f);
            const ImVec2 botL(c.x - r * 0.80f, c.y + r * 0.44f);
            const ImVec2 botR(c.x + r * 0.80f, c.y + r * 0.44f);
            const ImVec2 bot(c.x, c.y + r * 0.90f);
            dl->AddQuad(top, right, c, left, col, stroke);
            dl->AddLine(left, botL, col, stroke);
            dl->AddLine(right, botR, col, stroke);
            dl->AddLine(c, bot, col, stroke);
            dl->AddLine(botL, bot, col, stroke);
            dl->AddLine(botR, bot, col, stroke);
            break;
        }
        case EditorIcon::Warning: {
            dl->AddTriangle(ImVec2(c.x, c.y - r * 0.85f), ImVec2(c.x - r * 0.85f, c.y + r * 0.75f), ImVec2(c.x + r * 0.85f, c.y + r * 0.75f), col, stroke);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.32f), ImVec2(c.x, c.y + r * 0.25f), col, stroke);
            dl->AddCircleFilled(ImVec2(c.x, c.y + r * 0.50f), stroke * 0.65f, col);
            break;
        }
        case EditorIcon::Help: {
            dl->AddCircle(c, r * 0.82f, col, 16, stroke);
            dl->AddCircleFilled(ImVec2(c.x, c.y + r * 0.38f), stroke * 0.65f, col);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.35f), ImVec2(c.x, c.y + r * 0.12f), col, stroke);
            break;
        }
        case EditorIcon::Grid: {
            dl->AddRect(ImVec2(c.x - r * 0.80f, c.y - r * 0.80f), ImVec2(c.x + r * 0.80f, c.y + r * 0.80f), col, 1.0f, 0, stroke * 0.9f);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.80f), ImVec2(c.x, c.y + r * 0.80f), col, stroke * 0.85f);
            dl->AddLine(ImVec2(c.x - r * 0.80f, c.y), ImVec2(c.x + r * 0.80f, c.y), col, stroke * 0.85f);
            break;
        }
        case EditorIcon::Wireframe: {
            dl->AddTriangle(ImVec2(c.x, c.y - r * 0.85f), ImVec2(c.x - r * 0.85f, c.y + r * 0.72f), ImVec2(c.x + r * 0.85f, c.y + r * 0.72f), col, stroke);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.85f), ImVec2(c.x, c.y + r * 0.72f), col, stroke * 0.8f);
            break;
        }
        case EditorIcon::Preset: {
            dl->AddRect(ImVec2(c.x - r * 0.85f, c.y - r * 0.65f), ImVec2(c.x + r * 0.85f, c.y + r * 0.75f), col, 1.5f, 0, stroke);
            dl->AddLine(ImVec2(c.x - r * 0.85f, c.y + r * 0.15f), ImVec2(c.x + r * 0.85f, c.y + r * 0.15f), col, stroke * 0.85f);
            dl->AddLine(ImVec2(c.x - r * 0.10f, c.y - r * 0.65f), ImVec2(c.x - r * 0.10f, c.y + r * 0.15f), col, stroke * 0.85f);
            break;
        }
    }
}

bool EditorIcons::toolbarButton(const char* id,
                                EditorIcon icon,
                                const char* label,
                                const char* shortcut,
                                bool active,
                                const char* tooltip,
                                bool compactHorizontal)
{
    ImGui::PushID(id);
    const float iconSize = 14.0f;
    const ImVec2 textSz = ImGui::CalcTextSize(label);
    const float btnW = compactHorizontal ? (iconSize + textSz.x + 22.0f) : std::max(54.0f, textSz.x + 16.0f);
    const float btnH = compactHorizontal ? 28.0f : 42.0f;

    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, tokens::kSelected);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tokens::kPrimary);
        ImGui::PushStyleColor(ImGuiCol_Border, tokens::kPrimary);
    }

    const bool clicked = ImGui::Button("", ImVec2(btnW, btnH));
    const ImVec2 pMin = ImGui::GetItemRectMin();
    const ImVec2 pMax = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 fg = EditorTheme::toU32(active ? tokens::kTextPrimary : tokens::kTextPrimary);

    if (compactHorizontal) {
        draw(dl, icon, ImVec2(pMin.x + 13.0f, (pMin.y + pMax.y) * 0.5f), iconSize,
             EditorTheme::toU32(active ? tokens::kPrimary : tokens::kTextPrimary));
        dl->AddText(ImVec2(pMin.x + 24.0f, (pMin.y + pMax.y - textSz.y) * 0.5f), fg, label);
    } else {
        draw(dl, icon, ImVec2((pMin.x + pMax.x) * 0.5f, pMin.y + 13.0f), iconSize,
             EditorTheme::toU32(active ? tokens::kPrimary : tokens::kTextPrimary));
        dl->AddText(ImVec2((pMin.x + pMax.x - textSz.x) * 0.5f, pMax.y - textSz.y - 4.0f), fg, label);
    }

    if (active) {
        ImGui::PopStyleColor(3);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && tooltip != nullptr) {
        if (shortcut != nullptr && shortcut[0] != '\0') {
            ImGui::SetTooltip("%s (%s)", tooltip, shortcut);
        } else {
            ImGui::SetTooltip("%s", tooltip);
        }
    }

    ImGui::PopID();
    return clicked;
}

bool EditorIcons::smallIconButton(const char* id,
                                  EditorIcon icon,
                                  bool highlighted,
                                  const char* tooltip,
                                  ImVec4 iconTint)
{
    ImGui::PushID(id);
    if (highlighted) {
        ImGui::PushStyleColor(ImGuiCol_Button, tokens::kSelected);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    }
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    const bool clicked = ImGui::Button("", ImVec2(22.0f, 22.0f));
    const ImVec2 pMin = ImGui::GetItemRectMin();
    const ImVec2 pMax = ImGui::GetItemRectMax();
    draw(ImGui::GetWindowDrawList(), icon,
         ImVec2((pMin.x + pMax.x) * 0.5f, (pMin.y + pMax.y) * 0.5f),
         12.0f, EditorTheme::toU32(iconTint));

    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && tooltip != nullptr) {
        ImGui::SetTooltip("%s", tooltip);
    }
    ImGui::PopID();
    return clicked;
}

} // namespace roomcraft::editor
