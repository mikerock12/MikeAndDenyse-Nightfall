"""Reproducible music import; original WAV masters stay outside public packages."""
from pathlib import Path
import argparse,datetime,hashlib,json,shutil,subprocess,wave

def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def main():
 p=argparse.ArgumentParser();p.add_argument('--downloads',type=Path,required=True);p.add_argument('--sources',type=Path,required=True);p.add_argument('--ffmpeg',type=Path,required=True);p.add_argument('--masters',type=Path,required=True);args=p.parse_args()
 native=Path(__file__).resolve().parents[1];out=native/'Content/audio';(out/'music').mkdir(parents=True,exist_ok=True);args.masters.mkdir(parents=True,exist_ok=True)
 records=json.loads(args.sources.read_text(encoding='utf-8'));ids=['forest','cabin','swamp','desert','grave','castle','ice','volcano','coven','catacombs','woods','peak','cathedral','village','abyss','throne'];music=[]
 for i,key in enumerate(ids,1):
  record=next(r for r in records if r['title'].startswith(f'NF {i:02} - '));source=args.downloads/(record['title']+'.wav');master=args.masters/source.name
  if master.exists():assert digest(master)==digest(source),'Original master mismatch'
  else:shutil.copyfile(source,master)
  with wave.open(str(source)) as wav:seconds=wav.getnframes()/wav.getframerate();rate=wav.getframerate();channels=wav.getnchannels()
  assert 40<=seconds<=240,(key,seconds)
  # The last 1.5 seconds crossfade into the beginning. The resulting end meets
  # the original sample at 1.5 seconds, which is also the start of this loop.
  fade=1.5;end=seconds-fade
  filters=f'[0:a]asplit=3[h][m][t];[h]atrim=0:{fade},asetpts=PTS-STARTPTS[head];[m]atrim=start={fade}:end={end},asetpts=PTS-STARTPTS[mid];[t]atrim=start={end},asetpts=PTS-STARTPTS[tail];[tail][head]acrossfade=d={fade}:c1=tri:c2=tri[seam];[mid][seam]concat=n=2:v=0:a=1,loudnorm=I=-22:TP=-2:LRA=8[a]'
  target=out/'music'/f'{key}.ogg'
  subprocess.run([str(args.ffmpeg),'-nostdin','-hide_banner','-loglevel','error','-y','-i',str(source),'-filter_complex',filters,'-map','[a]','-ar','44100','-ac','2','-c:a','libvorbis','-q:a','4',str(target)],check=True)
  subprocess.run([str(args.ffmpeg),'-nostdin','-v','error','-i',str(target),'-f','null','-'],stdout=subprocess.DEVNULL,check=True)
  music.append(dict(id=key,world=i,title=record['title'],sunoUrl=record['url'],masterFile=source.name,masterSha256=digest(source),masterSeconds=seconds,masterRate=rate,masterChannels=channels,file='music/'+target.name,sha256=digest(target),bytes=target.stat().st_size,loopSeconds=round(seconds-fade,3)))
  print('Imported and decoded:',key,flush=True)
 provenance=dict(schemaVersion=1,createdAtUtc=datetime.datetime.now(datetime.timezone.utc).isoformat(),generator='Suno v6; user account with existing Pro subscription',transform='1.5s loop crossfade, -22 LUFS / -2 dBTP / LRA8, stereo Vorbis q4 at44100Hz',originalWavIncludedInPublicPackage=False,music=music,voices=[],effects=[])
 (out/'provenance.json').write_text(json.dumps(provenance,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 (out/'bank.txt').write_text(''.join('music '+r['id']+' '+r['file']+'\n' for r in music),encoding='utf-8')
 print('Imported all16 music tracks; originals preserved:',args.masters,flush=True)

if __name__=='__main__':main()
