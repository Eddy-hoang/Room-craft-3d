"""Generate 08 — Minimalist Scandinavian Woven Carpet (target <= 500 triangles)."""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import common


def build():
    common.reset_scene()
    out_dir = os.path.join(common.MODELS_EXPORT_DIR, "carpet")

    rug_tex = common.ensure_procedural_texture(
        os.path.join(out_dir, "carpet_weave_beige.png"), kind="carpet", base_rgb=(218, 208, 192), size=128
    )
    mat_rug = common.create_material("ScandiRugWeave", (0.86, 0.82, 0.76, 1.0), roughness=0.92, texture_path=rug_tex)

    # Thin beveled rectangular rug (2.20m x 1.50m x 0.016m) with clean planar top UV
    rug = common.add_box(
        "MinimalistCarpet",
        (2.20, 1.50, 0.016),
        (0.0, 0.0, 0.008),
        mat_rug,
        bevel_width=0.006,
        bevel_segments=1,
    )
    common.apply_planar_top_uv(rug.data, 2.20, 1.50)

    return common.save_and_export("carpet", "minimalist_carpet")


if __name__ == "__main__":
    build()
