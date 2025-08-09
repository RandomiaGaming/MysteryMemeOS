from PIL import Image
import os

os.chdir(os.path.dirname(__file__))
image = Image.open("./mysteryimage.png")
if image.mode != "RGBA":
    image = image.convert("RGBA")
width, height = image.size
pixelBuffer = image.load()
file = open("./mysteryimage.raw", "wb")
file.write(width.to_bytes(4, byteorder = "little"))
file.write(height.to_bytes(4, byteorder = "little"))
for y in range(height):
    for x in range(width):
        r, g, b, a = pixelBuffer[x, y]
        file.write(r.to_bytes(1))
        file.write(g.to_bytes(1))
        file.write(b.to_bytes(1))
        file.write(a.to_bytes(1))
file.close()