# 배열을 90도 시계방향 회전
$original = @(
  @( 0,15,15,15,15,15,15,15,15, 0),
  @(15,15,15,15,15,15,15,15,15,15),
  @(15,15,15,15,15,15,15,15,15,15),
  @(15,15,15,15,15,15,15,15,15,15),
  @(15,15,15,15, 0, 0,15,15,15,15),
  @(15,15,15,15, 0, 0,15,15,15,15),
  @(15,15,15,15, 0, 0,15,15,15,15),
  @(15,15,15,15, 0, 0,15,15,15,15),
  @(15,15,15,15, 0, 0,15,15,15,15),
  @(15,15,15,15, 0, 0,15,15,15,15),
  @(15,15,15,15, 0, 0,15,15,15,15),
  @(15,15,15,15,15,15,15,15,15,15),
  @(15,15,15,15,15,15,15,15,15,15),
  @(15,15,15,15,15,15,15,15,15,15),
  @(15,15,15,15,15,15,15,15,15,15),
  @(15,15,15,15,15,15,15,15,15,15),
  @(15,15,15,15, 0, 0,15,15,15,15),
  @(15,15,15, 0, 0, 0, 0,15,15,15),
  @(15,15,15,15, 0, 0,15,15,15,15),
  @(15,15,15,15,15,15,15,15,15,15),
  @(15,15,15,15,15,15,15,15,15,15),
  @(15,15,15,15,15,15,15,15,15,15),
  @( 0,15,15,15,15,15,15,15,15, 0)
)

$oldHeight = $original.Length
$oldWidth = $original[0].Length

Write-Host "Original: ${oldHeight}x${oldWidth}"
Write-Host "Rotated: ${oldWidth}x${oldHeight}"

# 90도 시계방향 회전: new[col][height-1-row] = old[row][col]
$rotated = @()
for ($newRow = 0; $newRow -lt $oldWidth; $newRow++) {
    $row = @()
    for ($newCol = 0; $newCol -lt $oldHeight; $newCol++) {
        $oldRow = $oldHeight - 1 - $newCol
        $oldCol = $newRow
        $row += $original[$oldRow][$oldCol]
    }
    $rotated += ,@($row)
}

# C 배열 생성
$cArray = "/* Cassette icon bitmap - ${oldWidth}x${oldHeight} (90° rotated) */`n"
$cArray += "static const uint8_t s_icon_bitmap[SLIDING_ICON_HEIGHT][SLIDING_ICON_WIDTH] = {`n"

for ($y = 0; $y -lt $rotated.Length; $y++) {
    $cArray += "  {"
    for ($x = 0; $x -lt $rotated[$y].Length; $x++) {
        $cArray += "{0,2:D}" -f $rotated[$y][$x]
        if ($x -lt ($rotated[$y].Length - 1)) {
            $cArray += ","
        }
    }
    $cArray += "}"
    if ($y -lt ($rotated.Length - 1)) {
        $cArray += ","
    }
    $cArray += "`n"
}

$cArray += "};`n"

# 출력
Write-Host "`n=========================================="
Write-Host "Rotated C Array:"
Write-Host "=========================================="
Write-Host $cArray

# 파일에 저장
$cArray | Out-File -FilePath ".\cassette_rotated_array.txt" -Encoding UTF8
Write-Host "`nSaved to: cassette_rotated_array.txt"
Write-Host "`nHeader update:"
Write-Host "#define SLIDING_ICON_WIDTH        ($oldHeight)"
Write-Host "#define SLIDING_ICON_HEIGHT       ($oldWidth)"
