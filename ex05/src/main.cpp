#include <Arduino.h>

/*
 * Ex05: Multi-Speed Touch Breathing LED
 *
 * 实验要求：
 * - 结合实验3 (PWM呼吸灯) 和实验4 (触摸引脚)
 * - 定义速度档位变量 (1、2、3档)
 * - 每次触摸引脚，档位循环切换：1->2->3->1...
 * - 根据档位改变呼吸灯速度
 *
 * 预期结果：
 * - LED持续呈呼吸灯效果
 * - 每触摸一次，LED"呼吸"的节奏发生改变
 * - 三个明显速度级别：缓慢、中速、急促
 */

#define TOUCH_PIN 4        // 触摸引脚 (T0)
#define LED_PIN 2          // LED引脚 (ESP32内置)
#define THRESHOLD 20       // 触摸阈值
#define DEBOUNCE_MS 50   // 防抖时间(毫秒)

// ========== 速度档位定义 ==========
#define SPEED_LEVELS 3     // 速度档位数量
int speedLevel = 1;       // 当前速度档位 (1=慢速, 2=中速, 3=快速)

// 速度参数：每档对应的亮度步进延迟(毫秒)
// 值越大，呼吸越缓慢
const int SPEED_DELAY[SPEED_LEVELS] = {30, 10, 3};

// ========== 呼吸灯参数 ==========
const int MAX_BRIGHTNESS = 255;  // 最大亮度 (PWM最大值)
const int MIN_BRIGHTNESS = 0;    // 最小亮度 (熄灭)
const int STEP = 5;              // 亮度步进值

// ========== 状态变量 ==========
bool lastTouched = false;           // 上一次触摸状态
unsigned long lastDebounceTime = 0; // 防抖时间戳
int brightness = 0;                 // 当前亮度
int fadeDirection = 1;              // 亮度变化方向：1=增加, -1=减少

void setup() {
    Serial.begin(115200);
    delay(1000);
    pinMode(LED_PIN, OUTPUT);

    // ESP32 LEDC PWM 配置
    // 通道0, 5KHz频率, 8位分辨率(0-255)
    ledcSetup(0, 5000, 8);
    // 将LED引脚绑定到通道0
    ledcAttachPin(LED_PIN, 0);
    // 初始亮度设为0(熄灭)
    ledcWrite(0, brightness);

    Serial.println("Ex05: Multi-Speed Touch Breathing LED");
    Serial.println("Touch T0 to change speed level");
    Serial.print("Current speed level: ");
    Serial.println(speedLevel);
}

void loop() {
    // ========== 触摸速度切换检测 ==========
    // 读取当前触摸值
    int touchValue = touchRead(TOUCH_PIN);
    // 判断当前是否被触摸：返回值小于阈值则认为触摸
    bool currentlyTouched = (touchValue < THRESHOLD);
    unsigned long currentTime = millis();

    // 边缘检测 + 防抖
    // 条件：上一次未触摸 && 当前被触摸（按下瞬间）
    if (currentlyTouched && !lastTouched) {
        // 防抖：距离上次触发超过防抖时间才响应
        if (currentTime - lastDebounceTime > DEBOUNCE_MS) {
            // 档位循环切换：1->2->3->1
            speedLevel++;
            if (speedLevel > SPEED_LEVELS) {
                speedLevel = 1;
            }
            lastDebounceTime = currentTime;

            Serial.print("Speed level changed to: ");
            Serial.println(speedLevel);
        }
    }
    lastTouched = currentlyTouched;

    // ========== 呼吸灯效果 ==========
    // 根据当前速度档位调整亮度
    brightness += STEP * fadeDirection;

    // 亮度达到上限或下限，反转方向
    if (brightness >= MAX_BRIGHTNESS) {
        brightness = MAX_BRIGHTNESS;
        fadeDirection = -1;
    } else if (brightness <= MIN_BRIGHTNESS) {
        brightness = MIN_BRIGHTNESS;
        fadeDirection = 1;
    }

    // 使用PWM控制LED亮度
    ledcWrite(0, brightness);

    // 延时根据速度档位变化
    // 档位1(30ms)：缓慢呼吸
    // 档位2(10ms)：中速呼吸
    // 档位3(3ms)：急促呼吸
    delay(SPEED_DELAY[speedLevel - 1]);
}
