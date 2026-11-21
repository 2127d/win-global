from PIL import Image,ImageDraw,ImageFont
from pathlib import Path
out=Path(__file__).parent
sheet=Image.new('RGB',(1350,600),'#263f3e');draw=ImageDraw.Draw(sheet);font=ImageFont.load_default(size=14)
gif=Image.open('/home/jeffreygustafson/Downloads/95699393844982aa01ce2d7707969c50.gif')
for k,(ref,ours,label) in enumerate([(38,0,'Catch'),(0,5,'Load transfer'),(8,25,'Swing through'),(14,40,'Reach'),(18,50,'Next catch')]):
 gif.seek(ref);sheet.paste(gif.convert('RGB').resize((270,270)),(k*270,30));draw.text((k*270+6,8),'GIF: '+label,font=font,fill='white')
 im=Image.open(out/'frames'/f'frame-{ours:03d}.png');left=73+round(300*ours*.016)
 im=im.crop((left,0,left+540,540));tile=Image.new('RGBA',im.size,'#3d9287');tile.alpha_composite(im)
 sheet.paste(tile.convert('RGB').resize((270,270)),(k*270,330));draw.text((k*270+6,308),'Updated: '+label,font=font,fill='white')
sheet.save(out/'reference-comparison.png')
for p in (out/'frames').glob('frame-*.png'):
 im=Image.open(p);assert im.crop((0,478,1600,480)).getbbox() is None,p
print('Verified complete cycle fits the canopy.')
