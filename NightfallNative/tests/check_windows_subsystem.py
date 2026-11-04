"""Reject a console-subsystem executable in the actual Windows candidate."""
from pathlib import Path
import struct, sys
exe=Path(sys.argv[1]);data=exe.read_bytes()
assert data[:2]==b'MZ'
pe=struct.unpack_from('<I',data,0x3c)[0]
assert data[pe:pe+4]==b'PE\0\0'
subsystem=struct.unpack_from('<H',data,pe+24+68)[0]
assert subsystem==2, f'Expected Windows GUI subsystem 2, got {subsystem}'
print(f'{exe.name}: Windows GUI subsystem, no attached console')
