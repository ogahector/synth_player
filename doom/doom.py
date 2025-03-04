import numpy as np
import cv2

# Load and process image
img = cv2.imread("doom/doomtext.jpeg", cv2.IMREAD_GRAYSCALE)
img = cv2.resize(img, (128, 32), interpolation=cv2.INTER_LINEAR)
_, img = cv2.threshold(img, 128, 1, cv2.THRESH_BINARY)

# Extract coordinates of 1s
coordinates = [(row, col) for row in range(img.shape[0]) for col in range(img.shape[1]) if img[row, col] == 1]

# Create C-style header file
with open("doom/doom.txt", "w") as f:
    # Define the Pixel struct and array
    f.write("#include <stdint.h>\n\n")
    f.write("typedef struct {\n")
    f.write("    uint8_t row;\n")
    f.write("    uint8_t col;\n")
    f.write("} Pixel;\n\n")
    
    # Write array of coordinates
    f.write(f"const Pixel doomImageOnes[{len(coordinates)}] = {{\n")
    for row, col in coordinates:
        f.write(f"    {{{row}, {col}}},\n")
    f.write("};\n\n")
    
    # Write the number of 1s
    f.write(f"const size_t numOnes = {len(coordinates)};\n")

print(f"Saved {len(coordinates)} coordinates to doom.txt.")
