# BMP to 4bpp Grayscale C Array Converter
$bmpPath = ".\cassette.bmp"
$outputPath = ".\cassette_array.txt"

Add-Type -AssemblyName System.Drawing

# 이미지 로드
$img = [System.Drawing.Bitmap]::FromFile($bmpPath)
$width = $img.Width
$height = $img.Height

Write-Host "Image Size: $width x $height"
Write-Host "Converting to 4bpp grayscale..."

# C 배열 생성
$cArray = "/* Cassette icon bitmap - ${width}x${height} */`n"
$cArray += "static const uint8_t s_icon_bitmap[$height][$width] = {`n"

for ($y = 0; $y -lt $height; $y++) {
    $cArray += "  {"
    for ($x = 0; $x -lt $width; $x++) {
        $pixel = $img.GetPixel($x, $y)
        
        # RGB를 Grayscale로 변환 (표준 luminosity method)
        $gray = [int]($pixel.R * 0.299 + $pixel.G * 0.587 + $pixel.B * 0.114)
        
        # 0-255를 0-15로 변환 (4bpp)
        $gray4bpp = [int]($gray / 16)
        
        $cArray += "{0,2:D}" -f $gray4bpp
        if ($x -lt ($width - 1)) {
            $cArray += ","
        }
    }
    $cArray += "}"
    if ($y -lt ($height - 1)) {
        $cArray += ","
    }
    $cArray += "`n"
}

$cArray += "};`n"

# 파일에 저장
$cArray | Out-File -FilePath $outputPath -Encoding UTF8

Write-Host "`n=========================================="
Write-Host "C Array saved to: $outputPath"
Write-Host "=========================================="
Write-Host "`nHeader definitions to update:"
Write-Host "#define SLIDING_ICON_WIDTH        ($width)"
Write-Host "#define SLIDING_ICON_HEIGHT       ($height)"
Write-Host "`nPreview (first few lines):"
Write-Host "=========================================="
$lines = $cArray -split "`n"
for ($i = 0; $i -lt [Math]::Min(10, $lines.Length); $i++) {
    Write-Host $lines[$i]
}
Write-Host "..."

$img.Dispose()
Write-Host "`nDone!"
