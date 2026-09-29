// 08P07 : 거리에 따른 LED 밝기 제어
// - 샘플링 주기 25ms, delay() 없이 millis() 폴링으로 주기 유지
// - 200mm에서 최대 밝기, 100mm/300mm에서 최소(꺼짐), 150mm/250mm에서 duty 50%
// - LED는 active low: analogWrite(pin, 0) = 가장 밝음, analogWrite(pin, 255) = 꺼짐

// Arduino pin assignment
#define PIN_LED  9    // PWM 핀이어야 함 (~9), analogWrite()로 밝기 제어
#define PIN_TRIG 12   // sonar sensor TRIGGER
#define PIN_ECHO 13   // sonar sensor ECHO

// configurable parameters
#define SND_VEL 346.0     // sound velocity at 24 celsius degree (unit: m/sec)
#define INTERVAL 25       // sampling interval (unit: msec)
#define PULSE_DURATION 10 // ultra-sound Pulse Duration (unit: usec)
#define _DIST_MIN 100.0   // minimum distance to be measured (unit: mm)
#define _DIST_MAX 300.0   // maximum distance to be measured (unit: mm)
#define _DIST_PEAK 200.0  // 최대 밝기가 되는 거리 (unit: mm)

// - (INTERVAL / 2)는 정수 나눗셈이라 25 / 2 = 12 → 12 * 1000 = 12000us
// - 12ms 동안 왕복 가능한 거리 = 약 2m (편도) → 300mm 측정에는 충분
// - 한 루프 최악 시간: pulseIn 12ms + 시리얼 출력 약 9ms < 25ms 이므로 주기 유지 가능
#define TIMEOUT ((INTERVAL / 2) * 1000.0) // maximum echo waiting time (unit: usec)
#define SCALE (0.001 * 0.5 * SND_VEL) // coefficent to convert duration to distance

unsigned long last_sampling_time = 0;   // unit: msec

void setup() {
  // initialize GPIO pins
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);  // sonar TRIGGER
  pinMode(PIN_ECHO, INPUT);   // sonar ECHO
  digitalWrite(PIN_TRIG, LOW);  // turn-off Sonar 
  analogWrite(PIN_LED, 255);    // 시작 시 LED OFF (active low)
  
  // initialize serial port
  Serial.begin(57600);
}

void loop() { 
  float distance;
  int duty;   // analogWrite() 값: 0 = 최대 밝기, 255 = 꺼짐 (active low)

  // wait until next sampling time. // polling
  // millis() returns the number of milliseconds since the program started.
  //    will overflow after 50 days.
  // → "현재 시각 - 마지막 시각" 형태로 비교하면 unsigned 연산 특성상
  //   millis()가 0으로 돌아가는 오버플로에도 정상 동작함
  // → 샘플링 주기(25ms)가 안 됐으면 바로 반환 (delay() 사용 안 함)
  if (millis() - last_sampling_time < INTERVAL)
    return;

  distance = USS_measure(PIN_TRIG, PIN_ECHO); // read distance

  if ((distance == 0.0) || (distance > _DIST_MAX)) {
      distance = _DIST_MAX + 10.0;    // Set Higher Value
      duty = 255;                     // LED OFF
  } else if (distance < _DIST_MIN) {
      distance = _DIST_MIN - 10.0;    // Set Lower Value
      duty = 255;                     // LED OFF
  } else {    // In desired Range
      // 삼각형(V자) 형태의 밝기 곡선
      //   duty = 255 * |distance - 200| / 100
      //   100mm -> 255 (꺼짐)
      //   150mm -> 127.5 -> 128 (50%)
      //   200mm -> 0 (최대 밝기)
      //   250mm -> 127.5 -> 128 (50%)
      //   300mm -> 255 (꺼짐)
      //
      // [data type 주의]
      // - 정수로 계산하면 |d - 200| / 100 이 0 또는 1이 되어 밝기가 단계적으로만 바뀜
      // - 그래서 fabs()와 float 나눗셈으로 비율(0.0 ~ 1.0)을 먼저 구한 뒤 int로 변환
      float ratio = fabs(distance - _DIST_PEAK) / (_DIST_MAX - _DIST_PEAK);

      // + 0.5 : 반올림 (150/250mm에서 128). 버림(127)을 원하면 + 0.5를 지우면 됨
      duty = (int)(255.0 * ratio + 0.5);
      duty = constrain(duty, 0, 255);   // analogWrite 허용 범위(0~255)로 제한
  }

  analogWrite(PIN_LED, duty);   // LED 밝기 적용

  // output the distance to the serial port
  // - duty는 active low 값이라 플로터에서 V자로 보이며, 0에 가까울수록 밝음
  Serial.print("Min:");        Serial.print(_DIST_MIN);
  Serial.print(",distance:");  Serial.print(distance);
  Serial.print(",Max:");       Serial.print(_DIST_MAX);
  Serial.print(",duty:");      Serial.print(duty);
  Serial.println("");
  
  // update last sampling time
  // - millis()를 다시 읽지 않고 INTERVAL만큼 더해 주기 누적 오차 방지
  last_sampling_time += INTERVAL;
}

// get a distance reading from USS. return value is in millimeter.
// - 측정 실패(TIMEOUT 초과) 시 pulseIn()이 0을 반환하므로 결과도 0.0
float USS_measure(int TRIG, int ECHO)
{
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);
  
  return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE; // unit: mm

  // Pulse duration to distance conversion example (target distance = 17.3m)
  // - pulseIn(ECHO, HIGH, timeout) returns microseconds (음파의 왕복 시간)
  // - 편도 거리 = (pulseIn() / 1,000,000) * SND_VEL / 2 (미터 단위)
  //   mm 단위로 하려면 * 1,000이 필요 ==>  SCALE = 0.001 * 0.5 * SND_VEL
  //
  // - 예, pusseIn()이 100,000 이면 (= 0.1초, 왕복 거리 34.6m)
  //        = 100,000 micro*sec * 0.001 milli/micro * 0.5 * 346 meter/sec
  //        = 100,000 * 0.001 * 0.5 * 346
  //        = 17,300 mm  ==> 17.3m
}
