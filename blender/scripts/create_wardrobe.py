"""Generate 07 — Scandinavian 3-Door Wardrobe (target <= 3,000 triangles)."""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import common


def build():
    common.reset_scene()
    out_dir = os.path.join(common.MODELS_EXPORT_DIR, "wardrobe")

    wood_tex = common.ensure_procedural_texture(
        os.path.join(out_dir, "wardrobe_oak.png"), kind="wood", base_rgb=(182, 142, 98)
    )

    mat_white = common.create_material("WardrobeMatteWhite", (0.91, 0.90, 0.88, 1.0), roughness=0.55)
    mat_oak = common.create_material("WardrobeOakAccent", (0.71, 0.56, 0.38, 1.0), roughness=0.55, texture_path=wood_tex)
    mat_recess = common.create_material("WardrobeSeamShadow", (0.78, 0.77, 0.75, 1.0), roughness=0.65)

    # Width = 1.40m, Height = 2.04m, Depth = 0.58m (front faces -Y)
    leg_h = 0.12
    for sx in (-0.62, 0.62):
        for sy in (-0.22, 0.22):
            common.add_cylinder(f"WardrobeLeg_{sx:.2f}_{sy:.2f}", 0.022, 0.030, leg_h, 12, (sx, sy, leg_h * 0.5), mat_oak)

    # Oak base plinth
    common.add_box("BasePlinth", (1.40, 0.56, 0.06), (0.0, 0.0, leg_h + 0.03), mat_oak, bevel_width=0.008, bevel_segments=1)

    # Main white cabinet body
    body_h = 1.82
    body_z = leg_h + 0.06 + body_h * 0.5
    common.add_box("CabinetBody", (1.38, 0.54, body_h), (0.0, 0.01, body_z), mat_white, bevel_width=0.012, bevel_segments=2)

    # Subtle oak top crown panel
    common.add_box("TopCrown", (1.42, 0.58, 0.04), (0.0, 0.0, leg_h + 0.06 + body_h + 0.02), mat_oak, bevel_width=0.008, bevel_segments=1)

    # Three distinct front doors with crisp vertical reveals & oak bar handles
    door_w = 0.445
    door_h = body_h - 0.03
    door_x_positions = (-0.455, 0.0, 0.455)
    handle_offsets = (0.175, -0.175, -0.175)

    for idx, (dx, hx_off) in enumerate(zip(door_x_positions, handle_offsets)):
        common.add_box(
            f"Door_{idx + 1}",
            (door_w, 0.024, door_h),
            (dx, -0.275, body_z),
            mat_white if idx != 1 else mat_recess,
            bevel_width=0.006,
            bevel_segments=1,
        )
        common.add_box(
            f"Handle_{idx + 1}",
            (0.020, 0.028, 0.28),
            (dx + hx_off, -0.300, body_z - 0.05),
            mat_oak,
            bevel_width=0.005,
            bevel_segments=1,
        )

    return common.save_and_export("wardrobe", "wardrobe")


if __name__ == "__main__":
    build()
