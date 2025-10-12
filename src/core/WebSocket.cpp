//
// Created by AWAY on 25-10-7.
//

#include "WebSocket.h"
#include "WsServer.h"
#include <cassert>
#include <WsServer.h>

WsSession::WsSession(const HttpServer::SessionPtr& session, WsServer* owner) :
    m_client(session->getClient()),
    m_loop(session->getLoop()),
    m_owner(owner),
    m_decodeData(WsParseBuf()), m_write_ctx(),
    m_parser(m_decodeData)
{
    session->transferBufferToWsSession(&m_recvBuf);
    readyState = WsStatus::CLOSED;

    std::cout << "websocket session created" << std::endl;
}

WsSession::~WsSession()
{
    std::cout << "websocket session destroyed" << std::endl;
}


void WsSession::connect()
{
    readyState = WsStatus::CONNECTING;

    //重新开始监听
    m_client->data = this;
    uv_read_start(m_client, recv_alloc_cb, recv_cb);
    std::cout << "websocket session start read" << std::endl;

    readyState = WsStatus::OPEN;
    if (m_onConnectCb)
    {
        m_onConnectCb(shared_from_this());
    }

}


void WsSession::close()
{
    if (readyState == WsStatus::CLOSED)
        return ;

    readyState = WsStatus::CLOSED;
    uv_read_stop(m_client);

    uv_close((uv_handle_t*)m_client, [](uv_handle_t* handle)
    {
        delete handle;
    });

    m_owner->removeWsSession(shared_from_this());

    if (m_onCloseCb)
    {
        m_onCloseCb(shared_from_this());
    }

    std::cout << "close websocket Session" << std::endl;
}

void WsSession::recv_alloc_cb(uv_handle_t* handle,
                            size_t suggested_size,
                            uv_buf_t* buf)
{
    WsSession* self = static_cast<WsSession*>(handle->data);
    assert(self != nullptr);

    size_t bufSize = self->m_recvBuf.len;
    if (bufSize < suggested_size)
    {
        while (bufSize < suggested_size)
        {
            bufSize += (bufSize << 1);
        }
        self->m_recvBuf.len = bufSize;
        self->m_recvBuf.base = static_cast<char*>(realloc(self->m_recvBuf.base, bufSize));
    }

    *buf = self->m_recvBuf;
}

void WsSession::recv_cb(uv_stream_t* stream,
                        ssize_t nread,
                        const uv_buf_t* buf)
{
    WsSession* self = static_cast<WsSession*>(stream->data);
    assert(self != nullptr);
    std::cout << "websocket session recv " << nread << " bytes" << std::endl;

    if (nread < 0)
    {
        if (nread != UV_EOF)
        {
            std::cerr << "recv err: " << uv_err_name(nread) << std::endl;
        } else
        {
            std::cout << "connection closed" << std::endl;
        }

        self->close();
        return ;
    }

    if (buf->base)
    {
        self->handleMessage(nread);
    }
}

void WsSession::handleMessage(size_t nread)
{
    std::cout << "websocket session handle message" << std::endl;
    WsParseErr err = m_parser.parse(m_recvBuf, nread);

    std::cout << "websocket parsed " << static_cast<int>(err) << std::endl;
    if (err == WsParseErr::NOT_COMPLETED)
        return ;
    if (err != WsParseErr::SUCCESS)
    {
        if (m_onErrorCb)
        {
            m_onErrorCb(shared_from_this());
        }
        return ;
    }

    int opcode = m_parser.getOpcode();
    std::cout << "opcode " << opcode << std::endl;
    if (opcode == WS_CLOSE)
    {
        close();
        return ;
    }
    if (opcode == WS_PING)
    {
        sendPong();
        return ;
    }

    if (m_onMessageCb)
    {
        std::cout << "onMessage" << std::endl;
        m_write_ctx.clearMsg();
        m_onMessageCb(shared_from_this());
        inter_send(WS_TEXT);
    }
}

lept_value WsSession::getJsonMessage()
{
    lept_value json;
    int ret = json.parse(getStrMessage());
    if (ret != LEPT_PARSE_OK)
    {
        std::cerr << "websocket frame json parse error" << std::endl;
    }
    json.set_null();
    return json;
}

void WsSession::send(const lept_value& json)
{
    m_write_ctx.setMsg(json.stringify());
}

void WsSession::send(const std::string& str)
{
    m_write_ctx.setMsg(str);
}

void WsSession::send(const char* str)
{
    m_write_ctx.setMsg(std::string(str));
}

void WsSession::inter_send(uint8_t opcode)
{
    std::cout << "ws session inter send" << std::endl;
    // 发送帧头

    std::vector<char>& header = m_write_ctx.m_head;
    header.clear();
    header.emplace_back(0x80 | opcode);
    uint8_t mask = 0x00;
    // 发送数据长度
    int payload_len = m_write_ctx.m_str.size();
    if (payload_len < 126)
    {
        header.emplace_back(static_cast<uint8_t>(payload_len | mask));
    }
    else if (payload_len < (1 << 16))
    {
        header.emplace_back(126 | mask);
        header.emplace_back(static_cast<uint8_t>(payload_len >> 8));
        header.emplace_back(static_cast<uint8_t>(payload_len | 0xFF));
    }
    else
    {
        header.push_back(127 | mask);
        for (int i = 0;i < 8;i ++)
        {
            header.emplace_back(static_cast<uint8_t>(payload_len >> (i * 8)));
        }
    }

    m_write_ctx.buf[0] = uv_buf_init(header.data(), header.size());

    // 发送数据
    m_write_ctx.buf[1] = uv_buf_init(m_write_ctx.m_str.data(),
                                    m_write_ctx.m_str.size());
    uv_write(&m_write_ctx.req,
                m_client,
                m_write_ctx.buf, 2,
                [](uv_write_t* req, int status)
    {
        if (status < 0)
        {
            std::cerr << "websocket send err: " << uv_err_name(status) << std::endl;
        }
    });
}

void WsSession::sendPong()
{
    send(getStrMessage());
    inter_send(WS_PONG);
}