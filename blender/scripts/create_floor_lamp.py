"""Generate 06 — Modern Tripod Floor Lamp (target <= 2,000 triangles)."""

import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import common


def build():
    common.reset_scene()
    out_dir = os.path.join(common.MODELS_EXPORT_DIR, "floor_lamp")

    wood_tex = common.ensure_procedural_texture(
        os.path.join(out_dir, "lamp_oak.png"), kind="wood", base_rgb=(176, 134, 90)
    )
    shade_tex = common.ensure_procedural_texture(
        os.path.join(out_dir, "lamp_linen_cream.png"), kind="fabric", base_rgb=(238, 230, 212)
    )

    mat_wood = common.create_material("FloorLampOak", (0.69, 0.52, 0.35, 1.0), roughness=0.55, texture_path=wood_tex)
    mat_brass = common.create_material("FloorLampBrass", (0.80, 0.66, 0.38, 1.0), roughness=0.30, metallic=0.8)
    mat_shade = common.create_material("FloorLampLinenShade", (0.93, 0.90, 0.83, 1.0), roughness=0.88, texture_path=shade_tex)
    mat_bulb = common.create_material("FloorLampBulb", (0.98, 0.94, 0.78, 1.0), roughness=0.20)

    # 3 splayed Scandinavian oak tripod legs meeting at collar z = 1.24m
    collar_z = 1.24
    leg_len = 1.28
    splay_r = 0.24
    for i in range(3):
        ang = i * (2.0 * math.pi / 3.0) + math.radians(30.0)
        foot_x = math.cos(ang) * splay_r
        foot_y = math.sin(ang) * splay_r
        mid_x = foot_x * 0.55
        mid_y = foot_y * 0.55
        tilt = math.atan2(splay_r * 0.82, collar_z)
        common.add_cylinder(
            f"TripodLeg_{i}",
            0.013,
            0.018,
            leg_len,
            12,
            (mid_x, mid_y, collar_z * 0.5),
            mat_wood,
            rotation=(math.sin(ang) * tilt, -math.cos(ang) * tilt, 0.0),
        )

    # Brass collar + upper stem + inner bulb
    common.add_cylinder("BrassCollar", 0.044, 0.044, 0.055, 18, (0.0, 0.0, collar_z), mat_brass)
    common.add_cylinder("UpperStem", 0.012, 0.012, 0.20, 14, (0.0, 0.0, collar_z + 0.11), mat_brass)
    common.add_uv_sphere("BulbSphere", 0.042, 14, 10, (0.0, 0.0, collar_z + 0.22), mat_bulb)

    # Tapered drum lampshade (open top & bottom)
    common.add_cylinder(
        "LinenShade",
        radius_bottom=0.23,
        radius_top=0.18,
        depth=0.34,
        segments=28,
        location=(0.0, 0.0, collar_z + 0.24),
        material=mat_shade,
        cap_ends=False,
    )

    return common.save_and_export("floor_lamp", "floor_lamp")


if __name__ == "__main__":
    build()
