/*
 * ESP32 物联网安防报警器 - 实验8
 * 通过网页布防/撤防 + 触摸引脚触发报警
 *
 * 功能说明:
 * 1. ESP32工作在AP模式,创建WiFi热点供手机/电脑连接
 * 2. 用户通过网页控制布防/撤防状态
 * 3. 布防状态下触摸引脚触发报警,LED高频闪烁
 * 4. 报警锁定后必须网页撤防才能解除
 *
 * 硬件连接:
 * - LED正极 -> GPIO 25 (通过220Ω电阻)
 * - LED负极 -> GND
 * - 触摸引脚 -> GPIO 14 (T6)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ==================== WiFi AP模式配置 ====================
const char* ap_ssid = "ESP32_Security";
const char* ap_password = "12345678";

// ==================== 硬件引脚定义 ====================
#define LED_PIN 25       // LED报警灯引脚
#define TOUCH_PIN T6     // 触摸引脚 (GPIO 14)
#define TOUCH_THRESHOLD 40 // 触摸阈值(低于此值表示被触摸)

// ==================== 系统状态定义 ====================
#define STATE_DISARMED 0  // 撤防状态
#define STATE_ARMED 1     // 布防状态
#define STATE_ALARM 2     // 报警状态

// ==================== 全局状态变量 ====================
int systemState = STATE_DISARMED;  // 当前系统状态
bool ledState = false;             // LED当前状态

// ==================== Web服务器 ====================
WebServer server(80);

// ==================== 状态标识 ====================
const char* stateNames[] = {"撤防", "布防", "报警"};

/**
 * 生成网页HTML
 * 显示当前状态 + Arm/Disarm按钮
 */
String makePage() {
  String armBtn = "";
  String disarmBtn = "";
  
  // 根据当前状态显示不同按钮
  if (systemState == STATE_DISARMED) {
    armBtn = "<a href=\"/arm\"><button class=\"btn arm-btn\">布防(Arm)</button></a>";
    disarmBtn = "";
  } else if (systemState == STATE_ARMED) {
    armBtn = "";
    disarmBtn = "<a href=\"/disarm\"><button class=\"btn disarm-btn\">撤防(Disarm)</button></a>";
  } else if (systemState == STATE_ALARM) {
    armBtn = "";
    disarmBtn = "<a href=\"/disarm\"><button class=\"btn disarm-btn\">撤防(Disarm)</button></a>";
  }

  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>物联网安防报警器</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin-top: 50px;
      background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
      color: white;
    }
    h1 { font-size: 2.5em; margin-bottom: 20px; }
    .status-container {
      background: rgba(255,255,255,0.1);
      padding: 20px;
      border-radius: 15px;
      display: inline-block;
      margin: 20px;
      min-width: 300px;
    }
    .status-label { font-size: 1.2em; opacity: 0.8; }
    .status-value { font-size: 2em; font-weight: bold; margin-top: 10px; }
    .status-armed { color: #00ff00; }
    .status-disarmed { color: #888; }
    .status-alarm { color: #ff0000; animation: blink 0.3s infinite; }
    @keyframes blink { 0%,100%{opacity:1;} 50%{opacity:0.3;} }
    .btn {
      padding: 15px 40px;
      font-size: 1.2em;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      margin: 10px;
    }
    .arm-btn { background: #00aa00; color: white; }
    .disarm-btn { background: #ff4444; color: white; }
    .info { margin-top: 30px; font-size: 0.9em; opacity: 0.7; }
  </style>
</head>
<body>
  <h1>物联网安防报警器</h1>
  <div class="status-container">
    <div class="status-label">当前状态</div>
    <div class="status-value" id="status">)rawliteral" + String(stateNames[systemState]) + R"rawliteral(</div>
  </div>
  <div style="margin-top: 20px;">
    )rawliteral" + armBtn + R"rawliteral(
    )rawliteral" + disarmBtn + R"rawliteral(
  </div>
  <div class="info">
    <p>提示: 布防后触碰GPIO14引脚将触发报警</p>
  </div>
  <script>
    // 定期刷新页面状态
    setTimeout(function(){ location.reload(); }, 1000);
  </script>
</body>
</html>
)rawliteral";
  return html;
}

void handleRoot() {
  server.send(200, "text/html; charset=UTF-8", makePage());
}

// 布防
void handleArm() {
  systemState = STATE_ARMED;
  Serial.println("系统布防");
  server.sendHeader("Location", "/");
  server.send(303);
}

// 撤防
void handleDisarm() {
  systemState = STATE_DISARMED;
  digitalWrite(LED_PIN, LOW);  // 关闭LED
  ledState = false;
  Serial.println("系统撤防");
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 初始化LED引脚
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 启动AP模式
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("AP模式已启动");
  Serial.print("热点名称: ");
  Serial.println(ap_ssid);
  Serial.print("IP地址: http://");
  Serial.println(WiFi.softAPIP());

  // 注册路由
  server.on("/", handleRoot);
  server.on("/arm", handleArm);
  server.on("/disarm", handleDisarm);
  server.begin();
}

void loop() {
  server.handleClient();

  // 读取触摸值
  int touchValue = touchRead(TOUCH_PIN);
  // Serial.printf("Touch: %d\n", touchValue);

  if (systemState == STATE_ARMED) {
    // 布防状态下检测触摸
    if (touchValue < TOUCH_THRESHOLD) {
      systemState = STATE_ALARM;
      Serial.println("报警触发!");
    }
  }

  if (systemState == STATE_ALARM) {
    // 报警状态: LED高频闪烁
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    delay(100);  // 100ms间隔,高频闪烁
  }
}
