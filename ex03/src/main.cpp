#include <Arduino.h>

/*
 * Ex03: SOS LED Blink using millis()
 *
 * 原理说明:
 * - 使用数组存储SOS闪烁模式，通过索引依次执行
 * - SOS = ...---... (3短 3长 3短)
 * - 短闪(DIT): 200ms
 * - 长闪(DAH): 600ms
 * - 闪烁间隔: 200ms
 * - 单词间隔: 1000ms
 *
 * 优点: 代码简洁，易于修改闪烁模式
*/

const int LED_PIN = 2;  // ESP32 默认 LED 引脚

// 闪烁时间常量 (毫秒)
const unsigned long SHORT = 200;      // 短闪 '滴'
const unsigned long LONG = 600;       // 长闪 '滴-'
const unsigned long GAP = 200;        // 闪烁间隔
const unsigned long WORD_DELAY = 1000; // 单词间隔

// SOS 模式数组: 依次执行每个时间值
// ... --- ... (3短 3长 3短)
int pattern[] = {
    SHORT, SHORT, SHORT,   // S: 3次短闪
    GAP,                   // 间隔
    LONG, LONG, LONG,      // O: 3次长闪
    GAP,                   // 间隔
    SHORT, SHORT, SHORT,   // S: 3次短闪
    WORD_DELAY             // 单词间隔后重复
};

// 状态变量
int patternIndex = 0;           // 当前播放到的模式索引
unsigned long previousMillis = 0; // 上次执行时间
int ledState = LOW;             // LED当前状态

void setup() {
    pinMode(LED_PIN, OUTPUT);   // 设置LED引脚为输出
    Serial.begin(115200);       // 初始化串口
    Serial.println("Ex03: SOS LED Blink with millis()");
    Serial.println("Pattern: ...---... (3 short, 3 long, 3 short)");
}

void loop() {
    unsigned long currentMillis = millis();  // 获取当前时间

    // 检查是否达到当前模式的时间
    if (currentMillis - previousMillis >= pattern[patternIndex]) {
        previousMillis = currentMillis;  // 更新执行时间

        // 翻转LED状态 (亮 -> 灭, 灭 -> 亮)
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);

        Serial.print("Index: ");
        Serial.print(patternIndex);
        Serial.print(", Time: ");
        Serial.print(pattern[patternIndex]);
        Serial.print("ms, LED: ");
        Serial.println(ledState == HIGH ? "ON" : "OFF");

        // 移动到下一个模式
        patternIndex++;

        // 如果播完一轮，重置索引
        if (patternIndex >= sizeof(pattern) / sizeof(pattern[0])) {
            patternIndex = 0;
            ledState = LOW;
            digitalWrite(LED_PIN, LOW);  // 确保LED关闭
            Serial.println("--- SOS Complete, Repeat ---");
        }
    }
    // 注意: 这里可以同时执行其他非阻塞任务
}
