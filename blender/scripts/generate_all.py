"""Master batch runner generating all 8 RoomCraft 3D Scandinavian Blender furniture assets."""

import os
import sys
import bpy

sys.path.insert(0, os.path.dirname(__file__))

import create_sofa
import create_bed
import create_gaming_chair
import create_computer
import create_plant
import create_floor_lamp
import create_wardrobe
import create_carpet


def main():
    print(f"=== RoomCraft 3D Blender Asset Pipeline (Blender {bpy.app.version_string}) ===")
    builders = [
        ("01_Modern_Sofa", create_sofa.build),
        ("02_Modern_Bed", create_bed.build),
        ("03_Gaming_Chair", create_gaming_chair.build),
        ("04_Computer_Setup", create_computer.build),
        ("05_Indoor_Plant", create_plant.build),
        ("06_Floor_Lamp", create_floor_lamp.build),
        ("07_Wardrobe", create_wardrobe.build),
        ("08_Minimalist_Carpet", create_carpet.build),
    ]

    summary = []
    for label, fn in builders:
        tris = fn()
        summary.append((label, tris))

    print("=== Blender Generation Summary ===")
    for label, tris in summary:
        print(f"  {label:<24}: {tris} triangles")
    print(f"Total assets generated: {len(summary)}")


if __name__ == "__main__":
    main()
