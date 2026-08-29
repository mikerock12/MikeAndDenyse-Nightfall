#!/usr/bin/env python3
"""Punch magenta on character/enemy/boss/item art. Leave world backgrounds intact."""
from pathlib import Path
import shutil
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "game" / "assets"
RES = ROOT / "NightfallUnity" / "Assets" / "Resources" / "Art"
STREAM = ROOT / "NightfallUnity" / "Assets" / "StreamingAssets" / "art"

def key_magenta(im: Image.Image) -> Image.Image:
    arr = np.array(im.convert("RGBA"), dtype=np.float32)
    r, g, b = arr[:, :, 0], arr[:, :, 1], arr[:, :, 2]
    mag = (r - g) + (b - g)
    alpha = np.clip(255.0 - np.maximum(mag - 90.0, 0.0) * 1.7, 0.0, 255.0)
    pure = (r > 180) & (b > 180) & (g < 160)
    alpha[pure] = 0
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

def is_bg(p: Path) -> bool:
    return p.stem.lower().endswith("_bg") or p.parent.name.lower() == "worlds"

def process(src: Path, dest: Path, punch: bool) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    im = Image.open(src)
    if punch:
        im = key_magenta(im)
        im = autocrop(im)
    else:
        im = im.convert("RGBA")
    im.save(dest, "PNG")

def main() -> None:
    RES.mkdir(parents=True, exist_ok=True)
    n = 0
    for p in sorted(SRC.rglob("*")):
        if p.suffix.lower() not in {".png", ".jpg", ".jpeg", ".webp"}:
            continue
        if p.name.startswith("_"):
            continue
        punch = not is_bg(p)
        name = p.stem + ".png"
        process(p, RES / name, punch)
        process(p, STREAM / p.parent.name / name, punch)
        n += 1
        print(("KEY " if punch else "BG  ") + f"{p.parent.name}/{p.name} -> {name} {Image.open(RES / name).size}")
    print(f"done {n}")

if __name__ == "__main__":
    main()
