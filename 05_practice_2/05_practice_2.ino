#define LED_PIN 7

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // 1. 처음 1초 동안 LED 켜기 (active-low: LOW = ON)
  digitalWrite(LED_PIN, LOW);
  delay(1000);

  // 2. 다음 1초 동안 5회 깜빡이기 (200ms 주기 = ON 100ms + OFF 100ms)
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, LOW);   // ON
    delay(100);
    digitalWrite(LED_PIN, HIGH);  // OFF
    delay(100);
  }

  // 3. LED 끄고 무한루프에서 종료
  digitalWrite(LED_PIN, HIGH);
  while (1) {} // infinite loop
}
