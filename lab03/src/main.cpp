#include<Arduino.h>

// 定义LED引脚
const int ledPin = 2;
const int ledPin1 = 26;

// 定义LEDC通道
const int ledChannel = 0;
const int ledChannel1 = 1;

// 设置PWM属性
const int freq = 5000;          // 频率 5000Hz
const int resolution = 8;       // 分辨率 8位 (0-255)

void setup() {
  Serial.begin(115200);

  // 旧版正确写法
  ledcSetup(ledChannel, freq, resolution);   // 设置通道0
  ledcAttachPin(ledPin, ledChannel);         // 绑定引脚2到通道0

  ledcSetup(ledChannel1, freq, resolution);  // 设置通道1
  ledcAttachPin(ledPin1, ledChannel1);       // 绑定引脚26到通道1
}

void loop() {
  // 逐渐变亮
  for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle++){
    ledcWrite(ledChannel, dutyCycle);
    ledcWrite(ledChannel1, dutyCycle);
    delay(10);
  }

  // 逐渐变暗
  for(int dutyCycle = 255; dutyCycle >= 0; dutyCycle--){
    ledcWrite(ledChannel, dutyCycle);
    ledcWrite(ledChannel1, dutyCycle);
    delay(10);
  }

  Serial.println("Breathing cycle completed");
}