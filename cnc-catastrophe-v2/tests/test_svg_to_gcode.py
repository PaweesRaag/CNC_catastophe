from pathlib import Path

import pytest

from tools.svg_to_gcode import convert


def write_svg(tmp_path: Path, body: str, width: str = "50mm",
              height: str = "50mm", viewbox: str = "0 0 50 50") -> Path:
    source = tmp_path / "input.svg"
    source.write_text(
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" '
        f'height="{height}" viewBox="{viewbox}">{body}</svg>',
        encoding="utf-8",
    )
    return source


def test_square_generates_absolute_plot_job(tmp_path: Path) -> None:
    source = write_svg(tmp_path, '<path d="M5 5 H45 V45 H5 Z"/>')
    output = tmp_path / "square.gcode"

    convert(source, output, 150, 150, 5, 1, 60)
    content = output.read_text(encoding="utf-8")

    assert content.startswith("; CNC Catastrophe G-code\nG21\nG90\nM5\n")
    assert "G0 X" in content
    assert "M3\n" in content
    assert "G1 X" in content
    assert content.rstrip().endswith("; End of job")
    assert content.count("M3") == 1
    assert content.count("M5") >= 3


def test_oversized_artwork_is_scaled_into_workspace(tmp_path: Path) -> None:
    source = write_svg(
        tmp_path, '<path d="M0 0 H200 V200 H0 Z"/>',
        width="200mm", height="200mm", viewbox="0 0 200 200",
    )
    output = tmp_path / "scaled.gcode"

    convert(source, output, 100, 80, 5, 2, 60)
    content = output.read_text(encoding="utf-8")

    coords = []
    for line in content.splitlines():
        for token in line.split():
            if token.startswith(("X", "Y")):
                coords.append(float(token[1:]))
    assert coords
    assert min(coords) >= 0
    assert max(coords) <= 100


def test_svg_without_paths_is_rejected(tmp_path: Path) -> None:
    source = write_svg(tmp_path, "<text x='2' y='20'>Hello</text>")
    with pytest.raises(ValueError, match="No SVG paths"):
        convert(source, tmp_path / "empty.gcode", 150, 150, 5, 1, 60)


def test_invalid_viewbox_is_rejected(tmp_path: Path) -> None:
    source = write_svg(
        tmp_path, '<path d="M0 0 L10 10"/>', viewbox="0 0 0 50"
    )
    with pytest.raises(ValueError, match="Invalid SVG viewBox"):
        convert(source, tmp_path / "invalid.gcode", 150, 150, 5, 1, 60)


def test_margin_must_leave_usable_workspace(tmp_path: Path) -> None:
    source = write_svg(tmp_path, '<path d="M0 0 L10 10"/>')
    with pytest.raises(ValueError, match="Margin leaves no usable workspace"):
        convert(source, tmp_path / "invalid.gcode", 10, 10, 5, 1, 60)
