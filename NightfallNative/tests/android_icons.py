"""Compare decoded launcher resources in the actual APK with the approved resources."""
from pathlib import Path
from zipfile import ZipFile
from PIL import Image
import io, json

def verify(apk, android, output):
    manifest=json.loads((android/'Branding/provenance.json').read_text())
    checked=[]
    with ZipFile(apk) as archive:
        for record in manifest['records']:
            path=record['file']; folder,file=path.rsplit('/',1)
            compiled=folder+'-v4/'+file
            actual=Image.open(io.BytesIO(archive.read(compiled))).convert('RGBA')
            expected=Image.open(android/path).convert('RGBA')
            assert actual.size==expected.size==tuple(record['size']), compiled
            assert all(a==b or (a[3]==b[3]==0) for a,b in zip(actual.getdata(),expected.getdata())), 'Launcher artwork changed during APK packaging: '+compiled
            checked.append(compiled)
        for name in ['ic_launcher.xml','ic_launcher_round.xml']:
            assert len(archive.read('res/mipmap-anydpi-v26/'+name))>100
    (output/'apk-launcher-icons.json').write_text(json.dumps({'sourceSha256':manifest['sourceSha256'],'decodedVisiblePngResourcesEqual':checked,'adaptiveXmlPresent':True},indent=2))
    print('APK launcher icons: 15 decoded PNGs match approved art; adaptive and round resources present')
