"""Encode the approved source artwork into Windows icon containers (Pillow required)."""
from pathlib import Path
from PIL import Image, ImageOps

root = Path(__file__).resolve().parent
art = ImageOps.contain(Image.open(root / "AppIconSource.png").convert("RGBA"), (256, 256), Image.Resampling.LANCZOS)
icon = Image.new("RGBA", (256, 256))
icon.alpha_composite(art, ((256-art.width)//2, (256-art.height)//2))
icon.save(root / "AppIcon.png")
icon.save(root / "AppIcon.ico", sizes=[(n, n) for n in [16,20,24,32,40,48,64,96,128,256]])
