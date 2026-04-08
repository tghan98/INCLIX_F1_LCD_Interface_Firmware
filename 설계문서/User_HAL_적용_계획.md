# User_HAL 적용 계획

## 1. 목적

`INCLIX_F1_LCD_Interface_Firmware_feature_st7735s_replace_ssd1322` 프로젝트에
`IDDD_FW_FUNC__non_OS`의 장점을 참고한 `User_HAL` 구조를 도입한다.

목표는 다음과 같다.

- 보드 의존 `GPIO/HAL` 접근을 `USER/Drive/User_HAL/`에 모은다.
- `App`과 상위 드라이버는 기능 중심 코드만 유지하도록 역할을 분리한다.
- 전원, LCD, DAC 같은 하드웨어 제어를 한 곳에서 관리하도록 정리한다.

---

## 2. 문서 목적

이 문서는 다음 사항을 미리 정리하기 위한 것이다.

- `User_HAL` 도입 범위와 책임 정의
- 구조 변경 시 우선순위와 단계 정리
- 직접 GPIO/HAL 접근이 상위 계층으로 다시 퍼지는 것 방지
- 네이밍 규칙 통일

---

## 3. 적용 원칙

1. `Core/` 자동생성 코드는 가능한 한 유지한다.
2. `App` 계층은 핀 이름과 `HAL_GPIO_*`를 직접 알지 않도록 한다.
3. 보드별 `active-high / active-low` 차이는 `User_HAL` 내부에서만 처리한다.
4. 범용성이 높은 드라이버(`Button_Drv` 등)는 유지하고, 보드 의존 부분만 `User_HAL`로 이동한다.
5. 새 네이밍은 개인 이니셜을 쓰지 않고 역할 기반 이름을 사용한다.

---

## 4. 네이밍 규칙

### 금지
- 개인 이니셜 기반 네이밍 사용 금지
- 예: `hsHW_Config()`, `BSP_hsSysTickTimer()`

### 권장
- `UserHAL_`
- `HW_`
- `BSP_`

### 예시
- `hsHW_Config()` → `UserHAL_Config()`
- `BSP_hsSysTickTimer()` → `BSP_TickTimer()`
- `HS_SW_Delay()` → `UserHAL_SwDelay()`

---

## 5. 대상 하드웨어 자원

### GPIO 핀
- `MPW_ONOFF_Pin`
- `PC_SW_SIG_Pin`
- `EX_PW_CK_Pin`
- `LCD_DC_Pin`
- `LCD_RST_Pin`
- `LCDVCC_EN_Pin`
- `LCD_CS_Pin`

### HAL 핸들
- `hdac1`
- `hspi2`
- `htim14`
- `hcomp2`

---

## 6. 도입 범위

### 포함 범위
- 전원 제어 핀 및 버튼/USB 감지 입력
- LCD 제어 핀 (`CS`, `DC`, `RST`, `VCC`) 및 관련 HAL 핸들 접근
- DAC 출력 제어와 타이머 기반 공통 유틸

### 제외 범위
- `BatteryDisplay_App`, `BatteryMonitor_App`, `PowerControl_App` 상태 머신 재설계
- `Core/` 핀 설정 구조 변경
- 일반 알고리즘/디바운스 로직 대규모 재작성

---

## 7. 권장 `User_HAL` 인터페이스 초안

신규 파일:

- `USER/Drive/User_HAL/User_HAL_Drv.h`
- `USER/Drive/User_HAL/User_HAL_Drv.c`

### 권장 함수 목록

#### 전원 / 입력 계열
- `int32_t UserHAL_Config(void);`
- `void HW_PW_OnOff(uint32_t on_off);`
- `int32_t HW_ReadPin_PWSW_Status(void);`
- `int32_t HW_ReadPin_UsbDetect_Status(void);`
- `void HW_PW_LED_ONnOFF(uint32_t on_off);`
- `void HW_PW_LED_Toggle(void);`
- `int32_t HW_ReadPin_PW_LED_Status(void);`

#### LCD 제어 계열
- `void HW_LCD_Power_ONnOFF(uint32_t on_off);`
- `void HW_LCD_Reset(uint32_t on_off);`
- `void HW_LCD_CS_Select(uint32_t select);`
- `void HW_LCD_DC_Set(uint32_t is_data);`

#### HAL 접근 계열
- `void *Read_LCD_SPI_HalDrive(void);`
- `void *Read_LCD_BL_Timer_HalDrive(void);`
- `void *Read_DAC_HalDrive(void);`
- `void *Read_COMP_HalDrive(void);`

#### 유틸 계열
- `int32_t HW_DAC_CTRL(uint32_t dac_12b_count);`
- `uint32_t BSP_TickTimer(uint32_t *p_tick_timer, uint32_t wait_tick_time);`

---

## 8. 단계별 추진 계획

### 1단계: 하드웨어 접근 지점 인벤토리 정리
- `Core/Inc/main.h`와 `Core/Src/main.c`를 기준으로 핀/핸들 목록 확정
- `PowerControl_Drv.c`, `ST7735S_Drv.c`, `DAC_Manager.c`의 직접 접근 위치 정리

### 2단계: `User_HAL` 인터페이스 정의
- `User_HAL_Drv.h/.c` 생성
- 함수 목록과 책임 범위 확정

### 3단계: 기본 `User_HAL` 골격 추가
- 타깃 보드용 핀/HAL 핸들 매핑 구현
- `main.h`의 CubeMX 정의를 그대로 사용

### 4단계: 기존 드라이버를 `User_HAL` 경유로 전환
- `PowerControl_Drv.c`
  - 버튼 입력, USB 감지, 홀드 핀, LED 제어를 `HW_*` 함수 경유로 변경
- `ST7735S_Drv.c`
  - `LCD_CS/DC/RST/VCC` 직접 제어를 `User_HAL` 함수로 치환
  - `hspi2`, `htim14` 직접 의존은 접근자 함수 또는 래퍼로 통일
- `DAC_Manager.c`
  - DAC 제어를 `HW_DAC_CTRL()` 중심으로 정리

### 5단계: 상위 계층 정리
- `User_Main.c`는 초기화/주기 실행만 유지
- `App` 계층은 직접 GPIO/HAL 접근 없이 인터페이스 함수만 호출하도록 유지

### 6단계: 점진 검증
- 1차: IAR 빌드 확인
- 2차: 전원 버튼 및 `MPW_ONOFF` 홀드 동작 확인
- 3차: LCD 초기화 및 문자열 표시 확인
- 4차: DAC/비교기 기반 배터리 판정 확인
- 5차: 상위 계층에 직접 `HAL_GPIO_*`가 남아 있지 않은지 검색 확인

---

## 9. 우선 작업 순서

### 1순위
`PowerControl_Drv.c`

이유:
- GPIO 접근이 단순함
- `User_HAL` 구조 검증용으로 가장 안전함

### 2순위
`ST7735S_Drv.c`

이유:
- LCD 제어 핀과 `SPI/PWM` 의존이 많아 구조 개선 효과가 큼

### 3순위
`DAC_Manager.c`

이유:
- DAC/COMP 제어를 정리하면 배터리 측정 경로 관리가 쉬워짐

---

## 10. 관련 파일

- `Core/Inc/main.h` — 핀/주변장치 정의 기준점
- `Core/Src/main.c` — HAL 핸들 정의 위치
- `USER/Drive/User_HAL/User_HAL_Drv.h` — 신규 인터페이스 선언 파일
- `USER/Drive/User_HAL/User_HAL_Drv.c` — 신규 구현 파일
- `USER/Drive/PowerControl_Drv/PowerControl_Drv.c` — 1차 이관 대상
- `USER/Drive/ST7735S_Drv/ST7735S_Drv.c` — LCD 제어 분리 핵심 파일
- `USER/Drive/DAC_Manager/DAC_Manager.c` — DAC 제어 정리 대상
- `USER/App/PowerControl_App/PowerControl_App.c` — 상위 호출부 점검 대상
- `USER/User_Main.c` — 초기화/주기 실행 순서 유지 확인

---

## 11. 검증 기준

1. 새 `User_HAL` 추가 후 빌드 오류가 증가하지 않아야 한다.
2. 전원 관련 실기 동작이 기존과 동일해야 한다.
3. LCD 초기화와 문자열 출력이 기존과 동일해야 한다.
4. 배터리 비교 동작과 DAC 설정 결과가 회귀 없이 유지되어야 한다.
5. 상위 계층에서 직접 핀/레지스터/HAL 접근이 줄어들어야 한다.

---

## 12. 결론

이 작업은 기존 구조를 무조건 복사하는 작업이 아니라,
타깃 보드에 맞게 **보드 의존 제어만 `User_HAL`에 집중시키는 구조 정리 작업**으로 진행하는 것이 가장 안전하다.

첫 착수는 `PowerControl_Drv.c`를 대상으로 하고,
구조 검증 후 `ST7735S_Drv.c`, `DAC_Manager.c`로 확장하는 순서를 권장한다.

---

## 13. 결론

이번 적용을 통해 `PowerControl_Drv.c`, `ST7735S_Drv.c`, `DAC_Manager.c` 중심의 하드웨어 의존 코드가 `User_HAL` 경유 구조로 정리되었고,
빌드 및 실기 동작 검증까지 완료되어 실제 적용 가능성도 확인되었다.

향후 신규 보드 의존 제어가 추가될 경우에도,
같은 원칙으로 `User_HAL`에 우선 수용하는 방향을 유지하는 것이 바람직하다.
