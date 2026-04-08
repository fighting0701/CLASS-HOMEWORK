#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ap_ssid = "ESP32_Dimmer";
const char* ap_password = "12345678";

#define LED_PIN 25    // LED 引脚 (GPIO 25)
#define PWM_FREQ 5000   // PWM 频率 5KHz
#define PWM_RES 8       // 8位分辨率 (0-255)

WebServer server(80);

// 初始亮度值
int brightness = 128;

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
    h1 {
      font-size: 2.5em;
      margin-bottom: 30px;
    }
    .slider-container {
      background: rgba(255,255,255,0.2);
      padding: 30px;
      border-radius: 15px;
      display: inline-block;
      margin: 20px;
    }
    input[type="range"] {
      width: 300px;
      height: 30px;
      cursor: pointer;
    }
    .brightness-value {
      font-size: 1.5em;
      margin-top: 15px;
    }
    .info {
      margin-top: 20px;
      font-size: 0.9em;
      opacity: 0.8;
    }
  </style>
</head>
<body>
  <h1>ESP32 无极调光器</h1>
  <div class="slider-container">
    <input type="range" id="brightness" min="0" max="255" value=")rawliteral" + String(brightness) + R"rawliteral(" onchange="sendBrightness(this.value)" oninput="updateDisplay(this.value)">
    <div class="brightness-value">亮度: <span id="display">)rawliteral" + String(brightness) + R"rawliteral(</span></div>
  </div>
  <div class="info">
    <p>拖动滑动条实时调节LED亮度</p>
    <p>范围: 0 (最暗) - 255 (最亮)</p>
  </div>

  <script>
    function updateDisplay(value) {
      document.getElementById('display').textContent = value;
    }

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

void handleRoot() {
  server.send(200, "text/html; charset=UTF-8", makePage());
}

void handleSetBrightness() {
  if (server.hasArg("value")) {
    brightness = server.arg("value").toInt();
    ledcWrite(0, brightness);
    Serial.printf("亮度设置为: %d\n", brightness);
    server.send(200, "text/plain", "OK: " + String(brightness));
  } else {
    server.send(400, "text/plain", "Missing value");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 配置LEDC通道 (GPIO 25)
  ledcSetup(0, PWM_FREQ, PWM_RES);
  ledcAttachPin(LED_PIN, 0);
  ledcWrite(0, brightness);

  // 启动AP模式
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("AP模式已启动");
  Serial.print("热点名称: ");
  Serial.println(ap_ssid);
  Serial.print("IP地址: http://");
  Serial.println(WiFi.softAPIP());

  // 设置路由
  server.on("/", handleRoot);
  server.on("/set", handleSetBrightness);
  server.begin();
}

void loop() {
  server.handleClient();
}
