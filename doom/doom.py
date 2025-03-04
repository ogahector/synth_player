import numpy as np
import cv2
import scipy.signal
import matplotlib.pyplot as plt

# Load and process image
img = cv2.imread("doom/doom_start.png", cv2.IMREAD_GRAYSCALE)

# Sobel operator kernels
sobel_x=np.array([[-1, 0, 1],
                  [-2, 0, 2],
                  [-1, 0, 1]])
sobel_y=np.array([[-1,-2,-1],
                  [0, 0, 0],
                  [1, 2, 1]])

# Image filtering
grad_x = scipy.signal.convolve2d(img, sobel_x, mode='same')
grad_y = scipy.signal.convolve2d(img, sobel_y, mode='same')

# Calculate the gradient magnitude
grad_mag = np.sqrt(grad_x**2 + grad_y**2)

# Normalize gradient magnitude to range [0, 255] for display
grad_mag = (grad_mag / grad_mag.max()) * 255
grad_mag = grad_mag.astype(np.uint8)

# Display the magnitude map using OpenCV
cv2.imshow("Sobel Filtered Image", grad_mag)
cv2.waitKey(0)  # Wait for a key press to close the window
cv2.destroyAllWindows()

# Resize and threshold the image
img = cv2.resize(img, (128, 32), interpolation=cv2.INTER_LINEAR)
_, img = cv2.threshold(img, 128, 1, cv2.THRESH_BINARY)

# Apply dilation to thicken the white lines
kernel = np.ones((3, 3), np.uint8)  # Structuring element (3x3 square)
img = cv2.dilate(img, kernel, iterations=1)  # Dilation to make lines thicker

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
