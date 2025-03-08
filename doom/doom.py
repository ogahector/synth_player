import numpy as np
import cv2


# Load image in grayscale
img = cv2.imread("doom/wave_icons.png", cv2.IMREAD_GRAYSCALE)

# Convert the image to grayscale

# Apply a binary threshold to get a black and white image.
# All pixels with intensity greater than 127 become 255 (white),
# and the rest become 0 (black).
_, bw = cv2.threshold(img, 140, 255, cv2.THRESH_BINARY)

# Display the black and white image
cv2.imshow("Black and White Image", bw)
cv2.waitKey(0)
cv2.destroyAllWindows()


# Resize the edge image to the desired dimensions (128x32) using nearest-neighbor
resized = cv2.resize(bw, (128, 32), interpolation=cv2.INTER_NEAREST)
cv2.imshow("Resized", resized)
cv2.waitKey(0)
cv2.destroyAllWindows()
print(resized)

resized=resized/255


# Extract coordinates of 1s (white pixels)
coordinates = [(row, col) for row in range(resized.shape[0])
               for col in range(resized.shape[1]) if resized[row, col] == 0]

# Create C-style header file with the pixel coordinates
with open("doom/doom.txt", "w") as f:
    f.write("#include <stdint.h>\n\n")
    f.write("typedef struct {\n")
    f.write("    uint8_t row;\n")
    f.write("    uint8_t col;\n")
    f.write("} Pixel;\n\n")
    
    f.write(f"const Pixel doomImageOnes[{len(coordinates)}] = {{\n")
    for row, col in coordinates:
        f.write(f"    {{{row}, {col}}},\n")
    f.write("};\n\n")
    
    f.write(f"const size_t numOnes = {len(coordinates)};\n")

print(f"Saved {len(coordinates)} coordinates to doom.txt.")
