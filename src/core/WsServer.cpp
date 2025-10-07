//
// Created by AWAY on 25-10-7.
//

#include "WsServer.h"

#include <utility>

WsServer::WsServer(const std::shared_ptr<HttpServer> &httpServer)
{
    m_httpSvr = httpServer;
    m_loop = m_httpSvr->getLoop();

    m_httpSvr->onUpgrade([this](std::shared_ptr<HttpServer::Session> session) {
        this->onUpgrade(session);
    });
}

void WsServer::onUpgrade(const std::shared_ptr<HttpServer::Session> &session)
{
    auto ws = std::make_shared<WebSocket>(session);
    addWebSocket(ws);
    pushWsSession(ws);
}

void WsServer::addWebSocket(const WebSocketPtr& wsSession)
{

}

void WsServer::pushWsSession(const WebSocketPtr &wsSession)
{
    std::lock_guard<std::mutex> lock(m_wsSessionsMtx);
    m_wsSessions.push_back(wsSession);
}

void WsServer::removeWsSession(const WebSocketPtr &wsSession)
{
    std::lock_guard<std::mutex> lock(m_wsSessionsMtx);
    auto it = std::find(m_wsSessions.begin(), m_wsSessions.end(), wsSession);
    if (it != m_wsSessions.end())
    {
        std::swap(*it, m_wsSessions.back());
        m_wsSessions.pop_back();
    }
}
