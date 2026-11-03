"""Drive a disposable emulator through real native menus and gameplay.

Screenshots/logcat are evidence for human visual review, not physical device approval.
"""
from pathlib import Path
import atexit
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
import time
import xml.etree.ElementTree as ET
import zipfile

apk = Path(sys.argv[1]).resolve()
output = Path(sys.argv[2]).resolve()
output.mkdir(parents=True, exist_ok=True)
package = 'com.mikeanddenyse.nightfall.qa'
activity = 'com.mikeanddenyse.nightfall.MainActivity'

# Validate the exact hosted candidate before installing: an APK can install and
# render NativeActivity even when its asset ZIP entries are unusable or absent.
with zipfile.ZipFile(apk) as archive:
    names = archive.namelist()
    assert all('\\' not in entry.orig_filename for entry in archive.infolist()), 'APK has Windows ZIP names'
    index = archive.read('assets/art/index.txt').decode('utf-8').splitlines()
    assert len(index) == 77, f'Expected 77 original images, got {len(index)}'
    for relative in index:
        asset = 'assets/art/' + relative
        assert asset in names, f'Index points to absent ZIP entry {asset!r}'
        assert archive.read(asset)[:8] == b'\x89PNG\r\n\x1a\n', asset
    (output/'apk-assets.json').write_text(json.dumps({'index':index,'entries':names},indent=2),encoding='utf-8')
def adb(*args):
    return subprocess.run(['adb', *args], capture_output=True, check=True).stdout

def save_device_evidence():
    # Keep diagnostic evidence even when a later navigation assertion fails.
    for name, args in [
        ('logcat.txt', ('logcat', '-d')),
        ('activity.txt', ('shell', 'dumpsys', 'activity', 'activities')),
        ('window.txt', ('shell', 'dumpsys', 'window')),
    ]:
        try:
            (output/name).write_bytes(adb(*args))
        except (OSError, subprocess.CalledProcessError):
            pass
atexit.register(save_device_evidence)
adb('install', '-r', str(apk))
adb('shell', 'am', 'force-stop', package)
# This package is isolated from the released game and contains only QA data.
adb('shell', 'pm', 'clear', package)
adb('logcat', '-c')
display_size = os.environ.get('ANDROID_SMOKE_SIZE', '540x960')
assert re.fullmatch(r'\d+x\d+', display_size)
adb('shell', 'wm', 'size', display_size)
adb('shell', 'wm', 'density', '160')
adb('shell', 'settings', 'put', 'secure', 'immersive_mode_confirmations', 'confirmed')
adb('shell', 'settings', 'put', 'system', 'accelerometer_rotation', '0')
adb('shell', 'settings', 'put', 'system', 'user_rotation', '1')
adb('shell', 'am', 'start', '-W', '-n', f'{package}/{activity}')
time.sleep(4)
def dismiss_system_onboarding():
    # A fresh emulator may show Android's first-fullscreen tutorial above the
    # NativeActivity. Dismiss its actual accessibility node; do not send blind
    # back events which would quit the game's title screen.
    try:
        adb('shell','uiautomator','dump','/sdcard/nightfall-window.xml')
        xml=adb('shell','cat','/sdcard/nightfall-window.xml').decode('utf-8',errors='replace')
        (output/'initial-window.xml').write_text(xml,encoding='utf-8')
        for node in ET.fromstring(xml).iter('node'):
            text=(node.get('text','')+' '+node.get('content-desc','')).strip().lower()
            if text in ['got it','ok','entendi']:
                numbers=[int(value) for value in re.findall(r'\d+',node.get('bounds',''))]
                if len(numbers)==4:
                    adb('shell','input','tap',str((numbers[0]+numbers[2])//2),str((numbers[1]+numbers[3])//2))
                    time.sleep(1)
    except (subprocess.CalledProcessError,ET.ParseError):
        pass
dismiss_system_onboarding()
for attempt in range(30):
    startup_log=adb('logcat','-d','-s','raylib').decode('utf-8',errors='replace')
    if 'indexed 77 original sprite assets' in startup_log and 'NIGHTFALL_SCREEN: 0' in startup_log:
        break
    time.sleep(1)
else:
    (output/'logcat.txt').write_bytes(adb('logcat','-d'))
    (output/'startup-blocked.png').write_bytes(adb('exec-out','screencap','-p'))
    raise RuntimeError('Native frontend did not finish initialization with all 77 assets')
initial_pid=adb('shell','pidof',package).strip()
assert initial_pid, 'Native game has no process after startup'
images = []
width, height = 960, 540
def capture(name, expected_screen):
    global width, height
    screen_log=adb('logcat','-d','-s','raylib').decode('utf-8',errors='replace')
    screens=re.findall(r'NIGHTFALL_SCREEN: (\d+)',screen_log)
    if not screens or int(screens[-1]) != expected_screen:
        raise RuntimeError(f'{name}: expected screen {expected_screen}, got {screens[-1:]!r}')
    data = adb('exec-out', 'screencap', '-p')
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise RuntimeError('screencap returned no PNG')
    width, height = struct.unpack('>II', data[16:24])
    if width < height:
        raise RuntimeError(f'Native game must be landscape, got {width}x{height}')
    path = output / f'{name}.png'
    path.write_bytes(data)
    images.append({'file': path.name, 'width': width, 'height': height, 'sha256': hashlib.sha256(data).hexdigest()})
def point(x, y):
    scale = min(width / 960, height / 540)
    return str(round((width - 960*scale)/2 + x*scale)), str(round((height - 540*scale)/2 + y*scale))
def tap(x, y):
    adb('shell', 'input', 'tap', *point(x, y))
    time.sleep(1)
capture('01-title',0)
tap(480,326)
capture('02-hero-select',1)
tap(305,250)
tap(480,481)
capture('03-map',2)
tap(480,501)
time.sleep(3)
capture('04-mike-playing',4)
right = point(204,430)
adb('shell','input','swipe',*right,*right,'1200')
tap(862,422)
tap(714,452)
capture('05-mike-after-input',4)
tap(55,33)
capture('06-pause',4)
tap(480,312)
adb('shell','input','keyevent','4')
time.sleep(1)
capture('07-back-pauses',4)
tap(480,377)
capture('08-return-map',2)
tap(880,33)
tap(645,250)
tap(480,481)
tap(480,501)
time.sleep(3)
capture('09-denyse-playing',4)
tap(714,452)
tap(862,422)
capture('10-denyse-after-input',4)
adb('shell','input','keyevent','3')
time.sleep(1)
adb('shell','am','start','-W','-n',f'{package}/{activity}')
time.sleep(2)
capture('11-resumed',4)
log = adb('logcat','-d').decode('utf-8',errors='replace')
(output/'logcat.txt').write_text(log,encoding='utf-8')
(output/'activity.txt').write_bytes(adb('shell','dumpsys','activity','activities'))
if re.search(r'FATAL EXCEPTION|Fatal signal|UnsatisfiedLinkError|Unable to start activity',log):
    raise RuntimeError('Crash/linker error in emulator logcat')
for evidence in ['indexed 77 original sprite assets','NIGHTFALL_SCREEN: 1','NIGHTFALL_SCREEN: 2','NIGHTFALL_SCREEN: 4','NIGHTFALL_PAUSE: 1']:
    if evidence not in log:
        raise RuntimeError(f'Missing evidence of native frontend state: {evidence}')
if adb('shell','pidof',package).strip() != initial_pid:
    raise RuntimeError('Game process was replaced during input/lifecycle test')
manifest = {'apk_sha256':hashlib.sha256(apk.read_bytes()).hexdigest(),'package':package,'device':'Android API 35 x86_64 emulator, software GPU','display_size_override':display_size,'scope':'native menu navigation with per-screen assertions, both heroes, movement/jump/attack input, pause/back, background/resume with same PID; screenshots require visual review','physical_device':False,'images':images,'passed':True}
(output/'provenance.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps(manifest,indent=2))
