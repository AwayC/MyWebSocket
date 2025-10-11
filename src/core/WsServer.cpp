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

WsServer::~WsServer()
{
    std::lock_guard<std::mutex> lock(m_wsSessionsMtx);
    m_wsSessions.clear();
}



void WsServer::onUpgrade(const std::shared_ptr<HttpServer::Session> &session)
{
    auto ws = std::make_shared<WsSession>(session, this);
    pushWsSession(ws);
    if (m_onConnectCb)
    {
        m_onConnectCb(ws);
    }
    ws->connect();
}

void WsServer::pushWsSession(const WsSessionPtr &ws)
{
    std::lock_guard<std::mutex> lock(m_wsSessionsMtx);
    m_wsSessions.push_back(ws);
}

void WsServer::removeWsSession(const WsSessionPtr &ws)
{
    std::lock_guard<std::mutex> lock(m_wsSessionsMtx);
    auto it = std::find(m_wsSessions.begin(), m_wsSessions.end(), ws);
    if (it != m_wsSessions.end())
    {
        std::swap(*it, m_wsSessions.back());
        m_wsSessions.pop_back();
    }
}
