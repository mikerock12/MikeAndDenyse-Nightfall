#!/usr/bin/env python3
"""Chroma-key magenta backgrounds, despill edges, auto-crop, write PNGs."""
from __future__ import annotations

import os
from pathlib import Path

from PIL import Image
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "game" / "assets"
SRC_EXTS = {".jpg", ".jpeg", ".png", ".webp"}


def key_magenta(im: Image.Image) -> Image.Image:
    arr = np.array(im.convert("RGBA"), dtype=np.float32)
    r, g, b = arr[:, :, 0], arr[:, :, 1], arr[:, :, 2]
    # Magenta: high R, high B, low G
    mag = (r - g) + (b - g)
    # Strong magenta -> fully transparent
    alpha = np.clip(255.0 - np.maximum(mag - 90.0, 0.0) * 1.7, 0.0, 255.0)
    # Extra kill for near-pure magenta
    pure = (r > 190) & (b > 190) & (g < 150)
    alpha[pure] = 0
    # Despill remaining fringe toward luminance
    fringe = (alpha > 0) & (alpha < 250) & (mag > 40)
    if np.any(fringe):
        lum = 0.3 * r + 0.5 * g + 0.2 * b
        arr[fringe, 0] = np.minimum(arr[fringe, 0], lum[fringe] + 18)
        arr[fringe, 2] = np.minimum(arr[fringe, 2], lum[fringe] + 18)
        arr[fringe, 1] = np.maximum(arr[fringe, 1], (arr[fringe, 0] + arr[fringe, 2]) * 0.35)
    arr[:, :, 3] = alpha
    return Image.fromarray(arr.astype(np.uint8), "RGBA")


def autocrop(im: Image.Image, pad: int = 6) -> Image.Image:
    a = np.array(im.split()[-1])
    ys, xs = np.where(a > 12)
    if len(xs) == 0:
        return im
    x0, x1 = int(xs.min()), int(xs.max())
    y0, y1 = int(ys.min()), int(ys.max())
    x0 = max(0, x0 - pad)
    y0 = max(0, y0 - pad)
    x1 = min(im.width - 1, x1 + pad)
    y1 = min(im.height - 1, y1 + pad)
    return im.crop((x0, y0, x1 + 1, y1 + 1))


def process_file(path: Path, out: Path | None = None, crop: bool = True) -> Path:
    im = Image.open(path)
    keyed = key_magenta(im)
    if crop:
        keyed = autocrop(keyed)
    if out is None:
        out = path.with_suffix(".png")
        if path.suffix.lower() == ".png":
            out = path
    keyed.save(out, "PNG")
    return out


def process_tree(folder: Path, crop: bool = True) -> list[Path]:
    written = []
    for p in sorted(folder.rglob("*")):
        if p.suffix.lower() not in SRC_EXTS:
            continue
        if p.name.startswith("_"):
            continue
        # Skip already-processed companions when a jpg sibling exists
        out = p.with_suffix(".png")
        try:
            process_file(p, out, crop=crop)
            written.append(out)
            print(f"OK  {out.relative_to(ASSETS)}  {Image.open(out).size}")
        except Exception as e:
            print(f"ERR {p}: {e}")
    return written


def main() -> None:
    for sub in ("chars", "enemies", "bosses", "items", "fx", "ui"):
        d = ASSETS / sub
        if d.exists():
            process_tree(d, crop=True)
    worlds = ASSETS / "worlds"
    if worlds.exists():
        for p in sorted(worlds.rglob("*")):
            if p.suffix.lower() not in SRC_EXTS:
                continue
            out = p.with_suffix(".png")
            try:
                if "tile" in p.stem or "ground" in p.stem or "plat" in p.stem:
                    process_file(p, out, crop=True)
                else:
                    Image.open(p).convert("RGB").save(out, "PNG")
                print(f"OK  {out.relative_to(ASSETS)}  {Image.open(out).size}")
            except Exception as e:
                print(f"ERR {p}: {e}")


if __name__ == "__main__":
    main()
