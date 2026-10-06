"""Says whether a screenshot shows something: "ok ..." or "blank ...", with its size and statistics.

A frame is blank when nearly all of it is one colour (a cleared or white screen): the luminance
varies little, or one 8-level bucket of it holds almost every pixel.
"""

import sys

from PIL import Image, ImageStat

image = Image.open(sys.argv[1]).convert("L")
stat = ImageStat.Stat(image)
histogram = image.histogram()
buckets = [sum(histogram[i : i + 8]) for i in range(0, 256, 8)]
dominant = max(buckets) / (image.width * image.height)
blank = stat.stddev[0] < 4 or dominant > 0.98
print(
    f"{'blank' if blank else 'ok'} ({image.width}x{image.height}, mean {stat.mean[0]:.0f}, "
    f"stddev {stat.stddev[0]:.1f}, {dominant:.0%} in one band)"
)
