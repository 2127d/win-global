from PIL import Image,ImageFilter
import numpy as np,math,json
from pathlib import Path
out=Path(__file__).parent
p=Image.open('/home/jeffreygustafson/Downloads/95699393844982aa01ce2d7707969c50.gif')
frames=[];metrics=[]
def components(mask):
 unseen=set(map(tuple,np.argwhere(mask)));parts=[]
 while unseen:
  start=unseen.pop();stack=[start];points=[]
  while stack:
   y,x=stack.pop();points.append((y,x))
   for q in ((y-1,x),(y+1,x),(y,x-1),(y,x+1)):
    if q in unseen:unseen.remove(q);stack.append(q)
  if len(points)>20:
   q=np.array(points);parts.append((len(points),q[:,1].mean(),q[:,0].mean()))
 return sorted(parts,reverse=True)
for i in range(p.n_frames):
 p.seek(i);im=p.convert('RGB');arr=np.array(im).astype(float);frames.append(arr[::10,::10])
 skin=np.linalg.norm(arr-np.array([211,143,106]),axis=2)<25
 parts=components(skin)
 fur=np.linalg.norm(arr-np.array([168,78,23]),axis=2)<20
 eroded=np.array(Image.fromarray((fur*255).astype('uint8')).filter(ImageFilter.MinFilter(21)))>0
 body=components(eroded)[0];face=parts[0]
 angle=math.degrees(math.atan2(face[2]-body[2],face[1]-body[1]))
 metrics.append(dict(frame=i,time=i*.04,body=[body[1],body[2]],face=[face[1],face[2]],angle=angle,skin=parts))
print('Reference torso/head estimates:')
for m in metrics[::5]:print({k:m[k] for k in ('frame','body','face','angle')})
print('Reference torso y extent:',min(m['body'][1] for m in metrics),max(m['body'][1] for m in metrics))
print('Video -> GIF frame matching:')
for path in sorted((out/'reference').glob('*.png'))[:17]:
 arr=np.array(Image.open(path).convert('RGB').crop((25,24,565,564))).astype(float)[::10,::10]
 errors=[np.mean((arr-ref)**2) for ref in frames]
 print(path.name,int(np.argmin(errors)),round(min(errors),1))
(out/'reference-motion.json').write_text(json.dumps(metrics,indent=2))
