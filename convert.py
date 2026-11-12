# convert.py
from PIL import Image
import io

# Open the PNG and convert it to a BMP in memory
img = Image.open("/home/zev/Downloads/sorpresa.png").convert("RGB")
buf = io.BytesIO()
img.save(buf, format="BMP")
data = buf.getvalue()

# Write the binary data into a C++ header file
with open("surprise_image.h", "w") as f:
    f.write("#pragma once\n")
    f.write("const unsigned char surprise_bmp[] = {\n")
    for i in range(0, len(data), 12):
        chunk = data[i:i+12]
        hex_str = ", ".join([f"0x{b:02X}" for b in chunk])
        f.write(f"    {hex_str},\n")
    f.write("};\n")
    f.write(f"const unsigned int surprise_bmp_len = {len(data)};\n")

print("Success! Generated surprise_image.h")