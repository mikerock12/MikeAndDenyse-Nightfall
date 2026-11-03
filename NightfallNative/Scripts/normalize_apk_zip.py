"""Normalize Windows aapt2/JAR ZIP entry names before alignment and signing.

Windows zipfile normalizes ZipInfo.filename on read, hiding backslashes that
Android's ZIP asset manager treats literally. Inspect orig_filename and the
local header as well, then verify the exact indexed artwork bytes.
"""
from pathlib import Path, PurePosixPath
import hashlib
import shutil
import struct
import sys
import zipfile


def normalize(apk: Path):
    temporary = apk.with_suffix('.normalized.apk')
    names = set()
    with zipfile.ZipFile(apk) as source, zipfile.ZipFile(temporary, 'w') as target:
        for entry in source.infolist():
            name = entry.orig_filename.replace('\\', '/')
            if (not name or name.startswith('/') or ':' in name or
                    '..' in PurePosixPath(name).parts or name in names):
                raise ValueError(f'Unsafe or duplicate APK ZIP entry: {name!r}')
            names.add(name)
            info = zipfile.ZipInfo(name, entry.date_time)
            info.compress_type = entry.compress_type
            info.external_attr = entry.external_attr
            # zipalign regenerates padding and signatures are added afterwards.
            with source.open(entry) as reader, target.open(info, 'w') as writer:
                shutil.copyfileobj(reader, writer)
    temporary.replace(apk)
    validate(apk)


def validate(apk: Path):
    with apk.open('rb') as raw, zipfile.ZipFile(apk) as archive:
        for entry in archive.infolist():
            assert '\\' not in entry.orig_filename, entry.orig_filename
            raw.seek(entry.header_offset)
            header = raw.read(30)
            assert header[:4] == b'PK\x03\x04'
            length = struct.unpack_from('<H', header, 26)[0]
            local_name = raw.read(length)
            assert b'\\' not in local_name, local_name
            assert local_name.decode('utf-8') == entry.orig_filename
        index = archive.read('assets/art/index.txt').decode('utf-8').splitlines()
        assert len(index) == 77 and len(set(index)) == 77
        assert all('\\' not in name for name in index), 'Nonportable artwork index'
        for name in index:
            data = archive.read('assets/art/' + name)
            assert data.startswith(b'\x89PNG\r\n\x1a\n'), name
        assert archive.testzip() is None
    with apk.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    print(f'APK ZIP validated: 77 original PNGs, portable local/central names, SHA256 {digest}')


if __name__ == '__main__':
    normalize(Path(sys.argv[1]).resolve())
