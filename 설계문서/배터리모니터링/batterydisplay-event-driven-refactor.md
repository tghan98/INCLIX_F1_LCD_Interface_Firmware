# BatteryDisplay 이벤트 기반 리팩토링 설계 문서

## 1. 목적

`BatteryDisplay_App`가 `BatteryMonitor`의 현재 상태를 직접 조회하는 구조를 제거하고,  
`BatteryMonitor`가 생성한 배터리 레벨 변화 이벤트를 소비하여 화면을 갱신하는 구조로 변경한다.

이번 리팩토링의 최종 목표는 다음과 같다.

- `BatteryDisplay_App`는 배터리 상태를 직접 조회하지 않는다.
- `BatteryMonitor`는 배터리 상태 변화 및 초기 상태를 이벤트로 전달한다.
- LCD는 이벤트를 소비한 결과로만 화면을 갱신한다.
- `GetLevel()` 기반 상태 조회 경로는 **참조 제거를 확인한 뒤 삭제**한다.

---

## 2. 배경

현재 `BatteryMonitor` 인터페이스는 아래 3가지 접근 경로를 동시에 제공하고 있다.

- `BatteryMonitor_Interface_Run()`
- `BatteryMonitor_Interface_GetLevel()`
- `BatteryMonitor_Interface_GetEvent()`, `BatteryMonitor_Interface_GetEventCount()`

즉, `BatteryMonitor`는 이벤트 큐를 이미 제공하고 있음에도,  
`BatteryDisplay_App`는 상태 polling 방식과 이벤트 방식 중 polling 방식으로 연동될 수 있는 구조를 갖고 있다.

이 구조는 다음 문제를 만든다.

1. 앱 간 연동 방식이 일관되지 않다.
2. `BatteryDisplay_App`가 `BatteryMonitor`의 내부 상태 표현에 직접 의존하게 된다.
3. 이벤트 큐가 존재하지만, 표시 경로의 주 통신 수단으로 사용되지 않는다.
4. 향후 producer-consumer 구조 설명이 불분명해진다.

따라서 이번 리팩토링에서는 `BatteryDisplay_App`가 오직 이벤트만 소비하도록 정리하고,  
상태 직접 조회 API인 `GetLevel()`은 **모든 참조 제거가 확인된 뒤 삭제**한다.

---

## 3. 현재 구조

### 3.1 `BatteryMonitor` 역할
`BatteryMonitor_App`는 비차단 상태 머신 기반으로 배터리 레벨을 판정한다.

- 상태 흐름: `INIT -> START -> WAIT_READY -> END`
- 3개의 threshold를 순차 비교
- 최종 배터리 레벨 산출
- 레벨 변화 시 이벤트 큐에 push

### 3.2 `BatteryDisplay` 역할
`BatteryDisplay_App`는 현재 배터리 레벨을 직접 조회하여 표시한다.

현재 구조의 특징은 아래와 같다.

- `BatteryDisplay_App`가 `BatteryMonitor`의 현재 상태를 직접 읽는다.
- 표시 갱신 여부는 `s_displayed_level` 비교로 결정한다.
- 이벤트 큐가 존재해도 LCD 표시는 직접 조회 방식에 의존할 수 있다.

### 3.3 문제점
이 구조는 책임 분리를 흐리게 만든다.

- `BatteryMonitor`는 “이벤트 생산자”로 보이지만,
- `BatteryDisplay`는 실제로는 “상태 조회자”로 동작한다.

즉, 시스템 구조를 producer-consumer로 설명하기 어려워진다.

---

## 4. 목표 구조

### 4.1 기본 원칙
이번 리팩토링의 원칙은 다음과 같다.

- 앱 간 연동은 이벤트 기반으로 통일한다.
- `BatteryMonitor`는 배터리 상태 이벤트 생산자이다.
- `BatteryDisplay`는 배터리 상태 이벤트 소비자이다.
- LCD는 최신 이벤트를 기준으로만 화면을 갱신한다.
- `GetLevel()`은 유지하지 않고 바로 삭제하지 않으며, **참조 제거 확인 후 삭제**한다.

### 4.2 목표 흐름

```text
BatteryMonitor_Interface_Run()
    -> BatteryMonitor_App_Task()
        -> 배터리 레벨 판정
        -> 이벤트 큐 push

BatteryDisplay_App_Run()
    -> BatteryMonitor_Interface_GetEvent()
    -> 대기 중 이벤트 반복 소비
    -> 마지막 이벤트 기준으로 LCD 갱신
```

### 4.3 초기 이벤트 보장
이벤트 기반 구조로 전환할 때 가장 중요한 보완 사항은 **초기 상태 이벤트 보장**이다.

다음 원칙을 적용한다.

- **첫 번째 유효 측정 완료 시에는 레벨 변화 여부와 관계없이 초기 상태 이벤트를 1회 발행한다.**
- 이후부터는 실제 배터리 레벨 변화가 발생했을 때만 이벤트를 발행한다.

이 보완이 필요한 이유는 현재 초기값이 이미 특정 레벨로 설정되어 있을 수 있기 때문이다.  
이 경우 첫 실측값이 초기값과 동일하면 변화 이벤트가 발생하지 않아,  
LCD가 이벤트만 기다릴 경우 초기 화면이 표시되지 않을 수 있다.

### 4.4 이벤트 소비 정책
`BatteryDisplay_App`는 배터리 이벤트를 소비할 때 아래 정책을 따른다.

- 대기 중인 이벤트를 `Run()` 주기에서 반복 소비한다.
- 이벤트가 여러 개 누적되어 있더라도 LCD는 **마지막 이벤트 기준으로만** 화면을 갱신한다.
- 중간 이벤트를 하나씩 모두 재생하지 않는다.
- 이미 표시 중인 레벨과 동일한 경우 불필요한 redraw는 수행하지 않는다.

---

## 5. 리팩토링 범위

이번 리팩토링 범위는 다음과 같이 제한한다.

- `BatteryDisplay_App`를 **단일 소비자(single consumer)** 로서 `BatteryMonitor` 이벤트 큐를 소비하도록 변경한다.
- `BatteryMonitor`는 표시 앱이 사용할 수 있도록 초기 상태 이벤트 보장을 포함한 이벤트 생산자로 정리한다.
- `BatteryDisplay`의 직접 상태 조회 경로는 제거한다.

다만 아래 내용은 **이번 범위에 포함하지 않는다.**

- 다중 앱이 하나의 동일 배터리 이벤트를 동시에 소비하는 구조
- 전역 이벤트 브로커/디스패처 설계
- pub-sub 프레임워크 도입

정리하면 다음과 같다.

> **이번 리팩토링 범위는 `BatteryDisplay` 단일 소비자 전환까지로 한정한다.**  
> **다중 앱 동시 소비가 필요해질 경우 후속으로 중앙 이벤트 디스패처(pub-sub)를 설계한다.**

---

## 6. `GetLevel()` 처리 방침

`GetLevel()` 기반 직접 조회 경로는 장기적으로 제거하는 것이 목표이지만,  
삭제 시점은 안전하게 관리해야 한다.

따라서 다음 순서를 따른다.

1. 먼저 `BatteryDisplay_App`에서 `GetLevel()` 의존을 제거한다.
2. 전체 프로젝트에서 `BatteryMonitor_Interface_GetLevel()` 참조 여부를 확인한다.
3. 남은 호출처가 없을 경우에만 API 및 구현을 삭제한다.

즉, 이번 작업에서는

> **“즉시 삭제”보다 “참조 제거 확인 후 삭제”** 원칙을 적용한다.

---

## 7. 기대 효과

이번 리팩토링이 완료되면 다음 효과를 기대할 수 있다.

- 앱 간 통신 방식이 이벤트 기반으로 일관된다.
- `BatteryDisplay_App`가 `BatteryMonitor` 내부 상태 표현에 직접 의존하지 않게 된다.
- producer-consumer 구조를 명확하게 설명할 수 있다.
- 향후 경고, 로그, 전력 제어 등 다른 앱 확장 시 동일 패턴으로 연계하기 쉬워진다.

---

## 8. 결론

이번 리팩토링은 단순히 LCD 갱신 방식만 바꾸는 작업이 아니라,  
프로젝트 전체의 앱 연동 철학을 **이벤트 기반으로 명확히 정렬하는 작업**이다.

핵심 반영 사항은 다음 3가지다.

1. **첫 번째 유효 측정 완료 시에는 레벨 변화 여부와 관계없이 초기 상태 이벤트를 1회 발행한다.**
2. **이번 리팩토링 범위는 `BatteryDisplay` 단일 소비자 전환까지로 한정한다. 다중 앱 동시 소비는 후속 pub-sub 구조에서 다룬다.**
3. **`GetLevel()`은 즉시 삭제하지 않고, 참조 제거 확인 후 삭제한다.**
