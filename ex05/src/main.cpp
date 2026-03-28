#include <Arduino.h>



#define TOUCH_PIN 4        // 触摸引脚 (T0)
#define LED_PIN 2          // LED引脚 (ESP32内置)
#define THRESHOLD 20       // 触摸阈值
#define DEBOUNCE_MS 50     // 防抖时间(毫秒)

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
int fadeDirection = 1;               // 亮度变化方向：1=增加, -1=减少

void setup() {
    Serial.begin(115200);
    delay(1000);
    pinMode(LED_PIN, OUTPUT);
    ledcSetup(0, 5000, 8);           // 通道0, 5KHz频率, 8位分辨率
    ledcAttachPin(LED_PIN, 0);      // LED_PIN绑定到通道0
    ledcWrite(0, brightness);       // 初始亮度

    Serial.println("Ex05: Multi-Speed Touch Breathing LED");
    Serial.println("Touch T0 to change speed level");
    Serial.print("Current speed level: ");
    Serial.println(speedLevel);
}

void loop() {
    // ========== 触摸速度切换检测 ==========
    int touchValue = touchRead(TOUCH_PIN);
    bool currentlyTouched = (touchValue < THRESHOLD);
    unsigned long currentTime = millis();

    // 边缘检测 + 防抖
    if (currentlyTouched && !lastTouched) {
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
    delay(SPEED_DELAY[speedLevel - 1]);
}
