#!/usr/bin/env python3
"""Generate project-owned native 1-bit Dashboard fonts from editable ASCII matrices.

Each .glyphs source stores an explicit bitmap at its FINAL display size.
No resizing or font library is used. Requires only Python 3 stdlib.

Examples:
  python3 tools/generate_dashboard_fonts.py
  python3 tools/generate_dashboard_fonts.py --check
  python3 tools/generate_dashboard_fonts.py --font 17
"""
from __future__ import annotations

import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FIRMWARE = ROOT / "firmware" / "CrowPanelDashboard"
SOURCES = FIRMWARE / "fonts_src"

REQUIRED = {
    "34": set("0123456789.:-+ "),
    "17": set("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -/.:+%"),
    "13": set("0123456789.-/%+C "),
}
SIZES = {"34": (22, 34), "17": (12, 17), "13": (8, 13)}


def parse_source(path: Path, size: str) -> tuple[str, int, int, dict[str, list[str]]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    noncomments = [line.strip() for line in lines if line.strip() and not line.lstrip().startswith("# ")]
    if not noncomments:
        raise ValueError(f"{path}: empty source")
    header = noncomments.pop(0).split()
    if len(header) != 4 or header[0] != "FONT":
        raise ValueError(f"{path}: expected FONT Name Width Height")
    _, name, wtext, htext = header
    width, height = int(wtext), int(htext)
    if (width, height) != SIZES[size] or name != f"DashboardFont{size}":
        raise ValueError(f"{path}: wrong font name or size")
    glyphs: dict[str, list[str]] = {}
    pos = 0
    while pos < len(noncomments):
        head = noncomments[pos]
        if not head.startswith("GLYPH U+"):
            raise ValueError(f"{path}: expected GLYPH U+XXXX, got {head!r}")
        char_code = head.partition("U+")[2]
        character = chr(int(char_code, 16))
        if character in glyphs:
            raise ValueError(f"{path}: duplicate {char_code}")
        rows = noncomments[pos + 1: pos + 1 + height]
        if len(rows) != height or any(len(row) != width for row in rows):
            raise ValueError(f"{path}: {char_code} must be {width}x{height}")
        if any(set(row) - {"#", "."} for row in rows):
            raise ValueError(f"{path}: {char_code} contains invalid pixels")
        if pos + height + 1 >= len(noncomments) or noncomments[pos + height + 1] != "END":
            raise ValueError(f"{path}: {char_code} missing END")
        if character != " " and not any("#" in row for row in rows):
            raise ValueError(f"{path}: blank non-space glyph U+{ord(character):04X}")
        if character == " " and any("#" in row for row in rows):
            raise ValueError(f"{path}: space must not draw pixels")
        glyphs[character] = rows
        pos += height + 2
    if set(glyphs) != REQUIRED[size]:
        missing = sorted(REQUIRED[size] - set(glyphs))
        extra = sorted(set(glyphs) - REQUIRED[size])
        raise ValueError(f"{path}: incorrect glyph set; missing={missing}, extra={extra}")
    return name, width, height, glyphs


def encode(rows: list[str], width: int) -> list[int]:
    packed: list[int] = []
    for row in rows:
        for start in range(0, width, 8):
            byte = 0
            for offset, pixel in enumerate(row[start:start + 8]):
                if pixel == "#":
                    byte |= (0x80 >> offset)
            packed.append(byte)
    assert len(packed) == len(rows) * ((width + 7) // 8)
    return packed


def cpp_character(ch: str) -> str:
    # Supported font glyphs are limited to uppercase, digits and ASCII punctuation.
    return "' ' " if ch == " " else f"'{ch}'"


def render_header(name: str, width: int, height: int, glyphs: dict[str, list[str]]) -> str:
    parts = [
        "#pragma once",
        '#include "BitmapFont.h"',
        "",
        "// GENERATED FILE: do not edit! See tools/generate_dashboard_fonts.py",
        "// Source: fonts_src/" + name + ".glyphs",
        "// Native fixed-cell, 1-bit, MSB-first. No runtime scaling.",
        f"namespace {name} {{",
    ]
    for ch, rows in glyphs.items():
        symbol = f"U{ord(ch):04X}"
        data = encode(rows, width)
        parts.append(f"static constexpr uint8_t PIXELS_{symbol}[] = {{")
        for start in range(0, len(data), 12):
            line = ", ".join(f"0x{v:02X}" for v in data[start:start+12])
            parts.append("  " + line + ("," if start + 12 < len(data) else ""))
        parts.append("};")
    parts.append("")
    parts.append("static constexpr BitmapGlyph GLYPHS[] = {")
    for ch in glyphs:
        parts.append(
            f"  {{ {cpp_character(ch)}, {width}, {height}, {width}, PIXELS_U{ord(ch):04X} }},"
        )
    parts += [
        "};",
        "static constexpr BitmapFont FONT = {",
        "  GLYPHS, static_cast<uint16_t>(sizeof(GLYPHS) / sizeof(GLYPHS[0])),",
        f"  {height}",
        "};",
        f"}}  // namespace {name}",
        "",
    ]
    return "\n".join(parts)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Verify checked-in header files match sources")
    parser.add_argument("--font", choices=["all", "34", "17", "13"], default="all")
    args = parser.parse_args()
    fonts = tuple(SIZES) if args.font == "all" else (args.font,)
    errors = 0
    for size in fonts:
        source = SOURCES / f"DashboardFont{size}.glyphs"
        output = FIRMWARE / f"DashboardFont{size}.h"
        name, width, height, glyphs = parse_source(source, size)
        content = render_header(name, width, height, glyphs)
        if args.check:
            if not output.exists() or output.read_text(encoding="utf-8") != content:
                print(f"FAIL {size}px: source/header mismatch at {output}")
                errors += 1
            else:
                print(f"PASS {size}px: {len(glyphs)} verified fixed-cell glyphs")
        else:
            output.write_text(content, encoding="utf-8", newline="\n")
            print(f"WROTE {output} ({len(glyphs)} glyphs)")
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
