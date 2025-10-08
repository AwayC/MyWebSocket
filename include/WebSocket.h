//
// Created by AWAY on 25-10-7.
//

#pragma once

#include <WsServer.h>

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

class WsServer;


class WsSession : public std::enable_shared_from_this<WsSession>{
public:
    WsSession(const HttpServer::SessionPtr& session, WsServer* owner);
    ~WsSession();

    void close();
    void setBinaryType(const BinaryType type)
    {
        m_binaryType = type;
    }
    void send(std::string data);
    void connect();
    WsStatus getReadyState() const
    {
        return readyState;
    }

    void onConnect(const std::function<void(WsSession*)>& callback)
    {
        m_onConnectCb = callback;
    }
    void onClose(const std::function<void(WsSession*)>& callback)
    {
        m_onCloseCb = callback;
    }
    void onMessage(const std::function<void(WsSession*)>& callback)
    {
        m_onMessageCb = callback;
    }
    void onError(const std::function<void(WsSession*)>& callback)
    {
        m_onErrorCb = callback;
    }

    uv_stream_t* getClient()
    {
        return m_client;
    }


private:
    uv_loop_t* m_loop{};
    uv_stream_t* m_client;
    WsServer* m_owner;

    uv_buf_t m_recvBuf{};

    WsStatus readyState;

    //netWorking
    std::function<void(WsSession*)> m_onConnectCb;
    std::function<void(WsSession*)> m_onCloseCb;
    std::function<void(WsSession*)> m_onErrorCb;

    //messaging
    std::function<void(WsSession*)> m_onMessageCb;
    BinaryType m_binaryType;

    static void recv_alloc_cb(uv_handle_t* handle,
                            size_t suggested_size,
                            uv_buf_t* buf);

    static void recv_cb(uv_stream_t* stream,
                        ssize_t nread,
                        const uv_buf_t* buf);

    void handleMessage(int nread);
};