import numpy as np
import cv2

img = cv2.imread("doom/doomtext.jpeg", cv2.IMREAD_GRAYSCALE)  # Load image in grayscale
print(img.shape)
img = cv2.resize(img, (128, 32), interpolation=cv2.INTER_LINEAR)  # Compress with bilinear interpolation
_, img = cv2.threshold(img, 128, 1, cv2.THRESH_BINARY)  # Binarize the image
img_list = img.tolist()  # Convert to list of lists

# Create a C-style header file with the image array
with open("doom/doom.txt", "w") as f:
    f.write("const uint8_t doomImage[32][128] = {\n")  # Start array definition
    for row in img_list:
        row_str = ", ".join(map(str, row))  # Convert each row to a comma-separated string
        f.write("    {" + row_str + "},\n")  # Write with curly braces and indentation
    f.write("};\n")  # End array definition
