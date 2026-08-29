#!/usr/bin/env python3
"""Build launcher icons and copy the web game into the Android assets folder."""
from pathlib import Path
from shutil import copytree, rmtree
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "game"
WWW = ROOT / "android" / "app" / "src" / "main" / "assets" / "www"
RES = ROOT / "android" / "app" / "src" / "main" / "res"
MIKE = GAME / "assets" / "chars" / "mike_idle.png"


def make_icons() -> None:
    src = Image.open(MIKE).convert("RGBA")
    # square pad
    s = max(src.size)
    canvas = Image.new("RGBA", (s, s), (8, 4, 10, 255))
    canvas.paste(src, ((s - src.width) // 2, (s - src.height) // 2), src)
    sizes = {
        "mipmap-mdpi": 48,
        "mipmap-hdpi": 72,
        "mipmap-xhdpi": 96,
        "mipmap-xxhdpi": 144,
        "mipmap-xxxhdpi": 192,
    }
    for folder, px in sizes.items():
        d = RES / folder
        d.mkdir(parents=True, exist_ok=True)
        canvas.resize((px, px), Image.Resampling.LANCZOS).save(d / "ic_launcher.png")
        print("icon", folder, px)


def sync_www() -> None:
    if WWW.exists():
        rmtree(WWW)
    copytree(GAME, WWW)
    print("synced", WWW)


if __name__ == "__main__":
    make_icons()
    sync_www()
