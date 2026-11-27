"""Generate 04 — Modern Computer Setup (Monitor, Keyboard, Mouse, PC Case; target <= 3,000 tris)."""

import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import common


def build():
    common.reset_scene()
    out_dir = os.path.join(common.MODELS_EXPORT_DIR, "computer")

    screen_tex = common.ensure_procedural_texture(
        os.path.join(out_dir, "monitor_screen.png"), kind="screen", base_rgb=(40, 50, 70), size=128
    )

    mat_dark = common.create_material("ComputerMatteDark", (0.16, 0.18, 0.22, 1.0), roughness=0.45)
    mat_silver = common.create_material("ComputerAluminum", (0.72, 0.75, 0.80, 1.0), roughness=0.30, metallic=0.7)
    mat_keycap = common.create_material("KeyboardKeys", (0.28, 0.31, 0.37, 1.0), roughness=0.60)
    mat_accent = common.create_material("ComputerAccentBlue", (0.45, 0.58, 0.84, 1.0), roughness=0.40)
    mat_screen = common.create_material("MonitorScreenUI", (1.0, 1.0, 1.0, 1.0), roughness=0.20, texture_path=screen_tex)

    # 1. Monitor stand + wide slim-bezel display (front faces -Y)
    common.add_box("MonitorBase", (0.24, 0.18, 0.014), (-0.12, 0.08, 0.007), mat_silver, bevel_width=0.004, bevel_segments=1)
    common.add_box(
        "MonitorNeck",
        (0.055, 0.025, 0.24),
        (-0.12, 0.12, 0.13),
        mat_silver,
        rotation=(math.radians(8.0), 0.0, 0.0),
        bevel_width=0.005,
        bevel_segments=1,
    )
    common.add_box("MonitorFrame", (0.62, 0.026, 0.36), (-0.12, 0.08, 0.31), mat_dark, bevel_width=0.008, bevel_segments=2)
    screen_obj = common.add_box("MonitorDisplay", (0.59, 0.006, 0.33), (-0.12, 0.064, 0.312), mat_screen)
    common.apply_planar_top_uv(screen_obj.data, 0.59, 0.33)

    # 2. Desk mousepad + low-profile keyboard + ergonomic mouse
    common.add_box("DeskMat", (0.68, 0.26, 0.004), (-0.10, -0.14, 0.002), mat_dark)
    common.add_box(
        "KeyboardBody",
        (0.36, 0.13, 0.018),
        (-0.18, -0.14, 0.013),
        mat_silver,
        rotation=(math.radians(5.0), 0.0, 0.0),
        bevel_width=0.004,
        bevel_segments=1,
    )
    common.add_box(
        "KeyboardKeyDeck",
        (0.34, 0.11, 0.012),
        (-0.18, -0.14, 0.024),
        mat_keycap,
        rotation=(math.radians(5.0), 0.0, 0.0),
        bevel_width=0.003,
        bevel_segments=1,
    )
    common.add_uv_sphere("MouseBody", 0.036, 16, 10, (0.12, -0.14, 0.018), mat_dark, scale=(0.85, 1.45, 0.55))

    # 3. Compact Scandinavian-minimal PC tower case on right
    common.add_box("PCCaseBody", (0.21, 0.42, 0.40), (0.36, 0.02, 0.21), mat_dark, bevel_width=0.01, bevel_segments=2)
    common.add_box("PCCaseAccentStrip", (0.015, 0.008, 0.34), (0.30, -0.193, 0.21), mat_accent)
    for sx in (0.29, 0.43):
        for sy in (-0.14, 0.18):
            common.add_cylinder(f"PCFoot_{sx:.2f}_{sy:.2f}", 0.014, 0.016, 0.014, 10, (sx, sy, 0.007), mat_silver)

    return common.save_and_export("computer", "computer_setup")


if __name__ == "__main__":
    build()
