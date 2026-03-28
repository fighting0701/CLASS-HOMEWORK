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
 *
 * 硬件连接：
 * - 触摸引脚 T0 (GPIO 4)
 * - LED引脚 GPIO 2 (ESP32内置LED)
 *
 * 工作原理：
 * 1. 边缘检测：通过比较 lastTouched 和 currentlyTouched
 *    - lastTouched = false (上一次未触摸)
 *    - currentlyTouched = true (当前被触摸)
 *    - 条件满足说明检测到"按下瞬间"
 *
 * 2. 软件防抖：50ms内的重复触发会被忽略
 *    - 记录上次触发时间 lastDebounceTime
 *    - 只有当前时间与上次触发时间差 > DEBOUNCE_MS 时才响应
 *
 * 3. LED状态翻转：ledState取反，控制LED亮灭
 */

#define TOUCH_PIN 4      // 触摸引脚 (T0)
#define LED_PIN 2        // LED引脚 (ESP32内置)
#define THRESHOLD 20     // 触摸阈值（需根据实际测试修改）
                     // 触摸传感器返回值 < THRESHOLD 时认为被触摸
#define DEBOUNCE_MS 50  // 防抖时间(毫秒)
                     // 防止手抖导致短时间内多次触发

// ========== 状态变量 ==========
bool ledState = false;              // LED当前状态: false=灭, true=亮
bool lastTouched = false;           // 上一次触摸状态，用于边缘检测
                                 // 通过与当前状态对比判断是"按下瞬间"还是"持续触摸"
unsigned long lastDebounceTime = 0; // 上次有效触发的时间戳(毫秒)
                                 // 用于实现软件防抖

void setup() {
    Serial.begin(115200);
    delay(1000);  // 等待串口稳定
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);  // 初始状态LED关闭
    Serial.println("Ex04: Touch LED Toggle");
    Serial.println("Touch pin T0 to toggle LED");
}

void loop() {
    // 读取当前触摸值 (返回值越小表示触摸力度越大)
    int touchValue = touchRead(TOUCH_PIN);

    // 判断当前是否被触摸：返回值小于阈值则认为触摸
    bool currentlyTouched = (touchValue < THRESHOLD);

    // 获取当前时间(毫秒)
    unsigned long currentTime = millis();

    // ========== 边缘检测 ==========
    // 条件: 上一次未触摸 && 当前被触摸
    // 这确保只在"按下瞬间"触发，而不是在持续触摸期间一直触发
    if (currentlyTouched && !lastTouched) {

        // ========== 防抖检查 ==========
        // 只有距离上次有效触发超过防抖时间，才认为是有效触发
        // 这可以防止手抖导致的误触发（手抖可能在短时间内产生多次按下信号）
        if (currentTime - lastDebounceTime > DEBOUNCE_MS) {

            // 翻转LED状态 (亮->灭, 灭->亮)
            ledState = !ledState;

            // 控制LED输出
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);

            // 更新防抖时间记录
            lastDebounceTime = currentTime;

            // 串口输出调试信息
            Serial.print("Touch detected! LED: ");
            Serial.println(ledState ? "ON" : "OFF");
        }
    }

    // 更新上一次触摸状态，为下一次循环的边缘检测做准备
    lastTouched = currentlyTouched;

    delay(10);  // 小延时，避免频繁读取(约100次/秒)
}
