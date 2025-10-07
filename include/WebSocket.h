//
// Created by AWAY on 25-10-7.
//

#pragma once

#include "httpserver.h"

enum class WsStatus {
    CONNECTING,
    OPEN,
    CLOSING,
    CLOSED,
};

enum class BinaryType {
    blob,
    arraybuffer,
};

class WebSocket {
public:
    WebSocket(HttpServer::SessionPtr session);
    ~WebSocket();

    void close(int code, std::string reason);
    void send(std::string data);
    void init();
    uv_stream_t* getClient()
    {
        return m_client;
    }


private:
    uv_loop_t* m_loop;
    uv_stream_t* m_client;

    std::string m_url;
    std::string m_host;
    std::string m_port;
    std::string m_scheme; //ws or wss

    WsStatus readyState;

    //netWorking
    std::function<void(WebSocket*)> m_onOpenCb;
    std::function<void(WebSocket*)> m_onCloseCb;
    std::function<void(WebSocket*)> m_onErrorCb;

    //messaging
    std::function<void(WebSocket*)> m_onMessageCb;
    BinaryType m_binaryType;


};