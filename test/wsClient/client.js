const WebSocket = require('ws');

const serverAddress = 'ws://localhost:8080';

const ws = new WebSocket(serverAddress);

let pingInterval;

ws.on('open', function open() {
  console.log('✅ 连接成功！已连接到服务器:', serverAddress);

  const testMessage = '你好，服务器！我是 Node.js 客户端。';
  ws.send(testMessage);
  console.log(`[发送] -> ${testMessage}`);

  // --- 修改部分：使用 ws.ping() 发送心跳 ---
  pingInterval = setInterval(() => {
    if (ws.readyState === WebSocket.OPEN) {
      const pingMessage = `ping from client at ${new Date().toLocaleTimeString()}`;

      // 发送一个真正的 PING 帧
      // 你可以不带任何数据 ws.ping()，也可以像这样带上数据
      ws.ping(pingMessage);

      console.log(`[心跳] -> 发送 PING 帧: ${pingMessage}`);
    }
  }, 10000);
});

// --- 新增部分：监听服务器的 PONG 响应 ---
// PONG 响应不会触发 'message' 事件，而是触发 'pong' 事件
ws.on('pong', function pong(data) {
  const pongMessage = data.toString('utf8');
  console.log(`[心跳] <- 收到 PONG 帧: ${pongMessage}`);
});

ws.on('message', function incoming(data) {
  const message = data.toString('utf8');
  console.log(`[接收] <- ${message}`);
});

ws.on('close', function close(code, reason) {
  console.log(`❌ 连接已关闭。关闭代码: ${code}, 原因: ${reason.toString('utf8') || '无'}`);
  clearInterval(pingInterval);
});

ws.on('error', function error(err) {
  console.error('❗️ 发生错误:', err.message);
});

console.log(`🚀 正在尝试连接到 ${serverAddress}...`);