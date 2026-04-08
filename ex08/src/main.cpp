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
 * 工作流程:
 * [撤防状态] --点击布防--> [布防状态] --触摸触发--> [报警状态]
 * [报警状态] --点击撤防--> [撤防状态] (LED熄灭,系统重置)
 *
 * 硬件连接:
 * - LED正极 -> GPIO 25 (通过220Ω电阻)
 * - LED负极 -> GND
 * - 触摸引脚 -> GPIO 14 (T6)
 *
 * 使用说明:
 * 1. 上传代码到ESP32
 * 2. 手机/电脑连接WiFi热点: ESP32_Security (密码: 12345678)
 * 3. 浏览器打开 http://192.168.4.1
 * 4. 点击"布防"按钮进入警戒状态
 * 5. 用手触摸GPIO14引脚(或杜邦线),LED开始报警闪烁
 * 6. 点击"撤防"按钮解除报警
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ==================== WiFi AP模式配置 ====================
// ESP32作为无线接入点,创建WiFi热点
const char* ap_ssid = "ESP32_Security";    // 热点名称
const char* ap_password = "12345678";       // 热点密码(至少8位)

// ==================== 硬件引脚定义 ====================
#define LED_PIN 25              // LED报警灯引脚 (GPIO 25)
#define TOUCH_PIN T6            // 触摸引脚 (GPIO 14, T6)
#define TOUCH_THRESHOLD 40      // 触摸阈值: 低于此值表示被触摸

// ==================== 系统状态定义 ====================
// 使用宏定义三种系统状态,便于状态机管理
#define STATE_DISARMED 0        // 撤防状态: LED熄灭,触摸无效
#define STATE_ARMED 1           // 布防状态: LED熄灭,等待触发
#define STATE_ALARM 2           // 报警状态: LED高频闪烁,锁定报警

// ==================== 全局状态变量 ====================
int systemState = STATE_DISARMED;  // 当前系统状态(全局变量)
bool ledState = false;             // LED当前状态(用于翻转控制)

// ==================== Web服务器 ====================
// WebServer对象监听80端口(HTTP标准端口)
WebServer server(80);

// ==================== 状态标识 ====================
// 状态名称数组,用于网页显示
const char* stateNames[] = {"撤防", "布防", "报警"};

/**
 * 生成网页HTML
 * 功能: 显示当前系统状态 + 动态显示布防/撤防按钮
 * 说明: 报警状态时只能显示撤防按钮,防止误操作
 */
String makePage() {
  String armBtn = "";    // 布防按钮HTML
  String disarmBtn = ""; // 撤防按钮HTML

  // 根据当前状态动态生成按钮
  // 撤防状态: 只显示布防按钮
  if (systemState == STATE_DISARMED) {
    armBtn = "<a href=\"/arm\"><button class=\"btn arm-btn\">布防(Arm)</button></a>";
    disarmBtn = "";
  }
  // 布防状态: 只显示撤防按钮
  else if (systemState == STATE_ARMED) {
    armBtn = "";
    disarmBtn = "<a href=\"/disarm\"><button class=\"btn disarm-btn\">撤防(Disarm)</button></a>";
  }
  // 报警状态: 只显示撤防按钮(锁定状态,必须撤防)
  else if (systemState == STATE_ALARM) {
    armBtn = "";
    disarmBtn = "<a href=\"/disarm\"><button class=\"btn disarm-btn\">撤防(Disarm)</button></a>";
  }

  // 构建完整HTML页面
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>物联网安防报警器</title>
  <style>
    /* 页面整体样式: 深色背景,居中布局 */
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin-top: 50px;
      background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
      color: white;
    }
    h1 { font-size: 2.5em; margin-bottom: 20px; }

    /* 状态显示容器 */
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

    /* 不同状态的文字颜色 */
    .status-armed { color: #00ff00; }      /* 布防: 绿色 */
    .status-disarmed { color: #888; }      /* 撤防: 灰色 */
    .status-alarm { color: #ff0000; animation: blink 0.3s infinite; } /* 报警: 红色闪烁 */

    /* CSS动画: 报警闪烁效果 */
    @keyframes blink { 0%,100%{opacity:1;} 50%{opacity:0.3;} }

    /* 按钮样式 */
    .btn {
      padding: 15px 40px;
      font-size: 1.2em;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      margin: 10px;
    }
    .arm-btn { background: #00aa00; color: white; }    /* 布防按钮: 绿色 */
    .disarm-btn { background: #ff4444; color: white; } /* 撤防按钮: 红色 */

    .info { margin-top: 30px; font-size: 0.9em; opacity: 0.7; }
  </style>
</head>
<body>
  <h1>物联网安防报警器</h1>
  <div class="status-container">
    <div class="status-label">当前状态</div>
    <!-- 状态显示: 根据systemState显示对应文字 -->
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
    /* 自动刷新: 每秒刷新页面以同步服务器状态 */
    setTimeout(function(){ location.reload(); }, 1000);
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
 * 处理布防请求("/arm")
 * 将系统状态切换为STATE_ARMED(布防状态)
 * 使用303重定向回到首页
 */
void handleArm() {
  systemState = STATE_ARMED;
  Serial.println("系统布防");
  // 303状态码: 重定向到首页
  server.sendHeader("Location", "/");
  server.send(303);
}

/**
 * 处理撤防请求("/disarm")
 * 将系统状态切换为STATE_DISARMED(撤防状态)
 * 同时关闭LED,重置ledState
 */
void handleDisarm() {
  systemState = STATE_DISARMED;
  digitalWrite(LED_PIN, LOW);  // 确保LED关闭
  ledState = false;
  Serial.println("系统撤防");
  server.sendHeader("Location", "/");
  server.send(303);
}

/**
 * 初始化函数
 * 配置GPIO,启动AP模式,注册Web路由
 */
void setup() {
  Serial.begin(115200);
  delay(1000);

  // 初始化LED引脚为输出模式,初始为低电平(熄灭)
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 启动AP模式(ESP32创建WiFi热点)
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("AP模式已启动");
  Serial.print("热点名称: ");
  Serial.println(ap_ssid);
  Serial.print("IP地址: http://");
  Serial.println(WiFi.softAPIP());

  // 注册HTTP路由
  server.on("/", handleRoot);         // 网页首页
  server.on("/arm", handleArm);       // 布防接口
  server.on("/disarm", handleDisarm); // 撤防接口
  server.begin();
}

/**
 * 主循环函数
 * 处理Web请求 + 检测触摸输入 + 控制报警闪烁
 */
void loop() {
  // 处理Web客户端请求
  server.handleClient();

  // 读取触摸引脚的电容值
  // 正常值约40-60, 触摸时值会降低(低于阈值)
  int touchValue = touchRead(TOUCH_PIN);

  // ========== 状态机逻辑 ==========

  // 布防状态: 检测触摸触发
  if (systemState == STATE_ARMED) {
    if (touchValue < TOUCH_THRESHOLD) {
      // 触摸触发,进入报警状态
      systemState = STATE_ALARM;
      Serial.println("报警触发!");
    }
  }

  // 报警状态: LED高频闪烁(100ms间隔)
  // 注意: 报警状态会持续循环,直到撤防
  if (systemState == STATE_ALARM) {
    ledState = !ledState;  // LED状态翻转
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    delay(100);  // 100ms = 10Hz闪烁频率
  }
}
