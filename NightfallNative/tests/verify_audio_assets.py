"""Check every distributed audio payload against the approved source manifest."""
from pathlib import Path,PurePosixPath
import hashlib,json,sys,zipfile

def verify(apk=None):
 archive=zipfile.ZipFile(apk) if apk else None
 root=Path(__file__).resolve().parents[1]/'Content/audio'
 def read(name):return archive.read('assets/audio/'+name) if archive else (root/name).read_bytes()
 data=json.loads(read('provenance.json'));records=[]
 for group,count in [('music',16),('voices',51),('effects',8)]:
  rows=data[group];assert len(rows)==count,(group,len(rows));assert len({r['id'] for r in rows})==count
  for r in rows:
   file=PurePosixPath(r['file']);assert not file.is_absolute() and '..' not in file.parts and '\\' not in r['file']
   raw=read(r['file']);assert raw.startswith(b'OggS') and len(raw)==r['bytes']
   assert hashlib.sha256(raw).hexdigest()==r['sha256'],r['file']
  records+=rows
 assert len({r['file'] for r in records})==75
 indexed=[line.split() for line in read('bank.txt').decode().splitlines()]
 assert len(indexed)==75 and {x[2] for x in indexed}=={r['file'] for r in records}
 if archive:
  assert not any(n.startswith('assets/audio/') and n.lower().endswith('.wav') for n in archive.namelist()),'Original matrices must not be packaged'
  archive.close()
 print('Audio payloads verified:16 music loops,51 PT-BR lines,8 edited effects;75 SHA-256 matches')
 return {'music':16,'voices':51,'effects':8,'files':75,'sourceHashesVerified':True}
if __name__=='__main__':verify(sys.argv[1] if len(sys.argv)>1 else None)
