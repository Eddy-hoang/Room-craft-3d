"""Common utilities for RoomCraft 3D Blender Python asset generation (Blender 4.x / 5.x)."""

import math
import os
import struct
import zlib
import bpy
import bmesh
from mathutils import Vector, Euler

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
BLEND_SOURCE_DIR = os.path.join(PROJECT_ROOT, "blender", "source")
MODELS_EXPORT_DIR = os.path.join(PROJECT_ROOT, "assets", "models")


def reset_scene():
    """Clear the scene cleanly in background mode without relying on UI contexts."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1.0


def _write_png(filepath, width, height, rgb_bytes):
    """Write an uncompressed/zlib RGB PNG using Python stdlib (no external PIL required)."""
    os.makedirs(os.path.dirname(filepath), exist_ok=True)

    def chunk(tag, data):
        return (
            struct.pack("!I", len(data))
            + tag
            + data
            + struct.pack("!I", zlib.crc32(tag + data) & 0xFFFFFFFF)
        )

    raw = bytearray()
    stride = width * 3
    for y in range(height):
        raw.append(0)  # filter type 0
        raw.extend(rgb_bytes[y * stride : (y + 1) * stride])

    ihdr = struct.pack("!IIBBBBB", width, height, 8, 2, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(bytes(raw), 6)) + chunk(b"IEND", b"")
    with open(filepath, "wb") as f:
        f.write(png)


def ensure_procedural_texture(filepath, kind="fabric", base_rgb=(215, 206, 192), size=128):
    """Generate clean subtle Scandinavian procedural PNG textures directly alongside models."""
    buf = bytearray(size * size * 3)
    br, bg, bb = base_rgb
    for y in range(size):
        for x in range(size):
            r, g, b = br, bg, bb
            if kind == "wood":
                grain = math.sin(y * 0.25 + math.sin(x * 0.08) * 2.4) * 10.0 + math.sin(y * 0.9) * 4.0
                r = int(max(0, min(255, br + grain)))
                g = int(max(0, min(255, bg + grain * 0.85)))
                b = int(max(0, min(255, bb + grain * 0.65)))
            elif kind == "fabric":
                weave = ((x // 2 + y // 2) % 2) * 8 - 4 + ((x % 4 == 0) * 4 - 2)
                r = int(max(0, min(255, br + weave)))
                g = int(max(0, min(255, bg + weave)))
                b = int(max(0, min(255, bb + weave)))
            elif kind == "carpet":
                # Scandinavian subtle bordered geometric weave
                border = (x < 10 or x >= size - 10 or y < 10 or y >= size - 10)
                stripe = ((y // 16) % 2 == 0) and (x % 8 < 4)
                delta = -18 if border else (7 if stripe else -3)
                r = int(max(0, min(255, br + delta)))
                g = int(max(0, min(255, bg + delta)))
                b = int(max(0, min(255, bb + delta)))
            elif kind == "screen":
                # Stylized dark IDE/room-planner UI wallpaper on monitor screen
                if y < 14:
                    r, g, b = 34, 39, 50
                elif x < 30:
                    r, g, b = 42, 48, 62
                elif 44 <= x <= 104 and 32 <= y <= 92:
                    r, g, b = 96, 134, 206
                else:
                    r, g, b = 26, 30, 38
            idx = (y * size + x) * 3
            buf[idx] = r
            buf[idx + 1] = g
            buf[idx + 2] = b

    _write_png(filepath, size, size, buf)
    return filepath


def create_material(name, color_rgba, roughness=0.65, metallic=0.0, texture_path=None):
    """Create a Principled BSDF material with optional diffuse image texture (exports to MTL map_Kd)."""
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
    if bsdf is None:
        bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")

    bsdf.inputs["Base Color"].default_value = color_rgba
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = metallic

    if texture_path and os.path.exists(texture_path):
        tex_node = nt.nodes.new("ShaderNodeTexImage")
        img = bpy.data.images.load(texture_path, check_existing=True)
        tex_node.image = img
        nt.links.new(tex_node.outputs["Color"], bsdf.inputs["Base Color"])
        # Keep diffuse tint white in MTL so texture colors aren't darkened twice
        bsdf.inputs["Base Color"].default_value = (1.0, 1.0, 1.0, 1.0)

    return mat


def apply_box_uv(mesh, uv_scale=1.0):
    """Generate clean box-projected UVs using bmesh (works reliably in headless background mode)."""
    bm = bmesh.new()
    bm.from_mesh(mesh)
    uv_layer = bm.loops.layers.uv.verify()

    for face in bm.faces:
        n = face.normal
        ax, ay, az = abs(n.x), abs(n.y), abs(n.z)
        for loop in face.loops:
            co = loop.vert.co
            if az >= ax and az >= ay:
                u, v = co.x * uv_scale, co.y * uv_scale
            elif ax >= ay and ax >= az:
                u, v = co.y * uv_scale, co.z * uv_scale
            else:
                u, v = co.x * uv_scale, co.z * uv_scale
            loop[uv_layer].uv = (u + 0.5, v + 0.5)

    bm.to_mesh(mesh)
    bm.free()


def apply_planar_top_uv(mesh, width, depth):
    """Map XY bounds linearly to [0, 1] x [0, 1] (ideal for carpets / monitor screens)."""
    bm = bmesh.new()
    bm.from_mesh(mesh)
    uv_layer = bm.loops.layers.uv.verify()
    inv_w = 1.0 / max(width, 1e-4)
    inv_d = 1.0 / max(depth, 1e-4)
    for face in bm.faces:
        for loop in face.loops:
            co = loop.vert.co
            u = (co.x * inv_w) + 0.5
            v = (co.y * inv_d) + 0.5
            loop[uv_layer].uv = (u, v)
    bm.to_mesh(mesh)
    bm.free()


def add_box(name, size, location, material, rotation=(0.0, 0.0, 0.0), bevel_width=0.0, bevel_segments=2, uv_scale=1.0):
    """Add a scaled, optionally beveled box in Blender Z-up space."""
    mesh = bpy.data.meshes.new(name + "_Mesh")
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)

    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=Vector(size), verts=bm.verts)
    if bevel_width > 0.0:
        bmesh.ops.bevel(
            bm,
            geom=list(bm.edges),
            offset=bevel_width,
            segments=bevel_segments,
            affect='EDGES',
            clamp_overlap=True,
        )
    bm.normal_update()
    bm.to_mesh(mesh)
    bm.free()

    apply_box_uv(mesh, uv_scale=uv_scale)
    obj.location = Vector(location)
    obj.rotation_euler = Euler(rotation, 'XYZ')
    if material:
        obj.data.materials.append(material)
    return obj


def add_cylinder(name, radius_bottom, radius_top, depth, segments, location, material, rotation=(0.0, 0.0, 0.0), cap_ends=True):
    """Add a cylinder or tapered frustum in Blender Z-up space."""
    mesh = bpy.data.meshes.new(name + "_Mesh")
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)

    bm = bmesh.new()
    bmesh.ops.create_cone(
        bm,
        cap_ends=cap_ends,
        cap_tris=False,
        segments=segments,
        radius1=radius_bottom,
        radius2=radius_top,
        depth=depth,
    )
    bm.normal_update()
    bm.to_mesh(mesh)
    bm.free()

    apply_box_uv(mesh, uv_scale=1.0)
    obj.location = Vector(location)
    obj.rotation_euler = Euler(rotation, 'XYZ')
    if material:
        obj.data.materials.append(material)
    return obj


def add_uv_sphere(name, radius, segments, ring_count, location, material, scale=(1.0, 1.0, 1.0)):
    """Add a stylized UV sphere in Blender Z-up space."""
    mesh = bpy.data.meshes.new(name + "_Mesh")
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)

    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segments, v_segments=ring_count, radius=radius)
    bmesh.ops.scale(bm, vec=Vector(scale), verts=bm.verts)
    bm.normal_update()
    bm.to_mesh(mesh)
    bm.free()

    apply_box_uv(mesh, uv_scale=1.0)
    obj.location = Vector(location)
    if material:
        obj.data.materials.append(material)
    return obj


def _count_scene_triangles():
    total_tris = 0
    depsgraph = bpy.context.evaluated_depsgraph_get()
    for obj in bpy.context.scene.objects:
        if obj.type != 'MESH':
            continue
        eval_obj = obj.evaluated_get(depsgraph)
        eval_mesh = eval_obj.to_mesh()
        eval_mesh.calc_loop_triangles()
        total_tris += len(eval_mesh.loop_triangles)
        eval_obj.to_mesh_clear()
    return total_tris


def _ensure_relative_mtl_textures(mtl_path):
    """Ensure map_Kd lines in MTL use clean relative filenames (no absolute machine paths)."""
    if not os.path.exists(mtl_path):
        return
    with open(mtl_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    new_lines = []
    for line in lines:
        stripped = line.strip()
        if stripped.startswith("map_Kd "):
            raw_path = stripped[len("map_Kd "):].strip()
            tex_name = os.path.basename(raw_path.replace("\\", "/"))
            new_lines.append(f"map_Kd {tex_name}\n")
        else:
            new_lines.append(line)

    with open(mtl_path, "w", encoding="utf-8") as f:
        f.writelines(new_lines)


def save_and_export(model_folder, asset_filename_stem):
    """Save .blend source and export triangulated Y-up, Forward -Z OBJ + MTL."""
    os.makedirs(BLEND_SOURCE_DIR, exist_ok=True)
    out_dir = os.path.join(MODELS_EXPORT_DIR, model_folder)
    os.makedirs(out_dir, exist_ok=True)

    blend_path = os.path.join(BLEND_SOURCE_DIR, f"{asset_filename_stem}.blend")
    obj_path = os.path.join(out_dir, f"{asset_filename_stem}.obj")
    mtl_path = os.path.join(out_dir, f"{asset_filename_stem}.mtl")

    # Apply transforms cleanly so exported OBJ matches object positions identically
    for obj in bpy.context.scene.objects:
        if obj.type == 'MESH':
            obj.select_set(True)
        else:
            obj.select_set(False)

    tri_count = _count_scene_triangles()
    bpy.ops.wm.save_as_mainfile(filepath=blend_path)

    # Blender 4.x / 5.x native C++ Wavefront OBJ exporter
    bpy.ops.wm.obj_export(
        filepath=obj_path,
        export_selected_objects=False,
        apply_modifiers=True,
        export_uv=True,
        export_normals=True,
        export_materials=True,
        export_triangulated_mesh=True,
        global_scale=1.0,
        forward_axis='NEGATIVE_Z',
        up_axis='Y',
        path_mode='RELATIVE',
    )
    _ensure_relative_mtl_textures(mtl_path)

    print(
        f"[BLENDER-EXPORT] {asset_filename_stem}: "
        f"objects={len([o for o in bpy.context.scene.objects if o.type == 'MESH'])}, "
        f"triangles={tri_count}, obj={obj_path}"
    )
    return tri_count
