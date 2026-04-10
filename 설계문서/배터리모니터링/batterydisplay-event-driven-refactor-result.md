# BatteryDisplay 이벤트 기반 리팩토링 결과서

## 1. 작업 개요

본 작업은 `BatteryDisplay_App`가 `BatteryMonitor`의 현재 상태를 직접 조회하던 구조를 제거하고,  
`BatteryMonitor`가 생성한 배터리 레벨 이벤트를 소비하여 LCD를 갱신하는 구조로 리팩토링한 결과를 정리한 문서이다.

이번 작업을 통해 배터리 표시 기능은 프로젝트의 이벤트 기반 앱 연동 철학에 맞추어  
**producer-consumer 구조**로 정렬되었다.

---

## 2. 작업 목적

리팩토링의 주요 목적은 다음과 같았다.

- `BatteryDisplay_App`의 직접 상태 polling 제거
- `BatteryMonitor` 이벤트 큐 기반 연동으로 통일
- LCD 표시 경로를 이벤트 소비 방식으로 정리
- 초기 표시 누락 가능성 보완
- `GetLevel()` 기반 직접 조회 경로 제거

---

## 3. 반영 내용

### 3.1 `BatteryDisplay_App` 변경
`BatteryDisplay_App_Run()`의 동작 방식을 다음과 같이 변경하였다.

- 기존:
  - `BatteryMonitor_Interface_GetLevel()`로 현재 상태 직접 조회
  - 이전 표시값과 비교하여 필요 시 redraw

- 변경 후:
  - `BatteryMonitor_Interface_GetEvent()`를 반복 호출하여 대기 중 이벤트 소비
  - 여러 이벤트가 누적된 경우 마지막 이벤트를 기준으로만 화면 갱신
  - 중복 레벨에 대해서는 불필요한 redraw 방지 유지

즉, LCD는 더 이상 배터리 상태를 직접 조회하지 않고,  
**이벤트를 소비한 결과로만 화면을 갱신**하도록 정리되었다.

---

### 3.2 `BatteryMonitor_App` 변경
`BatteryMonitor_App_Task()`의 이벤트 생성 조건을 보강하였다.

- 기존:
  - 배터리 레벨이 이전 값과 다를 때만 이벤트 생성

- 변경 후:
  - **첫 번째 유효 측정 완료 시에는 레벨 변화 여부와 관계없이 초기 상태 이벤트를 1회 발행**
  - 이후에는 실제 레벨 변화가 발생한 경우에만 이벤트 생성

이를 통해 이벤트 기반 구조 전환 시 발생할 수 있는  
**부팅 직후 초기 화면 미표시 문제**를 방지하였다.

---

### 3.3 `GetLevel()` 경로 제거
상태 직접 조회 API인 아래 경로를 정리하였다.

- `BatteryMonitor_App_GetLevel()`
- `BatteryMonitor_Interface_GetLevel()`

처리 순서는 다음 원칙을 따랐다.

1. 먼저 `BatteryDisplay_App`에서 직접 조회 의존 제거
2. 전체 참조 여부 확인
3. 남은 호출처가 없는 것을 확인한 뒤 API 및 구현 삭제

즉, **즉시 삭제가 아니라 참조 제거 확인 후 삭제** 방식으로 안전하게 반영하였다.

---

## 4. 변경 파일

이번 작업에서 주요 변경이 반영된 파일은 다음과 같다.

- `USER/App/BatteryDisplay_App/BatteryDisplay_App.c`
- `USER/App/BatteryMonitor_App/BatteryMonitor_App.c`
- `USER/App/BatteryMonitor_App/BatteryMonitor_App.h`
- `USER/App/BatteryMonitor_App/BatteryMonitor_Interface.c`
- `USER/App/BatteryMonitor_App/BatteryMonitor_Interface.h`

---

## 5. 검증 결과

### 5.1 정적 확인
- `BatteryDisplay_App`에서 `GetLevel()` 직접 호출 제거 확인
- `BatteryMonitor_Interface_GetLevel()` / `BatteryMonitor_App_GetLevel()` 참조 제거 확인

### 5.2 코드 오류 확인
- VS Code 기준 수정 대상 파일에서 **No errors found** 확인

### 5.3 빌드 확인
- **IAR 빌드 정상 확인 완료**

---

## 6. 적용 후 구조

리팩토링 이후 배터리 표시 흐름은 다음과 같이 정리된다.

```text
BatteryMonitor_Interface_Run()
    -> BatteryMonitor_App_Task()
        -> 배터리 상태 측정
        -> 초기/변화 이벤트 생성
        -> 이벤트 큐 저장

BatteryDisplay_App_Run()
    -> BatteryMonitor_Interface_GetEvent()
    -> 대기 이벤트 소비
    -> 마지막 이벤트 기준 LCD 갱신
```

즉,

- `BatteryMonitor` = 이벤트 생산자
- `BatteryDisplay` = 이벤트 소비자

역할이 명확해졌다.

---

## 7. 기대 효과

이번 반영으로 다음 효과를 기대할 수 있다.

- 앱 간 연동 방식이 이벤트 기반으로 일관됨
- 표시 앱이 배터리 모듈 내부 상태 표현에 직접 의존하지 않음
- producer-consumer 구조 설명이 명확해짐
- 향후 알림, 로그, 전력 제어 앱 등으로 구조 확장 시 패턴 재사용이 쉬워짐

---

## 8. 한계 및 후속 과제

이번 작업은 **`BatteryDisplay` 단일 소비자 전환**까지를 범위로 한다.

따라서 다음 항목은 후속 검토 대상이다.

1. **다중 소비자 지원**
   - 현재 `BatteryMonitor` 이벤트 큐는 단일 소비자 구조이다.
   - 여러 앱이 동일 이벤트를 독립적으로 소비해야 한다면 중앙 이벤트 디스패처 또는 pub-sub 구조가 필요하다.

2. **실기 동작 검증 확대**
   - 부팅 직후 첫 화면 표시
   - 배터리 레벨 변화 시 이벤트 기반 갱신
   - 장시간 동작 시 큐 적체 여부

---

## 9. 결론

이번 리팩토링을 통해 `BatteryDisplay_App`는 상태 polling 기반 구조에서 벗어나,  
`BatteryMonitor` 이벤트를 소비하는 구조로 성공적으로 전환되었다.

특히 아래 3가지 핵심 요구사항이 반영되었다.

1. 초기 측정 완료 시 1회 초기 상태 이벤트 발행
2. LCD 표시 경로의 이벤트 소비 기반 전환
3. `GetLevel()` 직접 조회 경로의 안전한 제거

결과적으로 이번 작업은 단순한 표시 로직 수정이 아니라,  
프로젝트의 **이벤트 기반 앱 연동 원칙을 실제 코드 구조에 반영한 정리 작업**으로 볼 수 있다.
