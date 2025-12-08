//
// Created by AWAY on 25-10-7.
//

#pragma once

#include "httpserver.h"
#include "../src/core/websocket_parser.h"
#include "leptjson.h"
#include "../src/util/jwt.h"

#define WSSESSION_ALLOC_SIZE 32
enum class WsStatus {
    CONNECTING,
    OPEN,
    CLOSING,
    CLOSED,
};

class WsServer;
class WsSession;

using WsSessionPtr = std::shared_ptr<WsSession>;


class WsSession : public std::enable_shared_from_this<WsSession> {
public:
    WsSession(const HttpServer::SessionPtr& session, WsServer* owner);
    ~WsSession();

    void close();

    void send(const lept_value& json);
    void send(const std::string& str);
    void send(std::string&& str);
    void send(const char* str);
    void sendFile(const std::string& path);

    void connect();
    WsStatus getReadyState() const
    {
        return readyState;
    }

    void onConnect(const std::function<void(WsSessionPtr)>& callback)
    {
        m_onConnectCb = callback;
    }
    void onClose(const std::function<void(WsSessionPtr)>& callback)
    {
        m_onCloseCb = callback;
    }
    void onMessage(const std::function<void(WsSessionPtr)>& callback)
    {
        m_onMessageCb = callback;
    }
    void onError(const std::function<void(WsSessionPtr, const std::exception err)>& callback)
    {
        m_onErrorCb = callback;
    }

    uv_stream_t* getClient() const
    {
        return m_client;
    }

    std::string_view getStrMessage() const
    {
        return {m_frame.payload.data(), m_frame.payload.size()};
    }

    lept_value getJsonMessage();


private:
    uv_loop_t* m_loop{};
    uv_stream_t* m_client;
    WsServer* m_owner;

    uv_buf_t m_recvBuf;

    WsStatus readyState;

    websocket_parser m_parser;
    WsFrame m_frame;

    struct WriteCtx
    {
        std::string m_str;
        std::vector<char> m_head;
        void setMsg(const std::string& str)
        {
            m_str = str;
        }
        void setMsg(std::string&& str)
        {
            m_str = std::move(str);
            isFile = false;
        }
        void setMsg(const char* str)
        {
            m_str = std::string(str);
            isFile = false;
        }
        void setMsg(FileReader* reader)
        {
            m_reader = reader;
            isFile = true;
        }
        void clearMsg()
        {
            m_str.clear();
        }
        bool isFile = false;
        FileReader* m_reader;
        uv_write_t req;
        std::vector<uv_buf_t> m_buffers;
        WsSessionPtr m_owner;
    };

    //netWorking
    std::function<void(WsSessionPtr)> m_onConnectCb;
    std::function<void(WsSessionPtr)> m_onCloseCb;
    std::function<void(WsSessionPtr, const std::exception)> m_onErrorCb;

    //messaging
    std::function<void(WsSessionPtr)> m_onMessageCb;

    static void recv_alloc_cb(uv_handle_t* handle,
                            size_t suggested_size,
                            uv_buf_t* buf);

    static void recv_cb(uv_stream_t* stream,
                        ssize_t nread,
                        const uv_buf_t* buf);

    void handleMessage(size_t nread);
    void handleWsFrame();

    void sendPong();

    void inter_send(WriteCtx* ctx, uint8_t opcode);
    void inter_sendFile(FileReader* reader);
};