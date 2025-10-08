//
// Created by AWAY on 25-10-7.
//

#pragma once

#include "httpserver.h"
#include "WebSocket.h"
#include "uv.h"


using WsSessionPtr = std::shared_ptr<WsSession>;

class WsServer {
public:

    explicit WsServer(const std::shared_ptr<HttpServer> &httpServer);
    ~WsServer();


    size_t getWsCount() const
    {
        std::lock_guard<std::mutex> lock(m_wsSessionsMtx);
        return m_wsSessions.size();
    }

    void pushWsSession(const WsSessionPtr &ws);
    void removeWsSession(const WsSessionPtr &ws);

    void onConnect(const std::function<void(WsSessionPtr)>& callback)
    {
        m_onConnectCb = callback;
    }

private:
    uv_loop_t* m_loop;
    std::shared_ptr<HttpServer> m_httpSvr;

    mutable std::mutex m_wsSessionsMtx;
    std::vector<WsSessionPtr> m_wsSessions;

    std::function<void(WsSessionPtr)> m_onConnectCb;

    void onUpgrade(const std::shared_ptr<HttpServer::Session> &session);

};