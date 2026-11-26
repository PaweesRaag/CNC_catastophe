#!/usr/bin/env python3
"""Convert simple SVG path geometry to CNC Catastrophe absolute G-code.

Convert shapes/text to paths and flatten transforms in Inkscape first.
Advanced SVG features such as clipping, filters and raster images are unsupported.
"""
from __future__ import annotations

import argparse
import math
import re
from pathlib import Path
from xml.etree import ElementTree as ET

from svgpathtools import svg2paths2

UNIT_TO_MM = {
    "mm": 1.0,
    "cm": 10.0,
    "in": 25.4,
    "pt": 25.4 / 72.0,
    "pc": 25.4 / 6.0,
    "px": 25.4 / 96.0,
}


def length_mm(value: str | None, fallback: float) -> float:
    if not value:
        return fallback
    match = re.fullmatch(r"\s*([0-9]*\.?[0-9]+)\s*([a-zA-Z]*)\s*", value)
    if not match:
        raise ValueError(f"Unsupported SVG dimension: {value!r}")
    number = float(match.group(1))
    unit = match.group(2) or "px"
    if unit not in UNIT_TO_MM:
        raise ValueError(f"Unsupported SVG unit: {unit}")
    return number * UNIT_TO_MM[unit]


def convert(input_svg: Path, output_gcode: Path, workspace_x: float,
            workspace_y: float, margin: float, sample_mm: float,
            feed: float) -> None:
    root = ET.parse(input_svg).getroot()
    viewbox_text = root.attrib.get("viewBox")
    if viewbox_text:
        vb = [float(v) for v in re.split(r"[ ,]+", viewbox_text.strip())]
        if len(vb) != 4 or vb[2] <= 0 or vb[3] <= 0:
            raise ValueError("Invalid SVG viewBox")
        vx, vy, vw, vh = vb
    else:
        vw = length_mm(root.attrib.get("width"), 100.0)
        vh = length_mm(root.attrib.get("height"), 100.0)
        vx = vy = 0.0

    physical_w = length_mm(root.attrib.get("width"), vw * 25.4 / 96.0)
    physical_h = length_mm(root.attrib.get("height"), vh * 25.4 / 96.0)
    if physical_w <= 0 or physical_h <= 0:
        raise ValueError("SVG physical dimensions must be positive")
    if workspace_x <= 2 * margin or workspace_y <= 2 * margin:
        raise ValueError("Margin leaves no usable workspace")
    if sample_mm <= 0:
        raise ValueError("Sample distance must be positive")
    if feed < 10 or feed > 300:
        raise ValueError("Feed must be between 10 and 300 mm/min to match firmware limits")

    paths, _, _ = svg2paths2(str(input_svg))
    if not paths:
        raise ValueError("No SVG paths found. Convert shapes/text to paths first.")

    fit_scale = min(
        1.0,
        (workspace_x - 2 * margin) / physical_w,
        (workspace_y - 2 * margin) / physical_h,
    )
    scale_x = physical_w / vw * fit_scale
    scale_y = physical_h / vh * fit_scale
    drawn_w = physical_w * fit_scale
    drawn_h = physical_h * fit_scale
    offset_x = (workspace_x - drawn_w) / 2.0
    offset_y = (workspace_y - drawn_h) / 2.0

    def transform(point: complex) -> tuple[float, float]:
        x = offset_x + (point.real - vx) * scale_x
        y = offset_y + (vh - (point.imag - vy)) * scale_y
        return x, y

    lines = [
        "; CNC Catastrophe G-code",
        "G21",
        "G90",
        "M5",
    ]
    segment_total = 0

    for path in paths:
        length = path.length()
        if not math.isfinite(length) or length <= 0:
            continue
        physical_length = length * (scale_x + scale_y) / 2.0
        segments = max(1, min(5000, math.ceil(physical_length / sample_mm)))
        points = []
        for index in range(segments + 1):
            arc_length = length * index / segments
            parameter = path.ilength(arc_length)
            points.append(transform(path.point(parameter)))

        start_x, start_y = points[0]
        lines.extend(["M5", f"G0 X{start_x:.3f} Y{start_y:.3f}", "M3"])
        for x, y in points:
            if not (0 <= x <= workspace_x and 0 <= y <= workspace_y):
                raise ValueError("Transformed path exceeds configured workspace")
            lines.append(f"G1 X{x:.3f} Y{y:.3f} F{feed:.1f}")
            segment_total += 1
        lines.append("M5")

    if segment_total == 0:
        raise ValueError("SVG contains no drawable, non-zero-length path geometry.")

    lines.append("; End of job")
    output_gcode.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {output_gcode}: {segment_total} line segments")
    print(f"SVG physical size: {physical_w:.2f} x {physical_h:.2f} mm")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("-o", "--output", type=Path, default=Path("drawing.gcode"))
    parser.add_argument("--workspace-x", type=float, default=150.0)
    parser.add_argument("--workspace-y", type=float, default=150.0)
    parser.add_argument("--margin", type=float, default=5.0)
    parser.add_argument("--sample-mm", type=float, default=1.0)
    parser.add_argument("--feed", type=float, default=60.0)
    args = parser.parse_args()
    if min(args.workspace_x, args.workspace_y, args.sample_mm, args.feed) <= 0 or args.margin < 0:
        parser.error("Workspace, sample distance and feed must be positive; margin cannot be negative")
    convert(args.input, args.output, args.workspace_x, args.workspace_y,
            args.margin, args.sample_mm, args.feed)


if __name__ == "__main__":
    main()
