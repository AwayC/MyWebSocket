//
// Created by AWAY on 25-10-7.
//

#pragma once

#include "httpserver.h"
#include "WebSocket.h"
#include "uv.h"

using WebSocketPtr = std::shared_ptr<WebSocket>;

class WsServer {
public:

    explicit WsServer(const std::shared_ptr<HttpServer> &httpServer);
    ~WsServer();


    size_t getWsCount() const
    {
        std::lock_guard<std::mutex> lock(m_wsSessionsMtx);
        return m_wsSessions.size();
    }
private:
    uv_loop_t* m_loop;
    std::shared_ptr<HttpServer> m_httpSvr;

    mutable std::mutex m_wsSessionsMtx;
    std::vector<std::shared_ptr<WebSocket>> m_wsSessions;

    std::function<void(WebSocketPtr)> m_onWsOpenCb;

    void onUpgrade(const std::shared_ptr<HttpServer::Session> &session);

    void addWebSocket(const WebSocketPtr& wsSession);
    void closeWebSocket(const WebSocketPtr& wsSession);

    void pushWsSession(const WebSocketPtr &wsSession);
    void removeWsSession(const WebSocketPtr &wsSession);
};