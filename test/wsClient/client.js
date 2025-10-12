const WebSocket = require('ws');


const serverAddress = 'ws://localhost:8080';


const ws = new WebSocket(serverAddress);

let pingInterval;


ws.on('open', function open() {
  console.log('✅ 连接成功！已连接到服务器:', serverAddress);

  const testMessage = '你好，服务器！我是 Node.js 客户端。';
  ws.send(testMessage);
  console.log(`[发送] -> ${testMessage}`);

  pingInterval = setInterval(() => {
    if (ws.readyState === WebSocket.OPEN) {
      const pingMessage = `ping from client at ${new Date().toLocaleTimeString()}`;
      ws.send(pingMessage);
      console.log(`[心跳] -> ${pingMessage}`);
    }
  }, 10000); 
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