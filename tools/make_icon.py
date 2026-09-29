# Génère res/foot.ico et src/icon_data.h (icône tricolore avec ballon)
from PIL import Image, ImageDraw
import math
def icon(sz):
    im = Image.new('RGBA', (sz, sz), (0,0,0,0)); d = ImageDraw.Draw(im)
    d.rounded_rectangle([0,0,sz-1,sz-1], radius=sz//8, fill=(20,34,64,255))
    x0, x1 = sz//10, sz*9//10; w = (x1-x0)//3
    for k, col in enumerate([(0,85,164,255),(255,255,255,255),(226,0,26,255)]):
        d.rectangle([x0+k*w, sz//10, x0+(k+1)*w if k < 2 else x1, sz*9//10], fill=col)
    c = sz//2; br = sz*3//10
    d.ellipse([c-br, c-br, c+br, c+br], fill=(250,250,250,255), outline=(20,20,20,255), width=max(1, sz//32))
    pr = br*2//5
    pts = [(c + pr*math.cos(math.radians(-90+72*i)), c + pr*math.sin(math.radians(-90+72*i))) for i in range(5)]
    d.polygon(pts, fill=(20,20,20,255))
    for i in range(5):
        a = math.radians(-90+72*i)
        d.line([c + pr*math.cos(a), c + pr*math.sin(a), c + br*0.95*math.cos(a), c + br*0.95*math.sin(a)], fill=(20,20,20,255), width=max(1, sz//40))
    return im
icon(256).save('res/foot.ico', sizes=[(16,16),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256)])
data = icon(32).tobytes()
with open('src/icon_data.h','w') as f:
    f.write('// Icône de la fenêtre (32x32 RGBA), générée par tools/make_icon.py\n#pragma once\nstatic const unsigned char ICON32[32*32*4] = {\n' + ','.join(str(b) for b in data) + '\n};\n')
