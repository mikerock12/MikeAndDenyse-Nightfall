"""Encode the approved Windows artwork into Android launcher resources (Pillow)."""
from pathlib import Path
import hashlib, json
from PIL import Image, ImageOps

root=Path(__file__).resolve().parents[1]
source=root/'Platform/Windows/Branding/AppIcon.png'
art=Image.open(source).convert('RGBA')
android=root/'Platform/Android'
res=android/'res'
records=[]
for density,scale in [('mdpi',1),('hdpi',1.5),('xhdpi',2),('xxhdpi',3),('xxxhdpi',4)]:
    directory=res/('mipmap-'+density);directory.mkdir(parents=True,exist_ok=True)
    legacy=ImageOps.contain(art,(round(48*scale),round(48*scale)),Image.Resampling.LANCZOS)
    for name in ['ic_launcher.png','ic_launcher_round.png']:legacy.save(directory/name)
    # Adaptive layer: keep both complete characters/weapons inside the 72dp mask.
    # 60dp content retains transparent safe padding under circle and squircle masks.
    foreground=Image.new('RGBA',(round(108*scale),round(108*scale)))
    contained=ImageOps.contain(art,(round(60*scale),round(60*scale)),Image.Resampling.LANCZOS)
    foreground.alpha_composite(contained,((foreground.width-contained.width)//2,(foreground.height-contained.height)//2))
    foreground.save(directory/'ic_launcher_foreground.png')
    for p in sorted(directory.glob('ic_launcher*.png')):records.append({'file':p.relative_to(android).as_posix(),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'size':list(Image.open(p).size)})
adaptive=res/'mipmap-anydpi-v26';adaptive.mkdir(parents=True,exist_ok=True)
xml='<?xml version="1.0" encoding="utf-8"?>\n<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n    <background android:drawable="@color/app_icon_background" />\n    <foreground android:drawable="@mipmap/ic_launcher_foreground" />\n</adaptive-icon>\n'
for name in ['ic_launcher.xml','ic_launcher_round.xml']:(adaptive/name).write_text(xml,encoding='utf-8')
background='#050711' if 'RuaDeAco' in str(root) else '#100918'
(res/'values').mkdir(parents=True,exist_ok=True)
(res/'values/icon_colors.xml').write_text('<?xml version="1.0" encoding="utf-8"?>\n<resources><color name="app_icon_background">'+background+'</color></resources>\n',encoding='utf-8')
metadata=android/'Branding';metadata.mkdir(exist_ok=True)
(metadata/'provenance.json').write_text(json.dumps({'source':'Platform/Windows/Branding/AppIcon.png','sourceSha256':hashlib.sha256(source.read_bytes()).hexdigest(),'method':'Technical resize/container encoding from approved Windows artwork; no creative changes. Adaptive foreground 108dp with 60dp content; density PNGs and v26 XML.','records':records},indent=2)+'\n',encoding='utf-8')
print('Android launcher icons encoded:',root.name,len(records),'PNG resources')
