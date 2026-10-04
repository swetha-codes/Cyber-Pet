from pathlib import Path
from PIL import Image

for f in sorted(Path("art").glob("*.png")):
    im = Image.open(f).convert("RGBA")
    px = list(im.getdata())
    clear = sum(1 for p in px if p[3] < 128)
    print(f.name, im.size, "corner pixel:", im.getpixel((0, 0)), f"transparent: {clear}/{len(px)}")