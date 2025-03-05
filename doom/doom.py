import numpy as np
import cv2
import matplotlib.pyplot as plt

# Load image in grayscale
img = cv2.imread("doom/doom_start.png", cv2.IMREAD_GRAYSCALE)

# Apply Gaussian blur to reduce noise
blurred = cv2.GaussianBlur(img, (5, 5), 0)

# Use Canny edge detector for robust edge detection
edges = cv2.Canny(blurred, 50, 150)


# Display the edge image
cv2.imshow("Canny Edge Image", edges)
cv2.waitKey(0)
cv2.destroyAllWindows()

# Resize the edge image to the desired dimensions (128x32)
resized = cv2.resize(edges, (128, 32), interpolation=cv2.INTER_LINEAR)

# Convert the resized image to a binary image
_, binary = cv2.threshold(resized, 50, 1, cv2.THRESH_BINARY)

# Apply dilation to thicken the edges if needed
kernel = np.ones((3, 3), np.uint8)
binary = cv2.dilate(binary, kernel, iterations=1)

# Extract coordinates of 1s (white pixels)
coordinates = [(row, col) for row in range(binary.shape[0])
               for col in range(binary.shape[1]) if binary[row, col] == 1]

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
