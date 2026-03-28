#include <Arduino.h>

/*
 * Ex04: Touch LED Toggle
 *
 * 实验要求：
 * - 基础的触摸实验："摸着亮，松开灭"
 * - 引入布尔型状态变量 (bool ledState) 和边缘检测逻辑
 * - 只有在检测到按下瞬间，才翻转LED的状态
 * - 加入软件防抖逻辑，防止手抖导致的误触发
 *
 * 预期结果：
 * - 摸一下触摸引脚，LED亮起并保持长亮
 * - 松开手再摸一下，LED熄灭
 */

#define TOUCH_PIN 4      // 触摸引脚 (T0)
#define LED_PIN 2        // LED引脚 (ESP32内置)
#define THRESHOLD 20     // 触摸阈值（需根据实际测试修改）
#define DEBOUNCE_MS 50   // 防抖时间(毫秒)

bool ledState = false;           // LED当前状态
bool lastTouched = false;        // 上一次触摸状态（用于边缘检测）
unsigned long lastDebounceTime = 0;  // 上次触发时间（防抖用）

void setup() {
    Serial.begin(115200);
    delay(1000);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);  // 初始状态LED关闭
    Serial.println("Ex04: Touch LED Toggle");
    Serial.println("Touch pin T0 to toggle LED");
}

void loop() {
    // 读取当前触摸值
    int touchValue = touchRead(TOUCH_PIN);
    bool currentlyTouched = (touchValue < THRESHOLD);

    unsigned long currentTime = millis();

    // 边缘检测：上一次未触摸 && 当前被触摸（按下瞬间）
    if (currentlyTouched && !lastTouched) {
        // 防抖：距离上次触发是否超过防抖时间
        if (currentTime - lastDebounceTime > DEBOUNCE_MS) {
            ledState = !ledState;                      // 翻转LED状态
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
            lastDebounceTime = currentTime;            // 更新防抖时间

            Serial.print("Touch detected! LED: ");
            Serial.println(ledState ? "ON" : "OFF");
        }
    }

    // 更新上一次触摸状态
    lastTouched = currentlyTouched;

    delay(10);  // 小延时，避免频繁读取
}
