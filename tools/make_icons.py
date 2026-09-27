#!/usr/bin/env python3
"""Write the app's compiled-in icons from their text grids.

Usage: tools/make_icons.py

Sources: assets/icons/*.txt, one icon per [name] section: rows of '#' (ink)
and '.' (paper), any size, ';' starts a comment. Each icon is written to
images/<name>.png for fap_icon_assets, except [digiflip_icon], which is the
Apps-list icon (application.fam's fap_icon) at the repo root.

The PNGs are build output (git ignores them); `make` and CI run this before
building. Standard library only, so it needs no image packages.
"""

from __future__ import annotations

import re
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOURCES = ROOT / "assets" / "icons"
IMAGES = ROOT / "images"
APP_ICON = "digiflip_icon"


def parse(path: Path) -> dict[str, list[str]]:
    icons: dict[str, list[str]] = {}
    current: str | None = None
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.split(";", 1)[0].strip()
        if not line:
            continue
        header = re.fullmatch(r"\[(\w+)\]", line)
        if header:
            current = header.group(1)
            if current in icons:
                raise SystemExit(f"{path}:{number}: duplicate icon {current!r}")
            icons[current] = []
            continue
        if current is None:
            raise SystemExit(f"{path}:{number}: pixels before any [icon] header")
        if set(line) - {"#", "."}:
            raise SystemExit(f"{path}:{number}: rows may only hold '#' and '.'")
        rows = icons[current]
        if rows and len(line) != len(rows[0]):
            raise SystemExit(f"{path}:{number}: [{current}] rows differ in width")
        rows.append(line)
    for name, rows in icons.items():
        if not rows:
            raise SystemExit(f"{path}: [{name}] is empty")
    return icons


def chunk(kind: bytes, data: bytes) -> bytes:
    body = kind + data
    return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))


def png(rows: list[str]) -> bytes:
    """1-bit greyscale PNG: bit set = white paper, clear = black ink."""
    width, height = len(rows[0]), len(rows)
    raw = bytearray()
    for row in rows:
        raw.append(0)  # filter: none
        bits = [0 if cell == "#" else 1 for cell in row]
        bits += [1] * (-len(bits) % 8)
        for i in range(0, len(bits), 8):
            byte = 0
            for bit in bits[i : i + 8]:
                byte = (byte << 1) | bit
            raw.append(byte)
    header = struct.pack(">IIBBBBB", width, height, 1, 0, 0, 0, 0)
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", header)
        + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + chunk(b"IEND", b"")
    )


def main() -> None:
    IMAGES.mkdir(exist_ok=True)
    written = 0
    for source in sorted(SOURCES.glob("*.txt")):
        for name, rows in parse(source).items():
            out = ROOT / f"{name}.png" if name == APP_ICON else IMAGES / f"{name}.png"
            out.write_bytes(png(rows))
            written += 1
    print(f"wrote {written} icons")


if __name__ == "__main__":
    main()
