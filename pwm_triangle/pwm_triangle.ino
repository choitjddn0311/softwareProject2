/*
 * 도전과제 2: PWM 함수 구현 (06C17)
 *
 * 회로: 5V - 220ohm - LED - GPIO7
 *   -> LED의 (+)쪽이 5V, (-)쪽이 GPIO7에 연결되는 "Active-LOW" 구조
 *   -> 핀을 LOW로 만들면 전류가 흘러 LED가 켜지고, HIGH면 꺼진다.
 *
 * 동작: 1초 동안 최소밝기 -> 최대밝기 -> 최소밝기 (triangle 패턴) 반복
 *       밝기(duty)는 0 ~ 100, 101단계 (0: 꺼짐, 100: 최대밝기)
 *
 * 주기 변경: 시리얼 모니터(9600bps)에 10000 / 1000 / 100 입력
 *           (10ms / 1ms / 0.1ms) -> 코드 재업로드 없이 영상 3개 녹화 가능
 */

const int  LED_PIN    = 7;      // GPIO 7번 핀
const bool ACTIVE_LOW = true;   // 5V-저항-LED-GPIO 구조이므로 LOW = 켜짐

int g_period = 10000;           // PWM 주기 (us), 100 ~ 10000
int g_duty   = 0;               // duty (%), 0 ~ 100

// ---------------- 과제에서 요구한 함수 ----------------

// 주기 설정: 100 ~ 10000 us
void set_period(int period) {
  if (period < 100)   period = 100;
  if (period > 10000) period = 10000;
  g_period = period;
}

// duty 설정: 0 ~ 100 %
void set_duty(int duty) {
  if (duty < 0)   duty = 0;
  if (duty > 100) duty = 100;
  g_duty = duty;
}

// ---------------- 내부 도우미 함수 ----------------

void led_on()  { digitalWrite(LED_PIN, ACTIVE_LOW ? LOW  : HIGH); }
void led_off() { digitalWrite(LED_PIN, ACTIVE_LOW ? HIGH : LOW ); }

// PWM 한 주기 출력: ON 구간(period * duty%) + OFF 구간(나머지)
void pwm_cycle() {
  // int(16bit)끼리 곱하면 10000*100에서 오버플로가 나므로 unsigned long으로 계산
  unsigned long on_time  = (unsigned long)g_period * g_duty / 100;
  unsigned long off_time = (unsigned long)g_period - on_time;

  if (on_time > 0) {            // duty 0%면 한 번도 켜지 않음
    led_on();
    delayMicroseconds(on_time);
  }
  if (off_time > 0) {           // duty 100%면 한 번도 끄지 않음
    led_off();
    delayMicroseconds(off_time);
  }
}

// 시리얼로 주기 변경 (예: "1000" 입력 후 Enter)
void check_serial() {
  if (Serial.available() > 0) {
    long p = Serial.parseInt();
    if (p > 0) {
      set_period((int)p);
      Serial.print("period = ");
      Serial.print(g_period);
      Serial.println(" us");
    }
    while (Serial.available() > 0) Serial.read();   // 남은 개행문자 등 비우기
  }
}

// ---------------- setup / loop ----------------

void setup() {
  pinMode(LED_PIN, OUTPUT);
  led_off();

  Serial.begin(9600);
  Serial.setTimeout(10);

  set_period(10000);            // 기본 10ms (1000 또는 100으로 바꿔서 테스트)
  set_duty(0);

  Serial.println("Enter period in us: 10000 / 1000 / 100");
}

void loop() {
  check_serial();

  // 1초(1000ms) 주기 triangle 파형으로 duty 계산
  //   0 ~ 500ms   : duty 0 -> 100 (밝아짐)
  //   500 ~ 1000ms: duty 100 -> 0 (어두워짐)
  unsigned long t = millis() % 1000;
  int duty;
  if (t < 500) duty = (int)(t * 100 / 500);
  else         duty = (int)((1000 - t) * 100 / 500);

  set_duty(duty);
  pwm_cycle();                  // 현재 duty로 PWM 한 주기 출력
}
