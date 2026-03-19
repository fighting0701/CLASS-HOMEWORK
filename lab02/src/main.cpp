#include<Arduino.h>
#define LED_PIN 2
#define LED_PIN1 26

void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  // 初始化板载LED引脚为输出模式
  pinMode(LED_PIN, OUTPUT); 
  pinMode(LED_PIN1, OUTPUT);
}

void loop() {
  Serial.println("Hello ESP32!");
  digitalWrite(LED_PIN, HIGH);   // 点亮LED
  digitalWrite(LED_PIN1, HIGH);
  delay(1000);                   // 等待1秒
  digitalWrite(LED_PIN, LOW);    // 熄灭LED
  digitalWrite(LED_PIN1, LOW);    // 熄灭LED
  delay(1000);              // 等待1秒
}