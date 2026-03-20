#include <Arduino.h>

// 使用 millis() 实现非阻塞 LED 闪烁
// 1Hz = 1秒闪烁一次 = 亮500ms，灭500ms

const int LED_PIN = 2;  // ESP32 默认 LED 引脚
unsigned long previousMillis = 0;
const long interval = 500;  // 500ms 亮 + 500ms 灭 = 1Hz

void setup() {
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);
    Serial.println("Ex02: 1Hz LED Blink with millis()");
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        Serial.println("LED Toggled!");
    }
}
