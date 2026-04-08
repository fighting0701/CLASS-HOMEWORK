/*
 * ESP32 实时传感器Web仪表盘 - 实验9
 * AJAX异步刷新 + 触摸传感器实时数据监控
 *
 * 功能说明:
 * 1. ESP32工作在AP模式,创建WiFi热点供手机/电脑连接
 * 2. 网页端使用AJAX技术定期从ESP32获取触摸传感器数值
 * 3. 实时显示传感器数值,类似仪器仪表的数据监控面板
 * 4. 手靠近引脚时数值变小,离开时恢复
 *
 * 技术要点:
 * - AJAX轮询: 网页定期发送请求获取最新数据(无需刷新页面)
 * - 数据下发: /data 接口返回纯文本格式的传感器数值
 * - 实时显示: JavaScript更新DOM元素显示最新数值
 *
 * 硬件连接:
 * - 触摸引脚 -> GPIO 14 (T6)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ==================== WiFi AP模式配置 ====================
const char* ap_ssid = "ESP32_Sensor";      // 热点名称
const char* ap_password = "12345678";       // 热点密码

// ==================== 硬件引脚定义 ====================
#define TOUCH_PIN T6            // 触摸引脚 (GPIO 14)
#define TOUCH_THRESHOLD 40      // 触摸阈值(参考值)

// ==================== Web服务器 ====================
WebServer server(80);

/**
 * 生成仪表盘网页HTML
 * 包含: 大数字显示 + 状态文字 + AJAX轮询逻辑
 */
String makePage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>实时传感器仪表盘</title>
  <style>
    /* 页面整体样式 */
    body {
      font-family: 'Segoe UI', Arial, sans-serif;
      text-align: center;
      margin: 0;
      padding: 0;
      background: linear-gradient(135deg, #0f0f1a 0%, #1a1a2e 50%, #16213e 100%);
      color: white;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      justify-content: center;
    }
    h1 { font-size: 2em; margin-bottom: 10px; opacity: 0.9; }
    .subtitle { font-size: 1em; opacity: 0.6; margin-bottom: 40px; }

    /* 仪表盘容器 */
    .gauge-container {
      background: rgba(255,255,255,0.05);
      border-radius: 20px;
      padding: 40px 60px;
      display: inline-block;
      border: 2px solid rgba(255,255,255,0.1);
      box-shadow: 0 10px 40px rgba(0,0,0,0.3);
    }

    /* 传感器标签 */
    .sensor-label {
      font-size: 1.2em;
      opacity: 0.7;
      margin-bottom: 10px;
    }

    /* 数值显示(大字体) */
    .sensor-value {
      font-size: 8em;
      font-weight: bold;
      color: #00ff88;
      text-shadow: 0 0 30px rgba(0,255,136,0.5);
      font-family: 'Courier New', monospace;
      line-height: 1;
    }

    /* 状态文字 */
    .status {
      font-size: 1.5em;
      margin-top: 20px;
      padding: 10px 20px;
      border-radius: 25px;
      display: inline-block;
    }
    .status-normal { background: rgba(0,255,136,0.2); color: #00ff88; }
    .status-touched { background: rgba(255,100,100,0.2); color: #ff6464; }

    /* 底部信息 */
    .info {
      margin-top: 40px;
      font-size: 0.9em;
      opacity: 0.5;
    }

    /* 进度条样式 */
    .bar-container {
      width: 300px;
      height: 20px;
      background: rgba(255,255,255,0.1);
      border-radius: 10px;
      margin: 20px auto 0;
      overflow: hidden;
    }
    .bar-fill {
      height: 100%;
      background: linear-gradient(90deg, #00ff88, #00ccff);
      border-radius: 10px;
      transition: width 0.1s ease;
    }
  </style>
</head>
<body>
  <h1>实时传感器仪表盘</h1>
  <p class="subtitle">触摸传感器实时监控</p>

  <div class="gauge-container">
    <div class="sensor-label">Touch Sensor (GPIO14)</div>
    <div class="sensor-value" id="value">--</div>
    <div id="status" class="status status-normal">待机中</div>
    <div class="bar-container">
      <div class="bar-fill" id="bar" style="width: 50%;"></div>
    </div>
  </div>

  <div class="info">
    <p>提示: 用手触摸GPIO14引脚,数值会变小</p>
  </div>

  <script>
    /**
     * AJAX轮询函数
     * 定期从ESP32获取传感器数值并更新页面显示
     */
    function fetchData() {
      fetch('/data')
        .then(response => response.text())
        .then(data => {
          // 更新数值显示
          document.getElementById('value').textContent = data;

          // 根据数值更新状态和样式
          const value = parseInt(data);
          const statusEl = document.getElementById('status');
          const barEl = document.getElementById('bar');

          // 数值越小表示越接近触摸阈值(被触摸)
          // 正常值约40-60, 触摸时低于40
          if (value < 40) {
            statusEl.textContent = '已触摸';
            statusEl.className = 'status status-touched';
            document.getElementById('value').style.color = '#ff6464';
            document.getElementById('value').style.textShadow = '0 0 30px rgba(255,100,100,0.5)';
          } else {
            statusEl.textContent = '待机中';
            statusEl.className = 'status status-normal';
            document.getElementById('value').style.color = '#00ff88';
            document.getElementById('value').style.textShadow = '0 0 30px rgba(0,255,136,0.5)';
          }

          // 更新进度条 (将0-100映射到0%-100%)
          // 数值越小进度条越短
          const percentage = Math.min(100, Math.max(0, (value / 100) * 100));
          barEl.style.width = percentage + '%';
        })
        .catch(error => {
          document.getElementById('value').textContent = 'Error';
        });
    }

    // 启动定时器,每100ms刷新一次数据
    setInterval(fetchData, 100);
    // 首次立即加载
    fetchData();
  </script>
</body>
</html>
)rawliteral";
  return html;
}

/**
 * 处理根路径("/")请求
 * 返回仪表盘网页HTML
 */
void handleRoot() {
  server.send(200, "text/html; charset=UTF-8", makePage());
}

/**
 * 处理数据请求("/data")
 * 返回触摸传感器的当前数值(纯文本)
 * AJAX轮询专用接口,返回格式简单便于解析
 */
void handleData() {
  int touchValue = touchRead(TOUCH_PIN);
  // 返回纯文本数字,便于JavaScript直接使用
  server.send(200, "text/plain", String(touchValue));
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 启动AP模式
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("AP模式已启动");
  Serial.print("热点名称: ");
  Serial.println(ap_ssid);
  Serial.print("IP地址: http://");
  Serial.println(WiFi.softAPIP());

  // 注册路由
  server.on("/", handleRoot);     // 网页首页
  server.on("/data", handleData);  // 数据接口(AJAX轮询)
  server.begin();
}

void loop() {
  // 处理Web客户端请求
  server.handleClient();
}
