"""Generate 01 — Modern Scandinavian Sofa (target budget <= 5,000 triangles)."""

import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import common


def build():
    common.reset_scene()
    out_dir = os.path.join(common.MODELS_EXPORT_DIR, "sofa")

    fabric_tex = common.ensure_procedural_texture(
        os.path.join(out_dir, "sofa_fabric_beige.png"), kind="fabric", base_rgb=(214, 203, 186)
    )
    wood_tex = common.ensure_procedural_texture(
        os.path.join(out_dir, "oak_wood.png"), kind="wood", base_rgb=(176, 134, 92)
    )

    mat_fabric = common.create_material("SofaFabricBeige", (0.84, 0.80, 0.73, 1.0), roughness=0.85, texture_path=fabric_tex)
    mat_cushion = common.create_material("SofaCushionCream", (0.89, 0.85, 0.78, 1.0), roughness=0.85)
    mat_pillow_a = common.create_material("PillowSage", (0.48, 0.62, 0.55, 1.0), roughness=0.80)
    mat_pillow_b = common.create_material("PillowBlue", (0.45, 0.58, 0.76, 1.0), roughness=0.80)
    mat_wood = common.create_material("SofaOakWood", (0.69, 0.52, 0.36, 1.0), roughness=0.55, texture_path=wood_tex)

    # Dimensions (Blender Z-up, front faces -Y so OBJ export maps front to +Z):
    # Width = 2.10m, Depth = 0.90m, Height = 0.85m
    leg_h = 0.16
    for sx in (-0.92, 0.92):
        for sy in (-0.34, 0.34):
            tilt_x = math.radians(6.0 if sy < 0 else -6.0)
            tilt_y = math.radians(-6.0 if sx < 0 else 6.0)
            common.add_cylinder(
                f"SofaLeg_{sx:.2f}_{sy:.2f}",
                radius_bottom=0.022,
                radius_top=0.032,
                depth=leg_h,
                segments=12,
                location=(sx, sy, leg_h * 0.5),
                rotation=(tilt_x, tilt_y, 0.0),
                material=mat_wood,
            )

    # Lower oak apron frame
    common.add_box("SofaWoodFrame", (2.04, 0.84, 0.08), (0.0, 0.0, leg_h + 0.04), mat_wood, bevel_width=0.01, bevel_segments=2)

    # Main upholstered base deck
    common.add_box("SofaBaseDeck", (2.06, 0.86, 0.12), (0.0, 0.0, leg_h + 0.14), mat_fabric, bevel_width=0.02, bevel_segments=2)

    # Left & right beveled armrests
    for side, sx in (("Left", -0.96), ("Right", 0.96)):
        common.add_box(
            f"SofaArm_{side}",
            (0.16, 0.86, 0.44),
            (sx, 0.0, leg_h + 0.36),
            mat_fabric,
            bevel_width=0.03,
            bevel_segments=2,
        )

    # Backrest structural shell (+Y is rear)
    common.add_box("SofaBackFrame", (1.84, 0.16, 0.56), (0.0, 0.35, leg_h + 0.44), mat_fabric, bevel_width=0.03, bevel_segments=2)

    # Two plush beveled seat cushions
    for side, sx in (("Left", -0.44), ("Right", 0.44)):
        common.add_box(
            f"SofaSeatCushion_{side}",
            (0.86, 0.72, 0.14),
            (sx, -0.05, leg_h + 0.27),
            mat_cushion,
            bevel_width=0.035,
            bevel_segments=2,
        )

    # Two slightly reclined backrest cushions
    for side, sx in (("Left", -0.44), ("Right", 0.44)):
        common.add_box(
            f"SofaBackCushion_{side}",
            (0.84, 0.16, 0.38),
            (sx, 0.20, leg_h + 0.52),
            mat_cushion,
            rotation=(math.radians(-10.0), 0.0, 0.0),
            bevel_width=0.038,
            bevel_segments=2,
        )

    # Two accent throw pillows
    common.add_box(
        "SofaPillow_Left",
        (0.34, 0.11, 0.32),
        (-0.70, 0.06, leg_h + 0.48),
        mat_pillow_a,
        rotation=(math.radians(-16.0), math.radians(12.0), math.radians(-18.0)),
        bevel_width=0.035,
        bevel_segments=2,
    )
    common.add_box(
        "SofaPillow_Right",
        (0.34, 0.11, 0.32),
        (0.70, 0.06, leg_h + 0.48),
        mat_pillow_b,
        rotation=(math.radians(-16.0), math.radians(-12.0), math.radians(18.0)),
        bevel_width=0.035,
        bevel_segments=2,
    )

    return common.save_and_export("sofa", "modern_sofa")


if __name__ == "__main__":
    build()
