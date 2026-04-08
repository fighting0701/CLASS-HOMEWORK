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
 *   原理: JavaScript定时调用fetch()请求/data接口
 * - 数据下发: /data 接口返回纯文本格式的传感器数值
 * - 实时显示: JavaScript更新DOM元素显示最新数值
 * - CORS跨域: 同源策略下直接请求ESP32的IP即可
 *
 * 工作流程:
 * [ESP32启动AP] --> [用户连接热点] --> [打开网页]
 *                                              |
 * [触摸传感器] --> [touchRead(T6)] --> [/data接口]
 *                                              |
 *     <-- [AJAX轮询100ms] <-- [fetch('/data')]
 *                                              |
 *     <-- [JavaScript解析] <-- [返回数值]
 *                                              |
 *     <-- [DOM更新] <-- [显示大数字+进度条+状态]
 *
 * 硬件连接:
 * - 触摸引脚 -> GPIO 14 (T6)
 * - 无需额外接线,用手触摸GPIO14即可触发
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ==================== WiFi AP模式配置 ====================
// ESP32作为无线接入点,创建WiFi热点
const char* ap_ssid = "ESP32_Sensor";      // 热点名称
const char* ap_password = "12345678";       // 热点密码(至少8位)

// ==================== 硬件引脚定义 ====================
#define TOUCH_PIN T6            // 触摸引脚 (GPIO 14, T6)
#define TOUCH_THRESHOLD 40      // 触摸阈值(参考值:低于此值表示被触摸)

// ==================== Web服务器 ====================
// WebServer对象监听80端口(HTTP标准端口)
WebServer server(80);

/**
 * 生成仪表盘网页HTML
 * 功能: 实时显示触摸传感器数值的仪表盘界面
 * 包含: 大数字显示 + 状态文字 + 进度条 + AJAX轮询逻辑
 *
 * 返回值: 包含完整HTML的String对象
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
    /* 页面整体样式: 深色背景,居中布局 */
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

    /* 仪表盘容器: 半透明玻璃效果 */
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

    /* 数值显示: 超大字体,便于远距离观看 */
    .sensor-value {
      font-size: 8em;           /* 超大字体 */
      font-weight: bold;        /* 粗体 */
      color: #00ff88;            /* 绿色发光效果 */
      text-shadow: 0 0 30px rgba(0,255,136,0.5);
      font-family: 'Courier New', monospace; /* 等宽字体,数字更整齐 */
      line-height: 1;           /* 紧凑行高 */
    }

    /* 状态文字: 圆角标签样式 */
    .status {
      font-size: 1.5em;
      margin-top: 20px;
      padding: 10px 20px;
      border-radius: 25px;      /* 圆角胶囊形状 */
      display: inline-block;
    }
    /* 待机状态: 绿色 */
    .status-normal { background: rgba(0,255,136,0.2); color: #00ff88; }
    /* 触摸状态: 红色警告 */
    .status-touched { background: rgba(255,100,100,0.2); color: #ff6464; }

    /* 底部信息 */
    .info {
      margin-top: 40px;
      font-size: 0.9em;
      opacity: 0.5;
    }

    /* 进度条容器 */
    .bar-container {
      width: 300px;
      height: 20px;
      background: rgba(255,255,255,0.1);
      border-radius: 10px;
      margin: 20px auto 0;
      overflow: hidden;         /* 隐藏溢出部分 */
    }
    /* 进度条填充: 渐变+动画 */
    .bar-fill {
      height: 100%;
      background: linear-gradient(90deg, #00ff88, #00ccff);
      border-radius: 10px;
      transition: width 0.1s ease; /* 平滑过渡 */
    }
  </style>
</head>
<body>
  <h1>实时传感器仪表盘</h1>
  <p class="subtitle">触摸传感器实时监控</p>

  <div class="gauge-container">
    <div class="sensor-label">Touch Sensor (GPIO14)</div>
    <!-- 数值显示区域: 初始值"--"表示加载中 -->
    <div class="sensor-value" id="value">--</div>
    <!-- 状态标签: 待机/已触摸 -->
    <div id="status" class="status status-normal">待机中</div>
    <!-- 进度条: 可视化数值大小 -->
    <div class="bar-container">
      <div class="bar-fill" id="bar" style="width: 50%;"></div>
    </div>
  </div>

  <div class="info">
    <p>提示: 用手触摸GPIO14引脚,数值会变小</p>
  </div>

  <script>
    /**
     * AJAX轮询函数 fetchData()
     * 功能: 从ESP32获取传感器数据并更新页面显示
     *
     * 工作原理:
     * 1. fetch()发送GET请求到/data接口
     * 2. 服务器返回纯文本数值
     * 3. JavaScript解析并更新DOM元素
     *
     * 注意: 不使用XMLHttpRequest,因为fetch()更现代简洁
     */
    function fetchData() {
      fetch('/data')
        .then(response => response.text())  // 解析为文本
        .then(data => {
          // ========== 更新数值显示 ==========
          document.getElementById('value').textContent = data;

          // ========== 根据数值更新状态 ==========
          const value = parseInt(data);  // 字符串转整数
          const statusEl = document.getElementById('status');
          const barEl = document.getElementById('bar');

          // 触摸传感器特性: 触摸时数值变小
          // 正常值约40-60, 触摸时低于40
          if (value < 40) {
            // 被触摸状态: 红色警告
            statusEl.textContent = '已触摸';
            statusEl.className = 'status status-touched';
            document.getElementById('value').style.color = '#ff6464';
            document.getElementById('value').style.textShadow = '0 0 30px rgba(255,100,100,0.5)';
          } else {
            // 待机状态: 绿色
            statusEl.textContent = '待机中';
            statusEl.className = 'status status-normal';
            document.getElementById('value').style.color = '#00ff88';
            document.getElementById('value').style.textShadow = '0 0 30px rgba(0,255,136,0.5)';
          }

          // ========== 更新进度条 ==========
          // 将数值(约0-100)映射为百分比
          // 注意: 数值越小进度条越短(反比关系)
          const percentage = Math.min(100, Math.max(0, (value / 100) * 100));
          barEl.style.width = percentage + '%';
        })
        .catch(error => {
          // 网络错误时显示Error
          document.getElementById('value').textContent = 'Error';
        });
    }

    // ========== 定时器: 每100ms刷新一次 ==========
    // 100ms = 10次/秒,足够平滑又不至于太频繁
    setInterval(fetchData, 100);
    // 首次立即加载(不等待100ms)
    fetchData();
  </script>
</body>
</html>
)rawliteral";
  return html;
}

/**
 * 处理根路径("/")请求
 * 返回完整仪表盘网页HTML
 */
void handleRoot() {
  server.send(200, "text/html; charset=UTF-8", makePage());
}

/**
 * 处理数据请求("/data")
 * 返回触摸传感器的当前数值(纯文本格式)
 *
 * AJAX轮询专用接口:
 * - 返回纯文本而非JSON,便于JavaScript直接使用
 * - 无需cookies或session
 * - 每次请求独立处理
 */
void handleData() {
  // 读取触摸传感器数值
  // 返回值范围: 约0-255(未触摸时高,触摸时低)
  int touchValue = touchRead(TOUCH_PIN);

  // 发送纯文本响应
  // Content-Type: text/plain 便于客户端解析
  server.send(200, "text/plain", String(touchValue));

  // 调试输出到串口(可选)
  // Serial.printf("Touch: %d\n", touchValue);
}

/**
 * 初始化函数
 * 配置WiFi AP模式,注册Web路由
 */
void setup() {
  Serial.begin(115200);
  delay(1000);

  // 启动AP模式(ESP32创建WiFi热点)
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("AP模式已启动");
  Serial.print("热点名称: ");
  Serial.println(ap_ssid);
  Serial.print("IP地址: http://");
  Serial.println(WiFi.softAPIP());

  // 注册HTTP路由
  server.on("/", handleRoot);        // 网页首页(仪表盘)
  server.on("/data", handleData);    // 数据接口(AJAX轮询)
  server.begin();

  Serial.println("Web服务器已启动");
  Serial.println("请连接热点并访问 http://192.168.4.1");
}

/**
 * 主循环函数
 * 处理Web客户端请求
 * 注意: 无需处理传感器数据采集,因为AJAX是请求-响应模式
 *       每次网页请求时才会读取传感器数值
 */
void loop() {
  // 处理Web客户端请求(关键!)
  // 必须调用此函数才能响应AJAX请求
  server.handleClient();
}
