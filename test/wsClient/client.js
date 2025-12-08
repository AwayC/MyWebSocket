const WebSocket = require('ws');

// 目标地址
const WS_URL = 'ws://localhost:8080/ws';

console.log(`🚀 准备连接到: ${WS_URL}`);

const ws = new WebSocket(WS_URL, {
  perMessageDeflate: false // 再次强调：必须禁用压缩，否则测不出解析器 BUG
});

ws.on('open', function open() {
  console.log('✅ WebSocket 连接成功！开始发送长 Token 测试...');

  // --- 场景 1: 中等长度 Token (模拟普通 JWT) ---
  // 长度: ~500 字节
  // 预期: 触发服务器 16位 长度解析逻辑 (Payload = 126)
  const mediumToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9." + "A".repeat(400) + ".Signature";
  const msg1 = JSON.stringify(['login_req', mediumToken]);

  console.log(`\n[发送] 场景 1: 中长消息 (${Buffer.byteLength(msg1)} bytes)`);
  console.log(`       用于测试 16位 长度解析 (之前左移 1 位那个 BUG)`);
  ws.send(msg1);

  // --- 场景 2: 超长 Token (压力测试) ---
  // 长度: ~70KB (65536 +)
  // 预期: 触发服务器 64位 长度解析逻辑 (Payload = 127)
  // 注意: 这取决于你服务器的 buffer 是否够大，不够大可能会断开，但解析逻辑必须是对的
  const hugeToken = "BIG_TOKEN_PREFIX_" + "B".repeat(70000);
  console.log(`\n[发送] 场景 2: 超长消息 (${hugeToken.length} bytes)`);
  console.log(`       用于测试 64位 长度解析`);
  ws.send(hugeToken);
});

ws.on('message', function incoming(data) {
  const msg = data.toString('utf8');
  const len = Buffer.byteLength(data);

  console.log(`\n<< [接收] 服务器回包，长度: ${len}`);

  // 验证回包内容摘要
  if (len < 100) {
    console.log(`   内容: "${msg}"`);
  } else {
    console.log(`   内容(前50字符): "${msg.substring(0, 50)}..."`);
    console.log(`   内容(后50字符): "...${msg.substring(len - 50)}"` );
  }

  // 验证逻辑
  if (msg.includes("A".repeat(20))) {
    console.log("   ✅ 成功：服务器正确解析了中长消息！(16位长度逻辑通过)");
  }
  if (msg.includes("B".repeat(20))) {
    console.log("   ✅ 成功：服务器正确解析了超长消息！(64位长度逻辑通过)");
  }
});

ws.on('close', function close(code, reason) {
  console.log(`\n❌ 连接断开。代码: ${code}`);
  process.exit(0);
});

ws.on('error', function error(err) {
  console.error(`❗️ 发生错误: ${err.message}`);
});