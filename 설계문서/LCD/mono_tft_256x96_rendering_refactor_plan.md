# Plan: Mono TFT 256x96 Rendering Refactor

## Status
- in-progress (Phase 0/1/2/3/4/5/6/7/8 완료. BatteryDisplay 화면 시각 검증은 해당 모듈이 임시 테스트용이라 폐기 + ScreenManager로 책임 이관 검토 중이므로 보류)

## Background
- 본 프로젝트의 LCD 모듈은 KJC 2.25 inch mono TFT 패널이며, 물리적 dot 해상도는 가로 256, 세로 96이다.
- 사용 중인 컨트롤러 ST7735S/ST7735SV는 본래 컬러 TFT용으로, 한 RGB565 픽셀이 가로로 인접한 R/G/B 3개의 서브픽셀(=mono dot 3개)에 매핑된다.
- 따라서 컨트롤러 기준 가로 1 픽셀을 켜면 mono 패널에서는 가로 3 dot이 동시에 켜진 것처럼 보이고, 텍스트와 이미지가 가로로 약 3배 늘어진다.
- 현재 드라이버는 RGB565 좌표(컨트롤러 픽셀)를 외부 API에 그대로 노출하고 있어, 상위 App이 mono 패널의 실제 dot 단위로 그림을 그릴 수 없다.

## Problem Definition
- RGB565 좌표계는 컨트롤러 픽셀 단위(가로 128)이지만, 실제 패널은 mono dot 단위(가로 256)다. 두 좌표계의 혼용이 가로 늘어짐의 근본 원인이다.
- 기존 `LOGICAL_WIDTH(85)`, `VIEW_X_MIN/X_MAX` 구조는 컨트롤러 픽셀 영역 잘라내기에는 유용했지만, mono dot 단위 미세 제어를 표현하지 못한다.
- 폰트만 가로로 압축해도 "RGB565 1 픽셀 = mono dot 3 개"라는 물리 매핑은 변하지 않으므로 근본 해결이 되지 않는다.
- 문제는 특정 한 함수가 아니라, **RGB565 컬러 픽셀 단위로 출력하는 모든 경로(픽셀 단위 전송 경로와 cell buffer 일괄 전송 경로 모두)가 공통적으로 가지는 한계**다.

## Goal
- 상위 App에 노출되는 좌표계를 256x96 mono 좌표계로 통일한다.
- 드라이버 내부에서 가로로 인접한 mono dot 3개를 RGB565 1 픽셀의 R/G/B 채널로 packing하여 컨트롤러에 전송한다.
- 텍스트, 아이콘, 배터리 UI가 가로로 3배 늘어져 보이는 현상을 제거한다.
- App 코드 수정 범위는 좌표 상수 재계산 수준으로 최소화한다.

## Non-Goals
- MADCTL 값 변경(0x08 → 0x00 등)은 1차 리팩터 범위에서 제외한다. 현재 0x08을 그대로 유지한다.
- LCD 초기화 시퀀스(FRMCTR/PWCTR/VMCTR/COLMOD 등)의 대규모 변경은 하지 않는다.
- SPI / User_HAL / 핀 매핑 등 하드웨어 추상화 계층의 구조 개편은 하지 않는다.
- ~~기존 4bpp `ST7735S_Drv_WriteFrame` 경로 삭제는 1차 범위에서 제외한다(legacy로만 분류).~~ → **Phase 2.9(2026-04-29)에서 legacy 경로 완전 삭제 완료.**

## Key Design

### 좌표계와 매크로
- `MONO_WIDTH = 256` (패널 사양 기준 가로 dot 수, 고정값)
- `MONO_HEIGHT = 96` (패널 사양 기준 세로 line 수, 고정값)
- `MONO_FRAME_BYTES = 256 * 96 / 8 = 3072`
- `MONO_USABLE_WIDTH` (초기 후보값 256, 캘리브레이션 결과로 조정 가능)
- `MONO_USABLE_HEIGHT` (초기 후보값 96, 캘리브레이션 결과로 조정 가능)
- 외부 API는 `mono_x ∈ [0, MONO_WIDTH-1]`, `mono_y ∈ [0, MONO_HEIGHT-1]` 단위로 좌표를 받는다.
- 드라이버 내부에서 `MONO_USABLE_*` 범위를 벗어나는 좌표는 클리핑(무시)한다.
- `MONO_WIDTH/HEIGHT`(패널 사양)와 `MONO_USABLE_WIDTH/HEIGHT`(실효 가시 영역)를 분리하는 이유: Phase 0 결과에 따라 가시 영역이 사양보다 작아지더라도 좌표계 정의 자체는 흔들리지 않게 하기 위함.

### VIEW_X / VIEW_Y 범위 재검토 (Phase 0 확정)
- `VIEW_X_MIN=23, VIEW_X_MAX=108` 확정 → controller 가로 86 px → mono 환산 `86 × 3 = 258` dot 중 앞 256 dot이 패널에 걸침. mono_x=255까지 정상 표시 가능을 육안으로 확인. `VIEW_X_MAX=109`는 패널 밖이라 기각.
- `VIEW_Y_MIN=1, VIEW_Y_MAX=96` 확정 → 96 line 전체 사용. `VIEW_Y_MIN=0`은 상단 실패, `VIEW_Y_MAX=97`은 하단 실패로 확인되어 주변값으로는 확장 불가.
- `MONO_USABLE_WIDTH = 256`, `MONO_USABLE_HEIGHT = 96`으로 패널 사양 100% 활용 가능.

### 컨트롤러 픽셀 변환 공식 (4단계)

실제 RAM 주소는 mono 좌표에서 아래 4단계로 단계적으로 계산한다. 각 단계의 의미를 분리해 오프셋 중복/누락을 차단한다.

```
relative_controller_x = mono_x / 3              // mono → 동일 윈도우 내 컨트롤러 픽셀
relative_controller_y = mono_y                  // 세로는 1:1 가정
ram_x = ST7735S_RAM_OFFSET_X + ST7735S_VIEW_X_MIN + relative_controller_x
ram_y = ST7735S_RAM_OFFSET_Y + ST7735S_VIEW_Y_MIN + relative_controller_y
subpixel = mono_x % 3                           // 값 0/1/2 → R/G/B 또는 B/G/R 채널
```

- `subpixel` 0/1/2가 R/G/B 순서인지 B/G/R 순서인지는 Phase 0 캘리브레이션으로 확정한다. MADCTL=0x08(BGR) 기준에서는 B/G/R로 추정되지만 코드에서 단정하지 않는다.
- **클램프 규약**: 범위 클램프(`MONO_USABLE_*`)는 mono 좌표가 드라이버로 진입하는 시점에서 **1회만** 수행한다. 이후 변환 단계(`relative_controller_*` → `ram_*`)에서는 추가 클램프를 하지 않는다(이중 적용 방지).
- **오프셋 적용 규약**: `RAM_OFFSET_*`과 `VIEW_*_MIN`은 오직 `ram_*` 산출 단계에서만 더한다. 하위 함수(예: `set_window`)에 전달되는 좌표는 항상 `ram_*` 기준임을 주석으로 명시.
- 코드 구현 시 매크로 이름은 1-step 압축형(`MONO_TO_RAM_X(x)`, `MONO_TO_RAM_Y(y)`)을 사용하되, 매크로 주석에 위 4단계 식을 명시한다.

### Framebuffer 전략
- 1bpp 풀 프레임버퍼를 도입한다. 크기 3072 bytes로 STM32G081 SRAM 36KB 내에 충분하다.
- 같은 컨트롤러 RGB565 픽셀 안에서 인접 mono dot 상태를 보존하기 위해서는 read-modify-write가 필수이며, framebuffer가 그 shadow 역할을 한다.
- 한 줄 갱신 시: framebuffer의 32 byte(=256 bit)를 읽어 86 개 RGB565 픽셀로 펼치고 SPI 한 번에 송신한다(VIEW_X_MAX=108 채택 시 라인 송신 바이트 = 86 × 2 = 172 byte).

### Packing 함수 설계
- `mono3_to_rgb565(uint8_t r_on, uint8_t g_on, uint8_t b_on)` → 16bit 색상 반환.
- `r_on/g_on/b_on`은 1bpp on/off 값. 각각 RGB565의 5/6/5 비트를 전부 1 또는 0으로 채운다.
- subpixel 0/1/2 → R/G/B 또는 B/G/R 매핑은 Phase 0 결과에 따라 결정.

### Draw / Flush 호출 규약
- 픽셀당 window 갱신 방식은 폐기하고 line/rect flush 단위 설계를 기본으로 한다.
- 부분 갱신을 위해 dirty rect 또는 dirty line bitmap을 함께 고려한다(Phase 8).
- **Draw 계열**(`ST7735S_Drv_DrawMonoDot`, `ST7735S_Drv_DrawChar3x5`, `ST7735S_Drv_DrawString3x5`)은 framebuffer만 수정하고 LCD에 즉시 반영하지 않는다.
- **Clear 계열**은 두 함수로 분리한다. 이로써 "Draw는 flush 안 함"이라는 규약의 일관성이 회복된다.
  - `ST7735S_Drv_ClearMonoBuffer(uint8_t on)`: framebuffer만 on/off로 채우고 **flush 안 함**. 일반적인 화면 구성에 사용.
  - `ST7735S_Drv_ClearMono(uint8_t on)`: framebuffer 채움 + **내부에서 즉시 `FlushMono` 호출**. 즉시 전체 화면 지움이 목적인 경우(테스트/디버그/긴급 전환)에만 사용.
- **App 표준 화면 구성 흐름**: `ClearMonoBuffer() → Draw*() → FlushMono()` 한 번으로 완성. ScreenManager/BatteryDisplay는 이 흐름을 기본으로 한다.
  - `ClearMono`(즉시 flush)를 화면 구성 쓰면 "검은 화면 → 텍스트 그리기 → 다시 전체 갱신"의 2단계가 되어 깜빡임이 보일 수 있으므로 권장하지 않는다.
- **Flush 책임은 호출자(App)** 가 진다. ScreenManager/BatteryDisplay 등 상위 App은 한 화면을 구성한 뒤 마지막에 `ST7735S_Drv_FlushMono()` 또는 `ST7735S_Drv_FlushMonoRect()`를 1회 호출한다.
- **드라이버 내부 자동 flush(타이머 주기 등)는 1차 리팩터 범위에서 제외**한다.
- 헤더 주석 규약:
  - Draw 계열 / `ClearMonoBuffer`: `"Does NOT flush. Caller must call ST7735S_Drv_FlushMono*()."` 표준 부착.
  - `ClearMono`: `"Performs immediate FlushMono internally. Prefer ClearMonoBuffer for normal screen composition."` 표준 부착.

## Proposed API

### 신규 mono API (권장 진입점)
- `void ST7735S_Drv_ClearMonoBuffer(uint8_t on);`
  - framebuffer 전체를 on/off로 채우고 **flush하지 않음**. 표준 화면 구성 흐름의 진입점.
- `void ST7735S_Drv_ClearMono(uint8_t on);`
  - framebuffer 전체를 on/off로 채우고 **내부에서 즉시 FlushMono를 호출**한다.
  - 테스트 / 디버그 / 긴급 전체 지움 용도에만 사용. 일반 화면 구성에는 `ClearMonoBuffer` 권장(깜빡임 방지).
- `void ST7735S_Drv_DrawMonoDot(uint16_t mono_x, uint16_t mono_y, uint8_t on);`
  - framebuffer만 갱신. 즉시 flush 없음. 호출자가 별도로 `FlushMono*` 호출 필요.
- `void ST7735S_Drv_FlushMono(void);`
  - 전체 framebuffer를 컨트롤러로 송신.
- `void ST7735S_Drv_FlushMonoRect(uint16_t mono_x0, uint16_t mono_y0, uint16_t mono_x1, uint16_t mono_y1);`
  - 부분 갱신용. 내부에서 controller pixel 경계로 정렬.
- `void ST7735S_Drv_DrawChar3x5(uint16_t mono_x, uint16_t mono_y, char ch, uint8_t on);`
  - **1-step 방식**: 함수명 유지하되 인자 의미를 mono 좌표 + on/off로 전환.
  - 헤더 주석에 "기존 RGB565 색상 인자 기반 의미는 제거되었음. on=1 → dot ON, on=0 → dot OFF." 명시.
  - 즉시 flush 없음. 호출자가 별도로 `FlushMono*` 호출 필요.
- `void ST7735S_Drv_DrawString3x5(uint16_t mono_x, uint16_t mono_y, const char *s, uint8_t on);`
  - DrawChar3x5와 동일한 1-step 전환 원칙. 동일한 헤더 주석 규약 적용.

### Internal / Debug only API
- `mono_fb_get_dot()`, `mono_fb_set_dot()`, `mono_fb_clear()`는 드라이버 내부 `static` 함수로만 존재한다. 헤더 노출하지 않는다.
- `ST7735S_Drv_ReadMonoDot()` 같은 framebuffer 읽기 API는 1차 리팩터 범위에서 **공개하지 않는다**. 외부 App이 framebuffer 내부 상태를 읽기 시작하면 포맷 변경(dirty bitmap 통합 등) 시 외부 호출자까지 영향을 받아 드라이버의 캡슐화가 깨진다.
- 향후 디버그 목적 등 실제 필요가 생기면 `#if ST7735S_DRV_ENABLE_DEBUG_API` 가드 안에 다시 도입하는 방식을 고려한다.

### Legacy API (분리/명시)
- `ST7735S_Drv_WriteFrame(const uint8_t *frame4bpp)` 등 4bpp/RGB565 컬러 경로는 `legacy_` prefix 또는 헤더 주석으로 "사용 금지(deprecated)"임을 명시한다. 시그니처 자체는 1차 리팩터에서 변경하지 않는다.
- 기존에 RGB565 색상을 인자로 받던 함수는 legacy 분류 후, 신규 호출자가 생기지 않도록 한다.

## Implementation Phases

### Phase 0: 서브픽셀 / VIEW 범위 캘리브레이션 테스트
- 상태: **완료** (2026-04-24).
- 수정 파일: `USER/App/ScreenManager_App/ScreenManager_App.c` (BOOT 단계 임시 코드) 또는 별도 임시 테스트 함수.
- 수정 내용:
  1) 컨트롤러 픽셀 1개의 R, G, B를 각각 단독으로 켜는 색상값(0xF800 / 0x07E0 / 0x001F)을 좌측 상단에 한 점씩 그려, 실제 mono dot이 어느 가로 위치에 켜지는지 육안 관찰. MADCTL=0x08 유지.
  2) VIEW_X 확장 검증: `VIEW_X_MAX`를 임시로 108(controller 86 px)로 바꾸어 mono_x=255가 실제 패널 가장자리에 켜지는지 확인. 보이지 않으면 109(controller 87 px) 예비 후보로 시도.
  3) VIEW_Y 확장 검증: `VIEW_Y_MIN`/`VIEW_Y_MAX`를 조절하며 mono_y=0, mono_y=95가 패널 상/하단에서 켜지는지 육안 확인. 96 line 전체 사용 가능 여부를 보수적으로 결정.
- 리스크: 임시 코드가 본 코드에 남는 것. 작업 후 반드시 제거 또는 `#if 0` 처리.
- 검증:
  - subpixel 0/1/2 → R/G/B 또는 B/G/R 매핑 표 확정.
  - VIEW_X_MAX 최종값(108 또는 109) 확정.
  - VIEW_Y_MIN/MAX 최종값 확정 및 `MONO_USABLE_HEIGHT` 값 확정.

#### Phase 0 결과 (확정)

**Subpixel 매핑 (MADCTL=0x08 BGR 가정 일치)**

| `subpx = mono_x % 3` | RGB565 채널 |
|---|---|
| 0 | B (하위 5bit) |
| 1 | G (중간 6bit) |
| 2 | R (상위 5bit) |

관찰 근거: `(0,0)=0xF800`, `(1,0)=0x07E0`, `(2,0)=0x001F` 출력 시 좌측 상단에서 mono dot 패턴이 `OFF-ON-OFF-ON-OFF-ON`으로 보임. 켜진 위치(mono_x=2 → R, mono_x=4 → G, mono_x=6 → B)가 위 매핑과 정확히 일치.

**VIEW_X 결과**

| `VIEW_X_MAX` | 우측 상단 점 관측 | 판정 |
|---|---|---|
| 107 | 가로 3 dot | mono_x=252~254까지만, mono_x=255 미도달 |
| **108** | **가로 1 dot** | **채택**. mono_x=255 정확히 도달 (패널 가장자리) |
| 109 | 안 보임 | 패널 밖. 기각 |

채택값: `ST7735S_VIEW_X_MIN=23`, `ST7735S_VIEW_X_MAX=108` → 컨트롤러 픽셀 86개. 마지막 컨트롤러 픽셀의 G/R 서브픽셀(mono_x=256, 257)은 패널 밖이지만 무해.

**VIEW_Y 결과**

| `VIEW_Y_MIN` | `VIEW_Y_MAX` | 관측 | 판정 |
|---|---|---|---|
| 2 | 95 | (기준) | — |
| 1 | 95 | 변화 없음 | 위쪽 1줄 확장 가능 |
| 0 | 95 | 모두 사라짐 | 위쪽 한계 초과. 기각 |
| 1 | 96 | 변화 없음 (95와 동일하게 보임) | 한 줄 차이라 육안 미식별 |
| 1 | 97 | 하단 두 점 사라짐 | 하단 한계 초과 → 96은 패널 안 |

채택값: `ST7735S_VIEW_Y_MIN=1`, `ST7735S_VIEW_Y_MAX=96` → 컨트롤러 픽셀 96 line. 패널 사양 96 line 100% 활용.

**최종 채택 매크로 값**

| 항목 | 값 |
|---|---|
| `ST7735S_VIEW_X_MIN` | 23U |
| `ST7735S_VIEW_X_MAX` | 108U |
| `ST7735S_VIEW_Y_MIN` | 1U |
| `ST7735S_VIEW_Y_MAX` | 96U |
| `ST7735S_LOGICAL_WIDTH` (컨트롤러 픽셀) | 86 |
| `ST7735S_LOGICAL_HEIGHT` (컨트롤러 픽셀) | 96 |
| `ST7735S_MONO_WIDTH` | 256U |
| `ST7735S_MONO_HEIGHT` | 96U |
| `ST7735S_MONO_USABLE_WIDTH` | **256U** (패널 100% 활용) |
| `ST7735S_MONO_USABLE_HEIGHT` | **96U** (패널 100% 활용) |
| `ST7735S_MONO_FRAME_BYTES` | 256×96/8 = 3072 byte |
| 라인 송신 바이트 (컨트롤러 86 px × 2 byte) | 172 byte |

### Phase 1: 256x96 mono 좌표계 매크로 추가
- 수정 파일: `USER/Drive/ST7735S_Drv/ST7735S_Drv.h`.
- 수정 내용:
  - `MONO_WIDTH=256`, `MONO_HEIGHT=96`, `MONO_FRAME_BYTES=3072` 추가.
  - `MONO_USABLE_WIDTH=256`, `MONO_USABLE_HEIGHT=96`으로 확정 추가 (Phase 0에서 패널 100% 활용 가능함이 확인됨).
  - `MONO_TO_CTRL_X(x)`, `MONO_SUBPX(x)`, `MONO_TO_RAM_X(x)`, `MONO_TO_RAM_Y(y)` 매크로 추가. `MONO_TO_RAM_*`은 4단계 변환의 1-step 압축형이며, 매크로 주석에 4단계 식을 병기한다.
  - `VIEW_X_MAX = 108U` 확정 (Phase 0 점검 완료), `VIEW_Y_MIN = 1U`, `VIEW_Y_MAX = 96U` 확정. 헤더 내 기존 stale 주석(`LOGICAL_WIDTH=85`, `LOGICAL_HEIGHT=94`)은 이 단계에서 86 / 96으로 갱신.
  - 기존 `LOGICAL_*`, `VIEW_*` 매크로는 그대로 두되, "legacy" 주석 부여.
- 리스크: 좌표계 혼용. 매크로명을 `MONO_*` 접두사로 구분.
- 검증: 빌드 통과 및 매크로 값 확인.

### Phase 2: 1bpp framebuffer와 dot get/set 함수 추가
- 수정 파일: `USER/Drive/ST7735S_Drv/ST7735S_Drv.c`.
- 수정 내용: `static uint8_t s_mono_fb[MONO_FRAME_BYTES];` 정적 배열, `mono_fb_set_dot()`, `mono_fb_get_dot()`, `mono_fb_clear()` 내부 함수 추가. 외부 노출 X.
- 리스크: SRAM 사용량 증가(3072 B). 다른 모듈과의 정적 메모리 충돌 가능성 점검.
- 검증: 단위 동작(특정 좌표 set 후 get 일치) 확인.

### Phase 3: mono3_to_rgb565 packing 함수 추가
- 수정 파일: `ST7735S_Drv.c`.
- 수정 내용: `mono3_to_rgb565(r_on, g_on, b_on)` 내부 함수 추가. Phase 0 결과를 반영해 subpixel→채널 매핑 매크로 정의.
- 리스크: BGR/RGB 순서 오판. Phase 0 결과를 한 곳에서만 정의하도록 매크로화.
- 검증: 흰색 단일 mono dot 입력 시 R/G/B 한 채널만 켜진 RGB565 값이 반환되는지 검산.

### Phase 4: ClearMonoBuffer / ClearMono / DrawMonoDot / FlushMono 구현 계획
- 상태: **완료** (2026-04-24).
- 수정 파일: `ST7735S_Drv.c`, `ST7735S_Drv.h`.
- 수정 내용:
  - 신규 API 구현. `FlushMono`는 한 라인(VIEW_X_MAX=108 기준 86 컨트롤러 픽셀 × 2 byte = 172 byte) 단위로 펼쳐 SPI 송신.
  - `DrawMonoDot`은 framebuffer만 변경, 즉시 flush 없음.
  - `ClearMonoBuffer`는 framebuffer만 채움, 즉시 flush 없음(표준 화면 구성 진입점).
  - `ClearMono`는 내부에서 즉시 `FlushMono` 호출(테스트/디버그/긴급 전환 전용).
  - `set_window` 등 하위 함수에 전달하는 좌표는 항상 `ram_*` 기준임을 주석으로 명시(4단계 변환의 마지막 단계).
  - 헤더 주석에 Draw/Flush 호출 규약 명문화:
    - Draw 계열 / `ClearMonoBuffer`: "Does NOT flush. Caller must call ST7735S_Drv_FlushMono*()."
    - `ClearMono`: "Performs immediate FlushMono internally. Prefer ClearMonoBuffer for normal screen composition."
- 리스크: VIEW_X_MIN/Y_MIN 오프셋 적용 위치 혼동. 컨트롤러 좌표 변환은 단일 함수에 집중. App이 Flush 호출을 누락하면 화면이 갱신되지 않는 함정. `ClearMono`와 `ClearMonoBuffer`를 혼동해 `ClearMono`를 화면 구성에 쓰면 깜빡임 발생.
- 검증:
  - 흰색 1 dot 출력 후 `FlushMono` 호출 → 실제 패널에서 가로 1 dot만 켜지는지 확인.
  - Flush 호출 없이 Draw만 실행했을 때 화면에 변화가 없음을 확인(규약 준수 검증).
  - `ClearMonoBuffer → Draw → FlushMono` 1회 흐름으로 깜빡임 없이 화면이 갱신되는지 확인.

#### Phase 4 결과 (확정)

**구현 완료 API**

| API | flush 여부 | 용도 |
|---|---|---|
| `ST7735S_Drv_ClearMonoBuffer(on)` | ✗ | 표준 화면 구성 진입점 |
| `ST7735S_Drv_ClearMono(on)` | ✓ (즉시) | 테스트/디버그/긴급 전체 지움 전용 |
| `ST7735S_Drv_DrawMonoDot(x, y, on)` | ✗ | 1 dot 갱신 |
| `ST7735S_Drv_FlushMono()` | — | 전체 송신 |
| `ST7735S_Drv_FlushMonoRect(x0, y0, x1, y1)` | — | 부분 송신 (컨트롤러 픽셀 경계 자동 정렬) |

**검증 결과 (ScreenManager BOOT 분기 임시 십자선 패턴)**

| 항목 | 기대 | 관측 | 판정 |
|---|---|---|---|
| 세로선 (x=128, y=10..85) 두께 | 1 mono dot | 1 mono dot | ✓ |
| 가로선 (y=48, x=10..245) 두께 | 1 mono dot | 1 mono dot | ✓ |
| 가로/세로 두께 동일성 | 동일 | 동일 (십자선) | ✓ packing 정상 |
| 모서리 4점 (0,0)/(255,0)/(0,95)/(255,95) | 패널 가장자리에 1 dot | 4점 모두 가장자리에 1×1 | ✓ VIEW 범위 정확 |

→ Phase 4 합격. mono → RGB565 packing 경로와 4단계 좌표 변환식이 모두 정상 동작.

**임시 코드 처리**: `ScreenManager_App.c` BOOT 분기의 `#if 1 /* PHASE4_TEST */`를 `#if 0`으로 변경하여 비활성화. 이후 빌드/실행 시 BOOT/Standby 화면 정상 표시 재확인 완료.

### Phase 5: DrawChar3x5 / DrawString3x5를 mono 기준으로 전환 (1-step, atomic commit)
- 상태: **완료** (2026-04-24).
- 수정 파일: `ST7735S_Drv.c`, `ST7735S_Drv.h`, 호출부(`USER/App/BatteryDisplay_App/BatteryDisplay_App.c`, `USER/App/ScreenManager_App/ScreenManager_App.c`).
- 수정 내용:
  - **1-step 방식**: 함수명은 `ST7735S_Drv_DrawChar3x5`, `ST7735S_Drv_DrawString3x5` 그대로 유지하고, 의미를 mono 좌표 + on/off로 전환.
  - 시그니처의 인자 타입/개수를 바꿔 RGB565 색상 인자 기반 호출이 **컴파일 에러로 강제 식별**되도록 함.
  - 내부적으로 `DrawMonoDot`을 호출. 즉시 flush 없음. 호출자가 마지막에 `FlushMono*` 호출.
  - 헤더 주석에 "기존 RGB565 색상 인자 기반 의미는 제거되었음. on=1 → dot ON, on=0 → dot OFF." 명시.
  - **Atomic single commit 원칙**: 드라이버 측 시그니처 변경과 App 측 호출부 수정을 하나의 commit으로 묶어 적용. 중간에 빌드가 깨진 commit이 남지 않도록 함.
- 리스크:
  - 시그니처 변경과 호출부 수정이 다른 commit에 나뉘면 빌드 불능 상태가 남아 git bisect/리버전 복구 곤란.
  - 1-step의 특성상 철저한 사전 검색 필요: 다른 모듈에서도 `DrawChar3x5`/`DrawString3x5` 호출이 있는지 전체 grep 1회 필수.
- 검증: 3x5 문자열을 (0, 0) ~ (mono 좌표) 위치에 출력 후 `FlushMono` 호출 → 가로 늘어짐 없음을 확인.

#### Phase 5 결과 (확정)

**시그니처 교체 (atomic single commit)**

| 함수 | Before | After |
|---|---|---|
| `ST7735S_Drv_DrawChar3x5` | `(uint16_t x, uint16_t y, char ch, uint16_t fg_rgb565, uint16_t bg_rgb565)` | `(uint16_t mono_x, uint16_t mono_y, char ch, uint8_t on)` |
| `ST7735S_Drv_DrawString3x5` | `(uint16_t x, uint16_t y, const char *text, uint16_t fg_rgb565, uint16_t bg_rgb565)` | `(uint16_t mono_x, uint16_t mono_y, const char *text, uint8_t on)` |

**구현 방식**: `DrawChar3x5`는 글리프 ON 비트만 `ST7735S_Drv_DrawMonoDot()`로 framebuffer set. 배경 dot은 건드리지 않음(투명). 즉시 flush 없음. `DrawString3x5`는 4 mono dot씩 cursor 전진하며 `DrawChar3x5` 호출.

**호출부 동시 수정 (총 11 사이트)**
- `ScreenManager_App.c`: 9 사이트 (BOOT/STANDBY/WAIT_CODECHIP/WAIT_CASSETTE/MEASURING/CALCULATING/RESULT_DISPLAY/POWER_OFF_NOTICE/BAT_LOW overlay).
- `BatteryDisplay_App.c`: 2 사이트 (레벨 텍스트, '>' 마커).

**검증**: 빌드 통과 + 코드 정합 확인. 시각 검증은 화면 흐름이 표준 흐름으로 전환된 Phase 6 이후에 의미를 가지므로 Phase 6 결과로 합산.

### Phase 6: BatteryDisplay / ScreenManager 좌표 영향 범위 정리
- 상태: **완료** (2026-04-24, ScreenManager 한정 시각 검증. BatteryDisplay는 코드 적용 완료, 시각 검증은 후속 미션으로 보류).
- 수정 파일: `USER/App/BatteryDisplay_App/BatteryDisplay_App.c`, `USER/App/ScreenManager_App/ScreenManager_App.c`.
- 수정 내용:
  - `TEXT_X`, `MARK_X`, `LINE0_Y`, `LINE_STEP` 등 좌표 상수를 mono 단위로 재계산. 함수 호출명은 동일.
  - **표준 화면 구성 흐름으로 일괄 전환**: `ST7735S_Drv_ClearMonoBuffer() → Draw*()... → ST7735S_Drv_FlushMono()` (또는 `FlushMonoRect()`) 1회 호출.
  - 기존 `ClearMono` 즉시 flush 경로 호출을 `ClearMonoBuffer + 마지막 FlushMono` 흐름으로 교체(깜빡임 방지).
- 리스크: 기존 "논리 좌표" 보정값과 mono 좌표 환산값을 혼동할 가능성. 환산 표를 주석으로 남길 것. Flush 호출 누락 시 화면이 갱신되지 않음. `ClearMono` vs `ClearMonoBuffer` 혼동 주의.
- 검증: 두 화면 출력 후 텍스트 위치가 의도한 영역에 있는지, 마지막 Flush 1회로 전체 장면이 깜빡임 없이 한 번에 갱신되는지 확인.

#### Phase 6 결과 (확정)

**ScreenManager 표준 흐름 전환**: `ST7735S_Drv_Clear(SCREEN_BG_COLOR)` 단일 호출 → `ST7735S_Drv_ClearMonoBuffer(0U)` + 함수 끝 `ST7735S_Drv_FlushMono()` 1회.

**좌표 매크로 신설 (ScreenManager_App.c)**

| 매크로 | 값 | 비고 |
|---|---|---|
| `SCREEN_TEXT_X` | 6U | 기존 logical 2 → mono 6 (×3) |
| `SCREEN_TEXT_Y` | 2U | y 1:1 |
| `SCREEN_BAT_LOW_Y` | 12U | y 1:1 |
| `SCREEN_BG_COLOR` / `SCREEN_TEXT_COLOR` | — | `[LEGACY]` 주석 부착 후 미사용 보존 (Phase 7에서 정리) |

**좌표 매크로 갱신 (BatteryDisplay_App.c)**: `TEXT_X 3 → 9`, `MARK_X 59 → 177` (×3). Y는 1:1 유지. `ClearMonoBuffer + 마지막 FlushMono` 흐름으로 전환.

**검증 결과**

| 항목 | 기대 | 관측 | 판정 |
|---|---|---|---|
| 빌드 통과 | OK | OK | ✓ |
| BOOT/STANDBY/INSERT CODECHIP 등 모든 화면 텍스트 출력 | 깜빡임 없이 1회 | 깜빡임 없이 1회 | ✓ |
| 3x5 글자 가로 폭 | mono dot 3 폭, 글자 간격 4 mono dot | 의도한 대로 | ✓ 가로 늘어짐 제거 |
| BatteryDisplay 화면 시각 검증 | — | (보류) | 후속 미션 |

→ 핵심 목표인 "가로 3배 늘어짐 제거"가 ScreenManager 모든 화면에서 달성됨. BatteryDisplay는 코드 적용은 완료됐고, 실제 화면 시각 검증은 사용자 요청에 따라 후속 미션으로 분리.

### Phase 7: 기존 WriteFrame / 4bpp 경로 legacy 처리
- 상태: **완료** (2026-04-24).
- 수정 파일: `ST7735S_Drv.h`, `ST7735S_Drv.c` (주석/문서위주, 동작 변경 없음).
- 수정 내용: `ST7735S_Drv_WriteFrame`, `gray_lut_565`, 4bpp 변환 코드에 "LEGACY: not used in mono path" 주석 추가. 헤더에서도 별도 섹션으로 분리.
- 리스크: 향후 4bpp 자산 도입 시 혼동. 분기 시점에 다시 검토.
- 검증: 코드 검색으로 신규 호출자가 없음을 확인.

#### Phase 7 결과 (확정)

**헤더 (`ST7735S_Drv.h`)**
- `ST7735S_Drv_WriteFrame`, `ST7735S_Drv_Clear` 선언을 `[LEGACY] RGB565 / 4bpp 경로` 배너 블록 하단으로 이동 + 각 함수 주석에 대체 대안과 `[LEGACY]` 태그 명시.
- `ST7735S_DRV_WIDTH/HEIGHT/FRAME_BYTES`는 `[LEGACY] 4bpp 경로용` 주석 유지.
- `ST7735S_PANEL_WIDTH/HEIGHT`는 Init 단계의 panel-wide clear/tuning에서 여전히 사용되므로 **legacy 처리 제외** (주석으로 용도 명시).

**소스 (`ST7735S_Drv.c`)**
- `ST7735S_Drv_WriteFrame` 앞에 `[LEGACY] RGB565 / 4bpp 경로 구현부` 배너 블록 주석 추가 (`gray_lut_565`, `st7735s_draw_pixel` 포함 범위 명시).
- `ST7735S_Drv_WriteFrame` / `st7735s_draw_pixel` / `ST7735S_Drv_Clear` 각 함수 doxygen `@note`에 `[LEGACY]` + mono 경로 대체 함수명 명시.

**신규 호출자 검증**

| 함수 | USER/App 경로 호출 | USER/Drive 내부 호출 |
|---|---|---|
| `ST7735S_Drv_WriteFrame` | 0 | 0 |
| `ST7735S_Drv_Clear` | 0 | 0 |
| `st7735s_draw_pixel` | — (static) | 4 (`ST7735S_Drv_Clear` 코너 마커 전용) |
| `gray_lut_565` | — (함수 내 지역) | 1 (`ST7735S_Drv_WriteFrame` 내부) |

→ 신규 App 경로에서는 RGB565 / 4bpp 함수 사용 0건 재확인. ILINK dead-strip이 legacy 함수를 제거하는지 여부는 map 파일에서 추후 확인 가능(동작 단계 검증 불필요).

### Phase 8: 성능 개선을 위한 line/rect flush 검토
- 상태: **완료** (2026-04-24).
- 수정 파일: `ST7735S_Drv.c` (드라이버 내부 최적화, 공개 API 시그니처 변경 없음).
- 수정 내용: dirty line bitmap(96 bit = 12 byte) 도입. `mono_fb_clear`/`mono_fb_set_dot`이 dirty 마킹 책임, `mono_flush_ctrl_rect`는 요청 범위 내 연속 dirty span 단위로 set_window+RAMWR+송신을 분할 수행
- 리스크: 외부 요인(노이즈/리셋 등)으로 패널이 깨졌을 때 dirty=0이면 복구 불가. 필요 시 강제 flush API를 신규로 도입 가능(명시적 호출 필요 관점에서만 트리거).
- 검증: 코드 인스펙션으로 dirty 명도화 확인. 동일 화면 재렌더 시 2회차 SPI 송신이 0으로 수렴됨을 설계상 보장. 실측 값은 필요 시 GPIO 토글로 추후 측정.

#### Phase 8 결과 (확정)

**드라이버 내부 구조**

| 항목 | 내용 |
|---|---|
| dirty bitmap 크기 | `(MONO_HEIGHT + 7) / 8 = 12 byte` (`s_mono_dirty[12]`) |
| dirty 단위 | mono_y 라인 1개 (= 컨트롤러 픽셀 ctrl_y 1:1) |
| dirty set | `mono_fb_clear()` → 전 라인 set, `mono_fb_set_dot()` → mono_y 라인 set |
| dirty clear | `mono_flush_ctrl_rect()` 송신 완료 시 해당 라인 clear |
| span 분할 | RAMWR 자동 진행 특성으로 clean line 중간 skip 불가 → 연속 dirty span 단위 set_window/RAMWR 분할 수행 |

**공개 API 영향**
- `ST7735S_Drv_ClearMonoBuffer`/`DrawMonoDot`/`FlushMono`/`FlushMonoRect`/`ClearMono`/`DrawChar3x5`/`DrawString3x5` 시그니처 및 동작 의미 변경 없음.
- App 측 코드 수정 0건.

**기대 효과**
- 동일 화면 재렌더 시 SPI 송신 0 byte. 일부 영역만 변경되는 장면(예: 배터리 텍스트만 갱신)에서는 해당 mono_y span만 송신 → SPI 원각 절감.
- 첫 프레임은 `ClearMonoBuffer`가 전 라인 dirty 마킹 하므로 기존과 동일하게 전체 송신 보장.

**SRAM 용량**
- `s_mono_dirty[12]` 증가 → 기존 mono framebuffer 3072 byte + 12 byte = **3084 byte**. 다른 모듈과 합산 점검 필요성 낮음.

**잔존 리스크**
- 노이즈/외부 요인으로 패널이 깨졌을 때 dirty=0 상태라면 다음 FlushMono에서도 관련 영역 재송신이 일어나지 않음. 완화는 추후 `ST7735S_Drv_FlushMonoForce()` 같은 강제 flush API 추가 검토(현재 도입은 보류 — 필요 증거 없음).

## Verification Plan
- [x] Phase 0 R/G/B 단색 픽셀 출력 → 실제 mono dot이 가로 어느 위치에 켜지는지 확인 후 subpixel 매핑 표 확정. → `0→B, 1→G, 2→R` 확정.
- [x] Phase 0 VIEW_X 확장 검증: `VIEW_X_MAX=108` 적용 상태에서 mono_x=255 위치의 dot이 실제로 켜지는지 확인. 실패 시 109 후보 시도. → **108 채택** (109는 패널 밖).
- [x] Phase 0 VIEW_Y 확장 검증: mono_y=0 및 mono_y=95 dot이 실제 패널 상/하단에서 켜지는지 확인, `MONO_USABLE_HEIGHT` 확정. → `VIEW_Y_MIN=1`, `VIEW_Y_MAX=96` 채택, `MONO_USABLE_HEIGHT=96` 확정.
- [x] 4단계 변환식 검증: mono_x=0 입력 시 `ram_x = RAM_OFFSET_X + VIEW_X_MIN`과 일치, mono_x=MONO_WIDTH-1 입력 시 `ram_x`가 예상 최대값과 일치. 오프셋이 두 번 더해지지 않는지 확인. → 모서리 4점이 패널 4 꼭짓점에 1×1로 정확히 출력 확인 (Phase 4).
- [x] 흰색 1 mono dot 출력 + `FlushMono` 시 가로 1 dot만 켜지는지 육안 확인. → 십자선 패턴에서 가로/세로 두께 동일 확인 (Phase 4).
- [x] mono_x = 0..255 범위에 1 dot 굵기 세로선을 그려 누락/겹침 없는지 확인. → 가로선 (x=10..245) 1 dot 굵기 확인 (Phase 4).
- [ ] 1 dot checkerboard 패턴 출력으로 채널 누설/이웃 dot 영향 점검.
- [x] 3x5 문자열 출력 후 글자 가로 폭이 의도한 mono dot 폭과 일치하는지 확인. → Phase 6에서 가로 늘어짐 제거 확인 (글자폭 3 mono dot, 간격 4 mono dot).
- [ ] Draw만 실행하고 Flush를 호출하지 않았을 때 화면에 변화가 없음을 확인(Draw/Flush 규약 준수).
- [x] `ClearMonoBuffer → Draw → FlushMono` 표준 흐름으로 깜빡임 없이 화면 장면이 한 번에 갱신됨을 확인. → ScreenManager 전 화면 합격 (Phase 6).
- [ ] BatteryDisplay 화면 구성 후 명시적 Flush 1회로 정상 출력 확인. → 검증 보류 (BatteryDisplay 모듈 자체가 임시 테스트용이었으며 ScreenManager로 책임 이관 검토 중 → 모듈 폐기 시 항목 자체 소멸 예정).
- [x] ScreenManager 각 상태 화면(BOOT, INSERT CODECHIP 등)이 Flush 1회로 정상 출력 확인. → BOOT/STANDBY/INSERT CODECHIP 등 모든 화면에서 합격 (Phase 6).
- [x] CodeChip 삽입/제거 이벤트에 따른 화면 전환에 영향 없는지 확인. → 버튼 입력 및 코드칩 삽입 시 텍스트 전환 깔끔하게 동작 육안 합격 (2026-04-24).
- [x] Phase 5 atomic commit 원칙 준수: 드라이버 시그니처 변경과 호출부 수정이 동일 commit으로 적용되었는지 git log 검증. → 단일 작업 단위로 시그니처 + 11 호출부 동시 변경 완료.
- [x] 리팩터 전/후 사진 비교로 가로 늘어짐 제거 확인. → Phase 6에서 육안 합격.

## Risks
- ~~MADCTL 0x08(BGR) 가정에 따른 subpixel 순서 오판.~~ → **Phase 0에서 해소**: `0→B, 1→G, 2→R` 확정.
- ~~VIEW_X 1 dot 부족(85 px → 255 dot)~~ → **Phase 0에서 해소**: `VIEW_X_MAX=108`으로 확장해 mono_x=255까지 커버.
- ~~VIEW_Y 2 line 부족(94 line → 96 line 목표)~~ → **Phase 0에서 해소**: `VIEW_Y_MIN=1`, `VIEW_Y_MAX=96`으로 96 line 전체 사용 확정.
- 신규 MONO 좌표와 기존 LOGICAL 좌표가 코드 내에 혼재될 수 있음. 매크로명/주석으로 구분.
- `VIEW_X_MIN`, `VIEW_Y_MIN` 컨트롤러 픽셀 오프셋이 mono 좌표 변환 단계에서 중복 적용될 위험. 4단계 변환식 명문화 + 클램프는 mono 진입 시 1회만 규약 준수.
- 4bpp WriteFrame legacy 경로와 신규 mono 경로의 동시 호출 시 framebuffer 일관성 깨짐. 한 화면에서 둘 중 하나만 사용하도록 규약 필요.
- 픽셀 단위 SPI 전송으로 회귀하면 성능 저하. 라인 단위 flush를 표준으로.
- SRAM 추가 사용 3072 B + 향후 dirty line bitmap. 다른 모듈 정적 사용량과 합산 점검.
- **Draw/Flush 규약 누락 위험**: Draw만 호출하고 Flush를 빼먹으면 "그렸는데 화면이 안 나온다" 버그 발생. 헤더 주석으로 명문화하고 App 코드에 Flush 1회 호출 패턴 고정.
- **`ClearMono` vs `ClearMonoBuffer` 혼동 위험**: 화면 구성에 `ClearMono`(즉시 flush)를 잘못 고르면 검은 화면 → 재그리기의 2단계 장면 전환으로 깜빡임 발생. 헤더 주석과 plan으로 용도를 명확히 안내.
- **Phase 5 시그니처 변경 비-atomic commit 위험**: 드라이버 변경과 App 호출부 수정이 서로 다른 commit에 나뉘면 빌드 불능 상태 commit 잔존. atomic single commit 원칙 준수 필요.
- 1-step 함수명 유지 방식의 특성상, 코드 검색/문서상 "이름은 같은데 의미가 다른 이전/이후 코드"가 혼동을 일으킬 수 있음. CHANGELOG 또는 커밋 메시지로 명확히 구분.
- **`ReadMonoDot` 공개 시 드라이버의 캡슐화 파괴**: 외부 App이 framebuffer 내부 상태를 읽기 시작하면 포맷 변경(dirty bitmap 통합 등) 시 외부 호출자까지 영향이 번짐. 1차 리팩터에서는 internal-only로 두고 필요 시 debug 가드로 재도입.

## Rollback Plan
- 기존 RGB565 기반 함수(`ST7735S_Drv_WriteFrame`, 컬러 인자 기반 Draw 함수 등)는 1차 리팩터에서 삭제하지 않고 legacy로 보존한다.
- 신규 mono API는 별도 함수로 추가한 뒤 단계적으로 호출자를 옮긴다.
- 문제가 발생하면 App 측 호출만 기존 RGB565 경로로 되돌려 동작을 즉시 복구할 수 있도록 한다.
- Phase별로 commit을 분리해 특정 Phase로 git revert가 가능하도록 한다.

## Acceptance Criteria
- 상위 App은 256x96 mono 좌표계만 사용하며, RGB565 색상값을 직접 다루지 않는다.
- 흰색 1 픽셀 출력 + Flush 시 패널에서 가로 1 dot만 켜진다.
- 3x5 텍스트의 세로획이 가로로 3배 두꺼워 보이지 않는다.
- BatteryDisplay와 ScreenManager의 모든 화면이 정상 출력된다.
- 기존 4bpp WriteFrame 경로는 legacy로 명확히 분리되어 신규 호출자가 없다.
- Phase 0의 subpixel 매핑(`0→B, 1→G, 2→R`)이 코드 한 곳에서만 정의되고 다른 곳에서 재정의되지 않는다.
- `MONO_WIDTH/HEIGHT`는 패널 사양(256/96)로 고정, `MONO_USABLE_WIDTH/HEIGHT`는 Phase 0 결과에 따라 확정된 실효 값으로 설정된다.
- mono → ram 변환은 4단계 공식을 따르며, `RAM_OFFSET_*`/`VIEW_*_MIN`은 계산 경로에서 따로 한 번씩만 더해진다(이중 적용 없음).
- 외부 App은 `DrawChar3x5`/`DrawString3x5`에 RGB565 색상 인자를 넘기지 않는다(시그니처 변경으로 강제 검출).
- App은 한 화면 구성 후 반드시 `ST7735S_Drv_FlushMono()` 또는 `ST7735S_Drv_FlushMonoRect()`를 1회 호출한다(`ClearMono` 제외).
- ScreenManager/BatteryDisplay의 화면 구성은 표준 흐름 `ClearMonoBuffer() → Draw*() → FlushMono()`를 따르며, 화면 구성에는 `ClearMono`(즉시 flush) 경로를 사용하지 않는다.
- `ST7735S_Drv_ClearMono()`는 내부에서 즉시 Flush를 수행하며, 용도(테스트/디버그/긴급 전체 지움)와 예외 사유가 헤더 주석과 plan에 명시되어 있다.
- 1차 리팩터에서는 framebuffer 읽기 API(`ST7735S_Drv_ReadMonoDot` 등)를 공개하지 않고, 외부 App은 framebuffer 상태를 직접 읽지 않는다.
- Phase 5 시그니처 변경과 호출부 수정이 동일 commit으로 적용되어 중간에 빌드 불능 commit이 남지 않는다.
