# MyWebSocket

![Build Status](https://img.shields.io/badge/build-passing-brightgreen) ![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Mac-blue) ![License](https://img.shields.io/badge/license-MIT-green) ![C++ Standard](https://img.shields.io/badge/C%2B%2B-17%2F20-orange)

**MyWebSocket** 是用于学习的基于`libuv`和 C++17 的高性能 WebSocke 和 HTTP 服务器框架，集成路由功能，JWT验证，JSON 解析等功能。



## ✨ 核心特性（Features)

- 🚀 **高性能架构**：
  - 现代 C++17 编写
  - 基于`libuv`异步网络`I/O` 库，单线程事件循环，避免多线程开销。
- 🛠 **现代化设计**：参考`node.js`的网络 api 设计，接口更加现代和方便。
- ✉️ **HTTP 服务器**：
  - 支持正则表达式路由参数解析。
  - 支持 RESTful 风格路由。

- ✉️ **WebSocket 支持**：提供 WebSocket 服务器实现
- 🧵 **内置工具**：集成加密运算库 `OpenSSL` 和 `leptjson`[JSON库](https://github.com/AwayC/json_parse)



## 📦 环境要求 (Requirements)

* **Compiler**: C++20 编译器（GCC, Clang)
* **Operate System**: Mac, linux
* **Build System**: CMake 3.10+
* **Dependencies**: 
  * OpenSSL(CMake 中导入)，用于加密运算
  * lept_json 用于 JSON 解析
  * JWT-cpp 用于 JWT 验证
  * libuv 网络I/O，底层框架




## 🔨 构建与安装 （Build and Download)

### 1. 克隆仓库
```bash
git clone git@github.com:AwayC/MyWebSocket.git
cd MyWebsocket
```

### 2. 构建

```bash
cmake -B build
cmake --build build
```

### 3. CMake快速加入到项目

```cmake
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third-party/MyWebSocket)

add_executable(main src/main.cpp)
target_link_libraries(main
	MyWebSocket
)
```



## 📖 使用示例 (Usage)

### 1. 快速创建 HTTP 服务器

```c++
#include "HttpServer.h"
#include <iostream>

int main() { 
  // 创建 HTTP 服务器实例，监听 127.0.0.1:8080
	auto httpSvr = HttpServer::create("127.0.0.1", 8080); 
  
  // 连接
  httpSvr->onConnect([](uv_tcp_t* client) { 
  	std::cout << "on connect" << std::endl; 
  }); 
  
  // 定义 GET 请求路由, 正则匹配
  httpSvr->get("/id/:id([0-9]+)", [](httpReq* req, httpRespPtr ) { 
    // 获取参数
  	std::cout << "get id: " << req->param["id"] << std::endl; 
    
    // 发送响应 (只能发送一次)
    resp->sendStr("getted id"); 
  }); 
  
  // 启动服务器
  httpSvr->start(); 
  return 0; 
}
```

### 2. 创建 WebSocket 服务器

``` c++
#include "WsSocket.h"
#include "WsServer"

int main() { 
	auto httpSvr = HttpServer::create("127.0.0.1", 8080); 
  WsServer svr(httpSvr); 
  
  // 连接到WebSocket
  svr.onConnect([](WsSessionPtr session){ 
  	std::cout << "WebSocket connected" << std::endl; 
    
    // 接受到信息
    Session->onMessage([](WsSessionPtr ws){ 
    	std::string_view msg = ws->getStrMessage(); 
      std::cout << msg << std::endl; 
      //发送信息
      ws->send(std::string(msg)); 
    }); 
  }); 
  
  httpSvr->start(); 
  return 0;
}
```

### 3.  响应与请求

`httpReq` : 

```c++
.version;
.url;
.body;
.method;
.param; // 路由参数
.query(); // 请求参数
```

`httpResp`:

```c++
.sendStr(std::sting msg); 	// 发送字符串
.sendJson(lept_value json); 	// 发送json
.sendFile(std::string path);	//发送文件
```

`WsSession`: 

```c++
.getStrMessage();
.getJsonMessage(); 
.send(lept_value json); 
.send(std::string msg); 
.sendFile(std::string path); 
```

### 4. 更多

​	持续更新... 🥳



