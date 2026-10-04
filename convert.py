from pathlib import Path
from PIL import Image
import re

SCALE = 3  # each drawn pixel becomes a 3x3 block on screen
DELAYS = {"cloudy": 500, "sunny": 300, "rainy": 500, "snowy": 500}  # ms per frame
DEFAULT_DELAY = 500

groups = {}
for f in Path("art").glob("*.png"):
    m = re.fullmatch(r"(.+)_(\d+)\.png", f.name, re.IGNORECASE)
    if not m:
        print(f"Skipping {f.name}")
        continue
    groups.setdefault(m.group(1).lower(), []).append((int(m.group(2)), f))

out = [
    "#pragma once",
    "#include <Arduino.h>",
    "",
    "struct Animation { const char* name; const uint16_t* const* frames; int count; int delayMs; };",
    "",
]
size = None
names = sorted(groups)

for name in names:
    frames = sorted(groups[name])
    var_names = []
    for num, f in frames:
        img = Image.open(f).convert("RGBA")
        img = img.resize((img.width * SCALE, img.height * SCALE), Image.NEAREST)
        if size is None:
            size = img.size
        elif img.size != size:
            raise SystemExit(f"{f.name} is {img.size}, expected {size}. Resize it in Piskel.")
        w, h = img.size
        vals = []
        for y in range(h):
            for x in range(w):
                r, g, b, a = img.getpixel((x, y))
                if a < 128:
                    v = 0
                else:
                    v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                    if v == 0:
                        v = 1  # avoid black pixels being transparent
                vals.append(f"0x{v:04X}")
        var = f"{name}_{num}"
        var_names.append(var)
        out.append(f"const uint16_t {var}[{w*h}] PROGMEM = {{")
        out.append(",".join(vals))
        out.append("};")
    out.append(f"const uint16_t* const {name}_frames[] = {{{','.join(var_names)}}};")
    out.append("")

out.append("enum AnimId { " + ", ".join(f"ANIM_{n.upper()}" for n in names) + " };")
out.append(f"#define ANIM_COUNT {len(names)}")
out.append("const Animation ANIMS[] = {")
for n in names:
    d = DELAYS.get(n, DEFAULT_DELAY)
    out.append(f'  {{"{n}", {n}_frames, {len(groups[n])}, {d}}},')
out.append("};")
out.append(f"#define FRAME_W {size[0]}")
out.append(f"#define FRAME_H {size[1]}")

Path("src/frames.h").write_text("\n".join(out))
print("Converted:", ", ".join(f"{n} ({len(groups[n])})" for n in names), f"at {size[0]}x{size[1]}")