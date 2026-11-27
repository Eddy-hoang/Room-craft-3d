"""Generate 02 — Modern Scandinavian Bed (target budget <= 4,000 triangles)."""

import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import common


def build():
    common.reset_scene()
    out_dir = os.path.join(common.MODELS_EXPORT_DIR, "bed")

    wood_tex = common.ensure_procedural_texture(
        os.path.join(out_dir, "bed_oak.png"), kind="wood", base_rgb=(184, 144, 102)
    )
    duvet_tex = common.ensure_procedural_texture(
        os.path.join(out_dir, "bed_linen_blue.png"), kind="fabric", base_rgb=(136, 158, 184)
    )

    mat_wood = common.create_material("BedOakWood", (0.72, 0.56, 0.40, 1.0), roughness=0.55, texture_path=wood_tex)
    mat_mattress = common.create_material("BedMattressWhite", (0.93, 0.92, 0.90, 1.0), roughness=0.85)
    mat_duvet = common.create_material("BedDuvetLinen", (0.53, 0.62, 0.72, 1.0), roughness=0.85, texture_path=duvet_tex)
    mat_pillow = common.create_material("BedPillowCream", (0.95, 0.94, 0.91, 1.0), roughness=0.82)

    # Width = 1.74m, Length = 2.14m (+Y headboard, -Y foot)
    leg_h = 0.18
    for sx in (-0.80, 0.80):
        for sy in (-0.98, 0.98):
            common.add_cylinder(
                f"BedLeg_{sx:.2f}_{sy:.2f}",
                radius_bottom=0.028,
                radius_top=0.036,
                depth=leg_h,
                segments=12,
                location=(sx, sy, leg_h * 0.5),
                material=mat_wood,
            )

    # Main wooden bed platform frame
    common.add_box("BedFrameBase", (1.72, 2.10, 0.16), (0.0, 0.0, leg_h + 0.08), mat_wood, bevel_width=0.015, bevel_segments=2)

    # Tall minimalist Scandinavian headboard (+Y)
    common.add_box("BedHeadboard", (1.74, 0.07, 0.88), (0.0, 1.03, leg_h + 0.44), mat_wood, bevel_width=0.018, bevel_segments=2)
    # Subtle upholstered headboard cushion insert
    common.add_box("BedHeadboardPad", (1.58, 0.05, 0.52), (0.0, 0.98, leg_h + 0.56), mat_duvet, bevel_width=0.02, bevel_segments=2)

    # Thick beveled mattress
    common.add_box("BedMattress", (1.62, 1.98, 0.22), (0.0, -0.02, leg_h + 0.25), mat_mattress, bevel_width=0.04, bevel_segments=2)

    # Folded duvet / blanket covering lower 70% of mattress
    common.add_box("BedDuvetMain", (1.66, 1.38, 0.10), (0.0, -0.34, leg_h + 0.34), mat_duvet, bevel_width=0.035, bevel_segments=2)
    common.add_box("BedDuvetFold", (1.66, 0.26, 0.05), (0.0, 0.22, leg_h + 0.38), mat_mattress, bevel_width=0.02, bevel_segments=2)

    # Two soft pillows at headboard
    for side, sx in (("Left", -0.40), ("Right", 0.40)):
        common.add_box(
            f"BedPillow_{side}",
            (0.64, 0.38, 0.14),
            (sx, 0.68, leg_h + 0.43),
            mat_pillow,
            rotation=(math.radians(-12.0), 0.0, 0.0),
            bevel_width=0.05,
            bevel_segments=2,
        )

    return common.save_and_export("bed", "modern_bed")


if __name__ == "__main__":
    build()
