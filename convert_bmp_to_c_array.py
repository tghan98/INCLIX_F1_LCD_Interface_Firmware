#!/usr/bin/env python3
"""
BMP 이미지를 4bpp grayscale C 배열로 변환
"""
import sys
from PIL import Image
import numpy as np

def bmp_to_4bpp_grayscale(image_path, output_path=None):
    try:
        # 이미지 열기
        img = Image.open(image_path)
        
        # Grayscale로 변환
        img_gray = img.convert('L')
        
        width, height = img_gray.size
        print(f"Image size: {width}x{height}")
        
        # NumPy 배열로 변환
        pixels = np.array(img_gray)
        
        # 0-255 범위를 0-15 범위로 변환 (4bpp)
        pixels_4bpp = (pixels / 16).astype(np.uint8)
        
        # C 배열 생성
        c_array = "/* Cassette icon bitmap - {}x{} */\n".format(width, height)
        c_array += "static const uint8_t s_icon_bitmap[{}][{}] = {{\n".format(height, width)
        
        for y in range(height):
            c_array += "  {"
            for x in range(width):
                c_array += "{:2d}".format(pixels_4bpp[y, x])
                if x < width - 1:
                    c_array += ","
            c_array += "}"
            if y < height - 1:
                c_array += ","
            c_array += "\n"
        
        c_array += "};\n"
        
        # 출력
        print("\n" + "="*60)
        print("C Array:")
        print("="*60)
        print(c_array)
        
        # 파일로 저장
        if output_path:
            with open(output_path, 'w') as f:
                f.write(c_array)
            print(f"\nSaved to: {output_path}")
        
        # 헤더 정의 출력
        print("\n" + "="*60)
        print("Header definitions to update:")
        print("="*60)
        print(f"#define SLIDING_ICON_WIDTH        ({width})")
        print(f"#define SLIDING_ICON_HEIGHT       ({height})")
        
        return width, height, c_array
        
    except Exception as e:
        print(f"Error: {e}")
        return None, None, None

if __name__ == "__main__":
    image_path = r"C:\Users\hansu\Desktop\INCLIX F-1\GUI\GUI 파편화\카세트.bmp"
    output_path = r"c:\Users\hansu\Documents\STM32_iar_Workspace\INCLIX_F1_LCD_Interface_test_hourglass_spinner_poc\cassette_array.txt"
    
    bmp_to_4bpp_grayscale(image_path, output_path)
