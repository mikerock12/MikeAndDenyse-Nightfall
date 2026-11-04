"""Cut the owner-reviewed PT-BR bank; derive short effects from the Suno palette."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess

# Seconds in the reviewed 209.56s master; word boundaries checked with local ASR.
CUTS=[[(0,6.12),(6.12,10.20),(10.20,13.68)],[(13.68,16.34),(16.34,18.75),(18.75,24.24)],[(24.24,26.68),(26.68,28.96),(28.96,32.36)],[(32.36,36.96),(36.96,39.92),(39.92,43.68)],[(43.68,46.88),(46.88,49.48),(49.48,54.04)],[(54.04,56.44),(56.44,60.16),(60.16,66.16)],[(66.16,68.92),(68.92,71.32),(71.32,76.84)],[(76.84,79.28),(79.28,82),(82,84.90)],[(86.60,91.44),(91.44,95.36),(95.36,100.68)],[(100.68,103.80),(103.80,107.40),(107.40,113.20)],[(113.20,115.665),(115.665,119.04),(119.04,125.44)],[(125.44,129.52),(129.52,132.64),(132.64,137.52)],[(137.52,140.68),(140.68,143.88),(143.88,149.40)],[(149.40,154.80),(154.80,158.60),(158.60,161.76)],[(161.76,165.28),(165.28,168.60),(168.60,174.72)],[(174.72,177.40),(177.40,180.48),(180.48,185.64)],[(185.64,189.76),(189.76,194.60),(194.60,202.76)]]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--downloads',type=Path,required=True);p.add_argument('--sources',type=Path,required=True);p.add_argument('--dialogues',type=Path,required=True);p.add_argument('--masters',type=Path,required=True);p.add_argument('--ffmpeg',type=Path,required=True);args=p.parse_args()
 out=Path(__file__).resolve().parents[1]/'Content/audio';data=json.loads((out/'provenance.json').read_text(encoding='utf-8'));sources=json.loads(args.sources.read_text(encoding='utf-8'));dialogues=json.loads(args.dialogues.read_text(encoding='utf-8'))['dialogues'];assert len(dialogues)==len(CUTS)==17
 voice=next(r for r in sources if r['title']=='A Luz que Você Não Conseguiu');palette=next(r for r in sources if r['title'].startswith('Nightfall Sound-design'));args.masters.mkdir(parents=True,exist_ok=True)
 for record in [voice,palette]:
  source=args.downloads/(record['title']+'.wav');dest=args.masters/source.name
  if dest.exists():assert sha(dest)==sha(source)
  else:shutil.copyfile(source,dest)
 def encode(source,start,end,file,filters):
  target=out/file;target.parent.mkdir(parents=True,exist_ok=True);duration=end-start;assert duration>.15
  subprocess.run([str(args.ffmpeg),'-nostdin','-hide_banner','-loglevel','error','-y','-ss',str(start),'-t',str(duration),'-i',str(source),'-af',filters,'-ar','44100','-ac','1','-c:a','libvorbis','-q:a','4',str(target)],check=True)
  subprocess.run([str(args.ffmpeg),'-nostdin','-v','error','-i',str(target),'-f','null','-'],stdout=subprocess.DEVNULL,check=True)
  return dict(file=file,sha256=sha(target),bytes=target.stat().st_size,sourceSeconds=[start,end])
 data['voices']=[];data['effects']=[];voice_source=args.downloads/(voice['title']+'.wav');fx_source=args.downloads/(palette['title']+'.wav')
 for dialogue,cuts in zip(dialogues,CUTS):
  boss=dialogue[0]
  for index,(speaker,(start,end)) in enumerate(zip(['mike','denyse','boss'],cuts),1):
   key=speaker+'_'+boss;duration=end-start;filters=f'afade=t=in:d=0.008,afade=t=out:st={duration-.025}:d=0.025,loudnorm=I=-17:TP=-2:LRA=7'
   record=encode(voice_source,start,end,'voices/'+key+'.ogg',filters);record.update(id=key,speaker=speaker,boss=boss,requestedScript=dialogue[index],sunoUrl=voice['url'],masterFile=voice_source.name,masterSha256=sha(voice_source),ownerReviewedCandidate=True)
   data['voices'].append(record)
  print('Cut and decoded PT-BR dialogue:',boss,flush=True)
 # These are edited timbral variants of one generated palette, not claims that
 # the source model obeyed the requested ten-event/16-second composition.
 recipes=[('attack',0,.68,'highpass=f=180,lowpass=f=8500'),('magic',.60,1.22,'asetrate=57600,highpass=f=650,lowpass=f=10000'),('stomp',1.18,1.92,'asetrate=36000,lowpass=f=1500'),('hit',1.82,2.48,'highpass=f=90,lowpass=f=5000'),('hurt',2.40,3.20,'asetrate=40800,lowpass=f=3500'),('enemy_beast',2.92,3.70,'asetrate=33600,lowpass=f=2200'),('enemy_bone',3.65,4.22,'asetrate=60000,highpass=f=1100'),('enemy_ghost',4.15,4.72,'areverse,highpass=f=500,lowpass=f=7000')]
 for key,start,end,filters in recipes:
  record=encode(fx_source,start,end,'sfx/'+key+'.ogg',filters+',afade=t=in:d=0.006,afade=t=out:st=0.35:d=0.10,loudnorm=I=-18:TP=-2:LRA=7');record.update(id=key,sunoUrl=palette['url'],masterFile=fx_source.name,masterSha256=sha(fx_source),transform=filters,sourceType='edited variant from generated sound-design palette')
  data['effects'].append(record)
 (out/'provenance.json').write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 (out/'bank.txt').write_text(''.join(kind+' '+r['id']+' '+r['file']+'\n' for kind,records in [('music',data['music']),('voice',data['voices']),('sfx',data['effects'])] for r in records),encoding='utf-8')
 print('Imported51 voice cuts and8 derived effects; procedural UI/pickup fallbacks retained',flush=True)
if __name__=='__main__':main()
