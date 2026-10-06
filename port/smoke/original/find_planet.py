"""Prints the window position of the orange planet (Hephaestus) in a main-menu frame: the middle of
the orange pixels away from the logo and the console."""

import sys

from PIL import Image

image = Image.open(sys.argv[1]).convert("RGB")
xs, ys = [], []
for y in range(220, 640, 2):
    for x in range(0, image.width, 2):
        r, g, b = image.getpixel((x, y))
        if r > 140 and 60 < g < 150 and b < 110 and r - g > 50:
            xs.append(x)
            ys.append(y)
if not xs:
    sys.exit("no orange planet")
xs.sort()
ys.sort()
print(xs[len(xs) // 2], ys[len(ys) // 2])
