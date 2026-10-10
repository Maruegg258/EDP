#!/usr/bin/env python3
"""Generate/check the project-owned native 32x32 1-bit weather icon assets.

Edit the ASCII bitmap source, not the generated C++ header:
  firmware/CrowPanelDashboard/weather_icons_src/WeatherIcons32.icons

Usage:
  python3 tools/generate_weather_icons.py --check
  python3 tools/generate_weather_icons.py
"""
from __future__ import annotations

import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASE = ROOT / "firmware" / "CrowPanelDashboard"
SOURCE = BASE / "weather_icons_src" / "WeatherIcons32.icons"
OUTPUT = BASE / "WeatherIconAssets.h"
KINDS = (
    "CLEAR_DAY",
    "CLEAR_NIGHT",
    "PARTLY_DAY",
    "PARTLY_NIGHT",
    "OVERCAST",
    "FOG",
    "DRIZZLE",
    "RAIN",
    "SNOW",
    "THUNDER",
    "UNKNOWN",
)


def parse_source() -> dict[str, list[str]]:
    rows = [
        line.strip()
        for line in SOURCE.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.lstrip().startswith("# ")
    ]
    parsed: dict[str, list[str]] = {}
    pos = 0
    while pos < len(rows):
        label = rows[pos]
        pos += 1
        if not label.startswith("ICON "):
            raise ValueError(f"Expected ICON name, got: {label!r}")
        name = label[5:]
        if name not in KINDS or name in parsed:
            raise ValueError(f"Unknown or duplicate icon: {name!r}")
        pixels = rows[pos:pos + 32]
        pos += 32
        if len(pixels) != 32 or any(
            len(row) != 32 or set(row) - {".", "#"}
            for row in pixels
        ):
            raise ValueError(f"{name}: expected 32 rows of 32 '.' / '#' pixels")
        if pos >= len(rows) or rows[pos] != "END":
            raise ValueError(f"{name}: missing END")
        pos += 1
        if not any("#" in row for row in pixels):
            raise ValueError(f"{name}: blank icon")
        parsed[name] = pixels

    if tuple(parsed) != KINDS:
        raise ValueError(f"Wrong icon order/set: {tuple(parsed)!r}")
    return parsed


def render(icons: dict[str, list[str]]) -> str:
    lines = [
        "#pragma once",
        '#include "Bitmap1bpp.h"',
        "",
        "// GENERATED 1-bit 32x32 assets. Do not edit this header directly.",
        "// Edit weather_icons_src/WeatherIcons32.icons then run tools/generate_weather_icons.py.",
        "namespace WeatherIconAssets {",
    ]

    for name, pixels in icons.items():
        values = []
        for row in pixels:
            for start in range(0, 32, 8):
                value = 0
                for bit, char in enumerate(row[start:start + 8]):
                    if char == "#":
                        value |= 0x80 >> bit
                values.append(value)
        if len(values) != 128:
            raise ValueError(f"{name}: wrong byte count")

        lines.append(f"static constexpr uint8_t {name}_DATA[] = {{")
        for start in range(0, 128, 12):
            text = ", ".join(f"0x{value:02X}" for value in values[start:start + 12])
            lines.append("  " + text + ("," if start + 12 < 128 else ""))
        lines += [
            "};",
            f"static constexpr Bitmap1bpp {name} = {{32, 32, {name}_DATA}};",
            "",
        ]

    lines += ["}  // namespace WeatherIconAssets", ""]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true",
                        help="Compare generated header to checked-in asset")
    args = parser.parse_args()

    expected = render(parse_source())
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != expected:
            print("FAIL: WeatherIconAssets.h differs from the editable .icons source")
            return 1
        print("PASS: 11 native 32x32 1-bit weather icons match their source")
    else:
        OUTPUT.write_text(expected, encoding="utf-8", newline="\n")
        print("WROTE WeatherIconAssets.h (11 x 32x32 icons)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
