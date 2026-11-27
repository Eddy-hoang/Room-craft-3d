"""Generate 03 — Stylized Gaming / Ergonomic Chair (target budget <= 8,000 triangles)."""

import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import common


def build():
    common.reset_scene()
    out_dir = os.path.join(common.MODELS_EXPORT_DIR, "gaming_chair")

    fabric_tex = common.ensure_procedural_texture(
        os.path.join(out_dir, "chair_charcoal.png"), kind="fabric", base_rgb=(46, 50, 58)
    )

    mat_dark = common.create_material("GamingChairCharcoal", (0.20, 0.22, 0.26, 1.0), roughness=0.62, texture_path=fabric_tex)
    mat_accent = common.create_material("GamingChairAccentBlue", (0.45, 0.58, 0.84, 1.0), roughness=0.55)
    mat_metal = common.create_material("GamingChairMetal", (0.16, 0.18, 0.21, 1.0), roughness=0.35, metallic=0.6)

    # 5-star caster base + 5 wheels (Z = 0 at bottom of wheels)
    wheel_r = 0.028
    hub_z = 0.095
    common.add_cylinder("BaseHub", 0.048, 0.042, 0.08, 16, (0.0, 0.0, hub_z), mat_metal)

    for i in range(5):
        angle = i * (2.0 * math.pi / 5.0)
        arm_len = 0.31
        cx = math.cos(angle) * (arm_len * 0.5)
        cy = math.sin(angle) * (arm_len * 0.5)
        common.add_box(
            f"StarArm_{i}",
            (arm_len, 0.036, 0.032),
            (cx, cy, hub_z - 0.015),
            mat_metal,
            rotation=(0.0, 0.0, angle),
            bevel_width=0.006,
            bevel_segments=1,
        )
        wx = math.cos(angle) * (arm_len - 0.015)
        wy = math.sin(angle) * (arm_len - 0.015)
        common.add_cylinder(
            f"CasterWheel_{i}",
            wheel_r,
            wheel_r,
            0.038,
            12,
            (wx, wy, wheel_r),
            mat_metal,
            rotation=(math.radians(90.0), 0.0, angle),
        )

    # Gas lift piston cylinder
    common.add_cylinder("GasLift", 0.028, 0.024, 0.32, 16, (0.0, 0.0, 0.26), mat_metal)
    common.add_box("SeatMechanism", (0.22, 0.24, 0.05), (0.0, 0.0, 0.42), mat_metal, bevel_width=0.008, bevel_segments=1)

    # Seat base cushion + raised side bolsters
    seat_z = 0.475
    common.add_box("SeatCenter", (0.48, 0.48, 0.085), (0.0, 0.0, seat_z), mat_dark, bevel_width=0.025, bevel_segments=2)
    for side, sx in (("Left", -0.245), ("Right", 0.245)):
        common.add_box(
            f"SeatBolster_{side}",
            (0.085, 0.46, 0.115),
            (sx, 0.0, seat_z + 0.025),
            mat_accent,
            rotation=(0.0, math.radians(-14.0 if sx < 0 else 14.0), 0.0),
            bevel_width=0.025,
            bevel_segments=2,
        )

    # Tall sculpted backrest (+Y is rear, reclined 9 deg)
    recline = math.radians(-9.0)
    common.add_box(
        "BackrestCenter",
        (0.42, 0.09, 0.74),
        (0.0, 0.24, 0.87),
        mat_dark,
        rotation=(recline, 0.0, 0.0),
        bevel_width=0.028,
        bevel_segments=2,
    )
    for side, sx in (("Left", -0.22), ("Right", 0.22)):
        common.add_box(
            f"BackBolster_{side}",
            (0.10, 0.11, 0.66),
            (sx, 0.21, 0.85),
            mat_accent,
            rotation=(recline, 0.0, math.radians(18.0 if sx < 0 else -18.0)),
            bevel_width=0.028,
            bevel_segments=2,
        )

    # Lumbar support & headrest pillows
    common.add_box(
        "LumbarPillow",
        (0.32, 0.08, 0.16),
        (0.0, 0.16, 0.62),
        mat_accent,
        rotation=(recline, 0.0, 0.0),
        bevel_width=0.032,
        bevel_segments=2,
    )
    common.add_box(
        "HeadrestPillow",
        (0.26, 0.08, 0.14),
        (0.0, 0.22, 1.13),
        mat_accent,
        rotation=(recline, 0.0, 0.0),
        bevel_width=0.03,
        bevel_segments=2,
    )

    # Two T-shaped armrests
    for side, sx in (("Left", -0.305), ("Right", 0.305)):
        common.add_box(f"ArmPost_{side}", (0.04, 0.05, 0.22), (sx, 0.02, 0.56), mat_metal, bevel_width=0.008, bevel_segments=1)
        common.add_box(f"ArmPad_{side}", (0.085, 0.27, 0.035), (sx, -0.01, 0.68), mat_dark, bevel_width=0.012, bevel_segments=2)

    return common.save_and_export("gaming_chair", "gaming_chair")


if __name__ == "__main__":
    build()
