#include <Arduino.h>

/*
 * Ex06: Police Light Flashing Effect (Dual-Channel PWM)
 *
 * 实验要求：
 * - 面包板上连接两个LED至不同引脚
 * - 初始化两个独立的PWM通道
 * - 两个灯呈现"反相"关系
 * - 当灯A从0增加到255时，灯B从255减小到0
 *
 * 预期结果：
 * - 两个LED平滑交替渐变闪烁
 * - 一个变亮时另一个逐渐变暗
 * - 过渡非常柔和
 */

#define LED_PIN_A 25    // LED A 引脚 (GPIO 25)
#define LED_PIN_B 33    // LED B 引脚 (GPIO 33)
#define PWM_FREQ 5000   // PWM 频率 5KHz
#define PWM_RES 8       // 8位分辨率 (0-255)

// ========== 状态变量 ==========
int brightness = 0;       // 当前亮度值
int fadeDirection = 1;  // 亮度变化方向：1=增加, -1=减少
const int STEP = 3;       // 亮度步进值(越大变化越快)
const int SPEED_DELAY = 15; // 每步延时(毫秒)(越大闪烁越慢)

void setup() {
    Serial.begin(115200);
    delay(1000);

    // 配置LEDC通道A (GPIO 25)
    // ledcSetup(通道号, 频率, 分辨率)
    // 通道0, 5KHz频率, 8位分辨率(0-255)
    ledcSetup(0, PWM_FREQ, PWM_RES);
    // 将GPIO 25绑定到LEDC通道0
    ledcAttachPin(LED_PIN_A, 0);

    // 配置LEDC通道B (GPIO 33)
    // 通道1, 5KHz频率, 8位分辨率(0-255)
    ledcSetup(1, PWM_FREQ, PWM_RES);
    // 将GPIO 33绑定到LEDC通道1
    ledcAttachPin(LED_PIN_B, 1);

    // 初始状态设置
    // 注意：初始时灯A亮度0，灯B亮度255
    // 这是因为在loop中灯B使用255-brightness计算
    // 初始brightness=0时，灯B=255-0=255(最亮)
    ledcWrite(0, 0);     // 灯A初始亮度0(熄灭)
    ledcWrite(1, 255);  // 灯B初始亮度255(最亮)

    Serial.println("Ex06: Police Light Flashing Effect");
    Serial.println("LED A: GPIO 25, LED B: GPIO 33");
}

void loop() {
    // 更新亮度值：根据方向增加或减少
    brightness += STEP * fadeDirection;

    // 边界检测：达到最大或最小亮度时反转方向
    if (brightness >= 255) {
        brightness = 255;
        fadeDirection = -1;  // 开始变暗
    } else if (brightness <= 0) {
        brightness = 0;
        fadeDirection = 1;   // 开始变亮
    }

    // 灯A: 直接使用当前亮度值
    // brightness=0时灯A最暗，brightness=255时灯A最亮
    ledcWrite(0, brightness);

    // 灯B: 使用反相值
    // 当brightness=0(灯A最暗)时，255-0=255(灯B最亮)
    // 当brightness=255(灯A最亮)时，255-255=0(灯B最暗)
    // 实现两个灯此亮彼暗的反相效果
    ledcWrite(1, 255 - brightness);

    delay(SPEED_DELAY);
}
