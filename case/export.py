#!/usr/bin/env python3
"""Regenerate the printable meshes and previews using local OpenSCAD."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent
SCAD = "/opt/homebrew/bin/openscad"
OUT = ROOT / "output"
OUT.mkdir(exist_ok=True)

for part in ("base", "lid", "fit_check", "print_layout"):
    subprocess.run([
        SCAD, "--hardwarnings", "--export-format", "binstl",
        "-D", f'part="{part}"', "-o", str(OUT / f"{part}.stl"),
        str(ROOT / "dcf77_case.scad"),
    ], check=True)

for part in ("assembly", "cutaway", "exploded", "print_layout", "fit_check"):
    subprocess.run([
        SCAD, "--hardwarnings", "--render=true", "--autocenter", "--viewall",
        "--imgsize", "1400,1100", "--camera=125,-145,120,34,29,20",
        "-D", f'part="{part}"', "-o", str(OUT / f"{part}.png"),
        str(ROOT / "dcf77_case.scad"),
    ], check=True)
