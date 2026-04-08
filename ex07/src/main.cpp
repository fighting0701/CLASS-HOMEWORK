/*
 * ESP32 Web无极调光器 - 实验7
 * 通过网页滑动条控制LED亮度
 *
 * 功能说明:
 * 1. ESP32工作在AP模式,创建WiFi热点供手机/电脑连接
 * 2. 用户连接热点后打开网页,拖动滑动条调节PWM占空比
 * 3. PWM信号控制LED实现无极调光效果
 *
 * 硬件连接:
 * - LED正极 -> GPIO 25 (通过220Ω电阻)
 * - LED负极 -> GND
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ==================== WiFi AP模式配置 ====================
// 热点名称: ESP32_Dimmer
// 连接密码: 12345678
// AP IP地址: 192.168.4.1 (固定)
const char* ap_ssid = "ESP32_Dimmer";
const char* ap_password = "12345678";

// ==================== PWM配置 ====================
// LED引脚: GPIO 25
// PWM频率: 5KHz (适合人眼观察,无闪烁)
// 分辨率: 8位 (0-255)
#define LED_PIN 25
#define PWM_FREQ 5000
#define PWM_RES 8

// ==================== Web服务器 ====================
// 使用WebServer库,监听80端口
WebServer server(80);

// ==================== 状态变量 ====================
// 当前亮度值: 0(最暗) - 255(最亮)
// 初始值为128(中等亮度)
int brightness = 128;

/**
 * 生成网页HTML
 * 包含: 滑动条(input type="range") + 实时亮度显示
 * 滑动条通过fetch发送GET请求到/set?value=xxx
 */
String makePage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>无极调光器</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin-top: 50px;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
    }
    h1 { font-size: 2.5em; margin-bottom: 30px; }
    .slider-container {
      background: rgba(255,255,255,0.2);
      padding: 30px;
      border-radius: 15px;
      display: inline-block;
      margin: 20px;
    }
    /* 滑动条样式 */
    input[type="range"] { width: 300px; height: 30px; cursor: pointer; }
    .brightness-value { font-size: 1.5em; margin-top: 15px; }
    .info { margin-top: 20px; font-size: 0.9em; opacity: 0.8; }
  </style>
</head>
<body>
  <h1>ESP32 无极调光器</h1>
  <div class="slider-container">
    <!-- 滑动条: min=0最暗, max=255最亮 -->
    <input type="range" id="brightness" min="0" max="255" value=")rawliteral" + String(brightness) + R"rawliteral(" onchange="sendBrightness(this.value)" oninput="updateDisplay(this.value)">
    <div class="brightness-value">亮度: <span id="display">)rawliteral" + String(brightness) + R"rawliteral(</span></div>
  </div>
  <div class="info">
    <p>拖动滑动条实时调节LED亮度</p>
    <p>范围: 0 (最暗) - 255 (最亮)</p>
  </div>

  <script>
    // 更新显示的数值(实时反馈,拖动时变化)
    function updateDisplay(value) {
      document.getElementById('display').textContent = value;
    }
    // 发送亮度值到ESP32(松开滑动条时触发)
    function sendBrightness(value) {
      fetch('/set?value=' + value)
        .then(response => response.text())
        .then(data => console.log(data));
    }
  </script>
</body>
</html>
)rawliteral";
  return html;
}

/**
 * 处理根路径("/")请求
 * 返回网页HTML内容
 */
void handleRoot() {
  server.send(200, "text/html; charset=UTF-8", makePage());
}

/**
 * 处理亮度设置请求("/set?value=xxx")
 * 解析URL参数,更新PWM输出
 */
void handleSetBrightness() {
  if (server.hasArg("value")) {
    brightness = server.arg("value").toInt();  // 字符串转整数
    ledcWrite(0, brightness);                   // 写入PWM通道
    Serial.printf("亮度设置为: %d\n", brightness);
    server.send(200, "text/plain", "OK: " + String(brightness));
  } else {
    server.send(400, "text/plain", "Missing value");
  }
}

/**
 * 初始化函数
 */
void setup() {
  Serial.begin(115200);
  delay(1000);

  // 配置LEDC通道0: 频率5KHz, 8位分辨率
  ledcSetup(0, PWM_FREQ, PWM_RES);
  // 绑定GPIO 25到LEDC通道0
  ledcAttachPin(LED_PIN, 0);
  // 初始化亮度
  ledcWrite(0, brightness);

  // 启动AP模式(ESP32创建WiFi热点)
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("AP模式已启动");
  Serial.print("热点名称: ");
  Serial.println(ap_ssid);
  Serial.print("IP地址: http://");
  Serial.println(WiFi.softAPIP());

  // 注册路由
  server.on("/", handleRoot);           // 网页首页
  server.on("/set", handleSetBrightness); // 亮度控制
  server.begin();
}

/**
 * 主循环: 处理客户端请求
 */
void loop() {
  server.handleClient();
}
