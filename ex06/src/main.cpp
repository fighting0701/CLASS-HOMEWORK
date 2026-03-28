#include <Arduino.h>


#define LED_PIN_A 25    // LED A 引脚 (GPIO 25)
#define LED_PIN_B 33    // LED B 引脚 (GPIO 33)
#define PWM_FREQ 5000   // PWM 频率 5KHz
#define PWM_RES 8       // 8位分辨率 (0-255)

// ========== 状态变量 ==========
int brightness = 0;       // 当前亮度值
int fadeDirection = 1;    // 亮度变化方向：1=增加, -1=减少
const int STEP = 3;        // 亮度步进值
const int SPEED_DELAY = 15; // 每步延时(毫秒)

void setup() {
    Serial.begin(115200);
    delay(1000);

    // 配置LEDC通道A (GPIO 25)
    ledcSetup(0, PWM_FREQ, PWM_RES);   // 通道0, 5KHz, 8位
    ledcAttachPin(LED_PIN_A, 0);        // GPIO 25 绑定到通道0

    // 配置LEDC通道B (GPIO 33)
    ledcSetup(1, PWM_FREQ, PWM_RES);   // 通道1, 5KHz, 8位
    ledcAttachPin(LED_PIN_B, 1);        // GPIO 33 绑定到通道1

    // 初始状态：灯A最亮，灯B熄灭
    ledcWrite(0, 0);     // 灯A初始亮度0
    ledcWrite(1, 255);   // 灯B初始亮度255

    Serial.println("Ex06: Police Light Flashing Effect");
    Serial.println("LED A: GPIO 25, LED B: GPIO 33");
}

void loop() {
    // 更新亮度值
    brightness += STEP * fadeDirection;

    // 边界检测：达到最大或最小亮度时反转方向
    if (brightness >= 255) {
        brightness = 255;
        fadeDirection = -1;
    } else if (brightness <= 0) {
        brightness = 0;
        fadeDirection = 1;
    }

    // 灯A: 直接使用当前亮度值
    ledcWrite(0, brightness);

    // 灯B: 使用反相值 (255 - brightness)
    // 当灯A最亮(255)时，灯B熄灭(0)
    // 当灯A熄灭(0)时，灯B最亮(255)
    ledcWrite(1, 255 - brightness);

    delay(SPEED_DELAY);
}
