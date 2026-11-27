"""Generate 05 — Stylized Indoor Plant in Scandinavian Planter (target <= 5,000 tris)."""

import math
import os
import sys
import bpy
import bmesh
from mathutils import Vector, Euler

sys.path.insert(0, os.path.dirname(__file__))
import common


def add_stylized_leaf(name, length, width, location, rotation, material):
    """Create a sculpted low-poly tropical leaf blade."""
    mesh = bpy.data.meshes.new(name + "_Mesh")
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)

    bm = bmesh.new()
    # Diamond-oval curved leaf with central spine fold for crisp silhouette
    v_base = bm.verts.new((0.0, 0.0, 0.0))
    v_l1 = bm.verts.new((-width * 0.42, length * 0.28, 0.025))
    v_m1 = bm.verts.new((0.0, length * 0.30, 0.012))
    v_r1 = bm.verts.new((width * 0.42, length * 0.28, 0.025))
    v_l2 = bm.verts.new((-width * 0.50, length * 0.65, 0.015))
    v_m2 = bm.verts.new((0.0, length * 0.68, -0.005))
    v_r2 = bm.verts.new((width * 0.50, length * 0.65, 0.015))
    v_tip = bm.verts.new((0.0, length, -0.045))

    bm.faces.new((v_base, v_m1, v_l1))
    bm.faces.new((v_base, v_r1, v_m1))
    bm.faces.new((v_l1, v_m1, v_m2, v_l2))
    bm.faces.new((v_m1, v_r1, v_r2, v_m2))
    bm.faces.new((v_l2, v_m2, v_tip))
    bm.faces.new((v_m2, v_r2, v_tip))

    # Extrude slight thickness so leaves read cleanly from all angles
    bmesh.ops.solidify(bm, geom=list(bm.faces), thickness=0.008)
    bm.normal_update()
    bm.to_mesh(mesh)
    bm.free()

    common.apply_box_uv(mesh, uv_scale=2.0)
    obj.location = Vector(location)
    obj.rotation_euler = Euler(rotation, 'XYZ')
    obj.data.materials.append(material)
    return obj


def build():
    common.reset_scene()
    out_dir = os.path.join(common.MODELS_EXPORT_DIR, "plant")

    wood_tex = common.ensure_procedural_texture(
        os.path.join(out_dir, "planter_wood.png"), kind="wood", base_rgb=(178, 138, 96)
    )

    mat_pot = common.create_material("CeramicPotCream", (0.90, 0.88, 0.84, 1.0), roughness=0.45)
    mat_wood = common.create_material("PlanterStandOak", (0.70, 0.54, 0.38, 1.0), roughness=0.55, texture_path=wood_tex)
    mat_soil = common.create_material("PlantSoilDark", (0.24, 0.18, 0.13, 1.0), roughness=0.95)
    mat_stem = common.create_material("PlantStemGreen", (0.22, 0.44, 0.24, 1.0), roughness=0.75)
    mat_leaf_a = common.create_material("LeafGreenRich", (0.24, 0.56, 0.30, 1.0), roughness=0.65)
    mat_leaf_b = common.create_material("LeafGreenFresh", (0.32, 0.64, 0.36, 1.0), roughness=0.65)

    # 1. Scandinavian wooden tripod stand
    for i in range(4):
        ang = i * (math.pi * 0.5) + math.pi * 0.25
        lx = math.cos(ang) * 0.155
        ly = math.sin(ang) * 0.155
        common.add_cylinder(f"StandLeg_{i}", 0.015, 0.017, 0.34, 10, (lx, ly, 0.17), mat_wood)
    common.add_box("StandCrossA", (0.31, 0.026, 0.026), (0.0, 0.0, 0.11), mat_wood, rotation=(0.0, 0.0, math.pi * 0.25))
    common.add_box("StandCrossB", (0.31, 0.026, 0.026), (0.0, 0.0, 0.11), mat_wood, rotation=(0.0, 0.0, -math.pi * 0.25))

    # 2. Ceramic pot + soil disk
    common.add_cylinder("CeramicPot", 0.135, 0.175, 0.30, 24, (0.0, 0.0, 0.27), mat_pot)
    common.add_cylinder("SoilSurface", 0.162, 0.162, 0.02, 20, (0.0, 0.0, 0.415), mat_soil)

    # 3. Organic radiating stems and 10 lush stylized leaves
    leaf_specs = [
        (0.0,   0.42, 0.30, 0.18, 22.0,  0.34, 0.22, mat_leaf_a),
        (52.0,  0.48, 0.34, 0.20, 26.0,  0.32, 0.20, mat_leaf_b),
        (108.0, 0.56, 0.36, 0.16, 18.0,  0.36, 0.23, mat_leaf_a),
        (165.0, 0.46, 0.32, 0.22, 28.0,  0.33, 0.21, mat_leaf_b),
        (220.0, 0.52, 0.35, 0.18, 20.0,  0.35, 0.22, mat_leaf_a),
        (285.0, 0.44, 0.31, 0.24, 30.0,  0.31, 0.20, mat_leaf_b),
        (25.0,  0.66, 0.26, 0.12, 12.0,  0.29, 0.19, mat_leaf_b),
        (140.0, 0.68, 0.25, 0.10, 10.0,  0.28, 0.18, mat_leaf_a),
        (255.0, 0.64, 0.28, 0.14, 15.0,  0.30, 0.19, mat_leaf_b),
        (330.0, 0.58, 0.29, 0.15, 16.0,  0.31, 0.20, mat_leaf_a),
    ]

    for idx, (az_deg, stem_h, _unused, spread, tilt_deg, leaf_len, leaf_w, leaf_mat) in enumerate(leaf_specs):
        az = math.radians(az_deg)
        tilt = math.radians(tilt_deg)
        dx = math.cos(az) * spread
        dy = math.sin(az) * spread
        top_z = 0.41 + stem_h * math.cos(tilt)

        common.add_cylinder(
            f"Stem_{idx}",
            0.008,
            0.006,
            stem_h,
            8,
            (dx * 0.45, dy * 0.45, 0.41 + stem_h * 0.46),
            mat_stem,
            rotation=(math.sin(az) * tilt, -math.cos(az) * tilt, 0.0),
        )
        add_stylized_leaf(
            f"Leaf_{idx}",
            leaf_len,
            leaf_w,
            (dx * 0.85, dy * 0.85, top_z),
            (math.radians(-26.0), 0.0, az - math.pi * 0.5),
            leaf_mat,
        )

    return common.save_and_export("plant", "indoor_plant")


if __name__ == "__main__":
    build()
