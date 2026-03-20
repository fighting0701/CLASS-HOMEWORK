#include <Arduino.h>

// SOS LED with millis()
// SOS = ...---... (3短 3长 3短)
// 短闪: 200ms, 长闪: 600ms

const int LED_PIN = 2;
const unsigned long SHORT = 200;
const unsigned long LONG = 600;
const unsigned long GAP = 200;
const unsigned long WORD_DELAY = 1000;

int pattern[] = {
    SHORT, SHORT, SHORT,  // 3次短闪 ... (S)
    GAP,
    LONG, LONG, LONG,     // 3次长闪 --- (O)
    GAP,
    SHORT, SHORT, SHORT,  // 3次短闪 ... (S)
    WORD_DELAY             // 单词间隔
};

int patternIndex = 0;
unsigned long previousMillis = 0;
int ledState = LOW;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);
    Serial.println("Ex03: SOS LED Blink");
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= pattern[patternIndex]) {
        previousMillis = currentMillis;

        // LED 翻转
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);

        // 移动到下一个模式
        patternIndex++;
        if (patternIndex >= sizeof(pattern) / sizeof(pattern[0])) {
            patternIndex = 0;
            ledState = LOW;
            digitalWrite(LED_PIN, LOW);
        }
    }
}
