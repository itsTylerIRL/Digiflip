#!/usr/bin/env python3
"""Pack character sprites into per-Digimon files for the SD card.

Usage: tools/build_sprites.py [--preview sheet.png]

Sources (18x18 text grids under [pose] headers: '#' ink, '.' paper, ';'
starts a comment; [idle] is required, the other poses are optional):
  assets/poses/<key>.txt            roster Digimon      -> sp_<key>.dfs
  assets/poses/colosseum/<key>.txt  Colosseum-only boss -> cs_<key>.dfs

Output: assets_sd/sprites/<sp|cs>_<key>.dfs, shipped to the SD card via
fap_file_assets and loaded by the app only while that Digimon is on screen.

File format (little endian):
  "DFSP"  magic
  u8      version (1)
  u8      pose mask: bit n set when pose n is present (bit 0, idle, always)
  then one 54-byte frame per present pose, in pose order: 18 rows of
  3 bytes, each row LSB-first (XBM order, like Flipper icons).
Missing poses fall back to idle in the app.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

from PIL import Image, ImageDraw  # only for --preview

# Order must match DigiflipPose in digiflip_sprites.h.
POSES = ["idle", "walk", "happy", "eat", "refuse", "sleep", "attack", "hurt"]
SIZE = 18
ROOT = Path(__file__).resolve().parent.parent


def parse(path: Path) -> dict[str, list[str]]:
    frames: dict[str, list[str]] = {}
    current: str | None = None
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.split(";", 1)[0].strip()
        if not line:
            continue
        header = re.fullmatch(r"\[(\w+)\]", line)
        if header:
            current = header.group(1)
            if current not in POSES:
                raise SystemExit(f"{path}:{number}: unknown pose {current!r}")
            if current in frames:
                raise SystemExit(f"{path}:{number}: duplicate pose {current!r}")
            frames[current] = []
            continue
        if current is None:
            raise SystemExit(f"{path}:{number}: pixels before any [pose] header")
        if len(line) != SIZE or set(line) - {"#", "."}:
            raise SystemExit(f"{path}:{number}: rows must be {SIZE} of '#'/'.'")
        frames[current].append(line)
    for pose, rows in frames.items():
        if len(rows) != SIZE:
            raise SystemExit(f"{path}: [{pose}] has {len(rows)} rows, expected {SIZE}")
    if "idle" not in frames:
        raise SystemExit(f"{path}: missing the required [idle] pose")
    return frames


def pack_frame(rows: list[str]) -> bytes:
    out = bytearray()
    for row in rows:
        for byte in range(3):
            value = 0
            for bit in range(8):
                x = byte * 8 + bit
                if x < SIZE and row[x] == "#":
                    value |= 1 << bit
            out.append(value)
    return bytes(out)


def render(rows: list[str], scale: int) -> Image.Image:
    image = Image.new("L", (SIZE, SIZE), 255)
    for y, row in enumerate(rows):
        for x, cell in enumerate(row):
            if cell == "#":
                image.putpixel((x, y), 0)
    return image.resize((SIZE * scale, SIZE * scale), Image.NEAREST)


def preview(sets: list[tuple[str, dict[str, list[str]]]], out: Path) -> None:
    """Contact sheet of every Digimon with drawn poses, 8x, labelled."""
    cell = SIZE * 8
    label = 14
    sheet = Image.new("L", (len(POSES) * (cell + 8) + 8, len(sets) * (cell + label + 8) + 8), 255)
    draw = ImageDraw.Draw(sheet)
    for row, (key, frames) in enumerate(sets):
        y = 8 + row * (cell + label + 8)
        for column, pose in enumerate(POSES):
            x = 8 + column * (cell + 8)
            draw.text((x, y), f"{key} {pose}", fill=0)
            box = (x, y + label, x + cell - 1, y + label + cell - 1)
            if pose in frames:
                sheet.paste(render(frames[pose], 8), (x, y + label))
                draw.rectangle(box, outline=180)
            else:
                draw.rectangle(box, outline=220)
    sheet.save(out)


def main() -> None:
    roster = (ROOT / "src" / "core" / "digiflip_roster_data.inc").read_text(encoding="utf-8")
    order = re.findall(r'^\s*\{"([^"]+)", "[^"]+", DigiflipStage', roster, flags=re.M)
    poses_dir = ROOT / "assets" / "poses"
    out_dir = ROOT / "assets_sd" / "sprites"
    out_dir.mkdir(parents=True, exist_ok=True)
    for stale in out_dir.glob("*.dfs"):
        stale.unlink()

    for pose_file in poses_dir.glob("*.txt"):
        if pose_file.stem not in order:
            raise SystemExit(f"{pose_file}: {pose_file.stem!r} is not a roster species")

    drawn: list[tuple[str, dict[str, list[str]]]] = []
    total_poses = 0
    sources = [("sp", f) for f in sorted(poses_dir.glob("*.txt"))]
    sources += [("cs", f) for f in sorted((poses_dir / "colosseum").glob("*.txt"))]
    for prefix, pose_file in sources:
        key = pose_file.stem
        frames = parse(pose_file)
        total_poses += len(frames) - 1
        mask = 0
        payload = bytearray()
        for bit, pose in enumerate(POSES):
            if pose in frames:
                mask |= 1 << bit
                payload += pack_frame(frames[pose])
        (out_dir / f"{prefix}_{key}.dfs").write_bytes(b"DFSP" + bytes([1, mask]) + payload)
        if len(frames) > 1:
            drawn.append((key, frames))

    drawn.sort(key=lambda item: order.index(item[0]))
    if "--preview" in sys.argv[1:]:
        out = Path(sys.argv[sys.argv.index("--preview") + 1])
        preview(drawn, out)
        print(f"preview written to {out}")
    print(f"packed {len(sources)} sprites ({total_poses} drawn poses for {len(drawn)} species)")


if __name__ == "__main__":
    main()
