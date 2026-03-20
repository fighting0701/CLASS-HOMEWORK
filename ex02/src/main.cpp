#include <Arduino.h>

/*
 * Ex02: 1Hz LED Blink using millis()
 *
 * 原理说明:
 * - millis() 返回系统启动后的毫秒数
 * - 通过比较当前时间和上次执行时间来实现定时
 * - 优点: 不阻塞CPU，可以同时执行其他任务
 *
 * 1Hz = 1秒闪烁一次 = 亮500ms + 灭500ms
*/

const int LED_PIN = 2;           // ESP32 默认 LED 引脚 (GPIO2)
const unsigned long INTERVAL = 500;  // 闪烁间隔 500ms

unsigned long previousMillis = 0;    // 上次LED状态改变的时间
int ledState = LOW;                  // 当前LED状态

void setup() {
    pinMode(LED_PIN, OUTPUT);   // 设置LED引脚为输出模式
    Serial.begin(115200);       // 初始化串口
    Serial.println("Ex02: 1Hz LED Blink with millis()");
    Serial.println("LED will blink at 1Hz (500ms ON, 500ms OFF)");
}

void loop() {
    unsigned long currentMillis = millis();  // 获取当前时间

    // 检查是否达到定时间隔
    if (currentMillis - previousMillis >= INTERVAL) {
        previousMillis = currentMillis;  // 更新上次时间

        // 翻转LED状态
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);

        // 打印当前状态
        Serial.print("LED: ");
        Serial.println(ledState == HIGH ? "ON" : "OFF");
    }
    // 注意: 这里可以添加其他非阻塞的任务
}
