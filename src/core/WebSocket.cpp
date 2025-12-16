//
// Created by AWAY on 25-10-7.
//

#include "WebSocket.h"
#include "WsServer.h"
#include <cassert>
#include <WsServer.h>
#include "FileReader.h"
#define WS_CALLBACK(cb, ...) do { \
        if(cb) { \
            cb(__VA_ARGS__); \
        } \
    } while (0)


WsSession::WsSession(const HttpServer::SessionPtr& session, WsServer* owner) :
    m_client(session->getClient()),
    m_loop(session->getLoop()),
    m_owner(owner)
{
    // recvBuf 继承自 httpSession
    session->transferBufferToWsSession(&m_recvBuf);
    readyState = WsStatus::CLOSED;

    m_parser.onComplete([this]()
    {
        this->handleWsFrame();
    });
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

    readyState = WsStatus::OPEN;
    WS_CALLBACK(m_onConnectCb, shared_from_this());

}

void WsSession::inter_close()
{
    readyState = WsStatus::CLOSED;

    uv_read_stop(m_client);

    uv_close((uv_handle_t*)m_client, [](uv_handle_t* handle)
    {
        delete handle;
    });

    WS_CALLBACK(m_onCloseCb, shared_from_this());
    m_owner->removeWsSession(shared_from_this());
}

void WsSession::close()
{
    if (readyState == WsStatus::CLOSED)
        return ;

    inter_close();
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
    std::cout << "ws recv_cb" << std::endl;
    WsSession* self = static_cast<WsSession*>(stream->data);
    assert(self != nullptr);

    if (nread < 0)
    {
        if (nread != UV_EOF)
        {
            std::cerr << "recv err: " << uv_err_name(nread) << std::endl;
        } else
        {
            std::cout << "websocket connection closed" << std::endl;
        }

        self->inter_close();
        return ;
    }

    if (buf->base && self->readyState == WsStatus::OPEN)
    {
        self->handleMessage(nread);
    }
}

void WsSession::handleMessage(size_t nread)
{
    WsParseErr err = m_parser.parse(m_recvBuf, nread, &m_frame);

    if (err != WsParseErr::SUCCESS)
    {
        std::cerr << "websocket session parse err: " << WsErr_Str(err) << std::endl;
        std::runtime_error ex("websocket session parse error");
        WS_CALLBACK(m_onErrorCb, shared_from_this(), ex);
        inter_close();
        return ;
    }
}

void WsSession::handleWsFrame()
{
    // reject no mask
    if (!m_frame.mask)
    {
        std::cerr << "websocket message no mask" << std::endl;
        WS_CALLBACK(m_onErrorCb, shared_from_this(), static_cast<std::exception>(std::runtime_error("websocket message no mask")));
        inter_close();
        return ;
    }

    switch (m_frame.opcode)
    {
        case WS_PONG:
            break;
        case WS_PING:
            sendPong();
            break;
        case WS_CLOSE:
            inter_close();
            break;
        default:
            WS_CALLBACK(m_onMessageCb, shared_from_this());
            break;
    }

}

lept_value WsSession::getJsonMessage()
{
    lept_value json;
    int ret = json.parse(std::string(getStrMessage()));
    if (ret != LEPT_PARSE_OK)
    {
        std::cerr << "websocket frame json parse error" << std::endl;
        json.set_null();
    }
    return json;
}

void WsSession::send(const lept_value& json)
{
    auto *ctx = new WriteCtx();
    ctx->setMsg(json.stringify());
    inter_send(ctx, WS_TEXT);
}

void WsSession::send(const std::string& str)
{
    auto *ctx = new WriteCtx();
    ctx->setMsg(str);
    inter_send(ctx, WS_TEXT);
}

void WsSession::send(std::string&& str)
{
    auto *ctx = new WriteCtx();
    ctx->setMsg(std::move(str));
    inter_send(ctx, WS_TEXT);
}

void WsSession::send(const char* str)
{
    this->send(std::string(str));
}

void WsSession::inter_send(WriteCtx* ctx, uint8_t opcode)
{
    // 发送帧头

    std::vector<char>& header = ctx->m_head;
    header.clear();
    header.emplace_back(0x80 | opcode);
    uint8_t mask = 0x00;
    FileReader* reader = ctx->m_reader;
    // 发送数据长度
    size_t payload_len;
    if (ctx->isFile)
    {
        payload_len = reader->getReadByte();
    } else
    {
        payload_len = ctx->m_str.size();
    }

    if (payload_len < 126)
    {
        header.emplace_back(static_cast<uint8_t>(payload_len | mask));
    }
    else if (payload_len < (1 << 16))
    {
        header.emplace_back(126 | mask);
        header.emplace_back(static_cast<uint8_t>(payload_len >> 8));
        header.emplace_back(static_cast<uint8_t>(payload_len & 0xFF));
    }
    else
    {
        header.emplace_back(127 | mask);
        for (int i = 7;i >= 0;i --)
        {
            header.emplace_back(static_cast<uint8_t>(payload_len >> (i * 8)));
        }
    }

    ctx->m_buffers.push_back(uv_buf_init(header.data(), header.size()));

    // 发送数据
    if (ctx->isFile)
    {
        reader->appendToBuff(ctx->m_buffers);
    } else
    {
        ctx->m_buffers.push_back(uv_buf_init(ctx->m_str.data(),
                                    ctx->m_str.size()));
    }

    ctx->req.data = ctx;
    ctx->m_owner = shared_from_this();
    uv_write(&ctx->req,
                m_client,
                ctx->m_buffers.data(),
                ctx->m_buffers.size(),
                [](uv_write_t* req, int status)
    {
        if (status < 0)
        {
            auto context = static_cast<WriteCtx*>(req->data);
            std::cerr << "websocket send err: " << uv_err_name(status) << std::endl;
            auto ss = context->m_owner;
            WS_CALLBACK(ss->m_onErrorCb, ss, static_cast<std::exception>(std::runtime_error("websocket send error")));
            delete context;
            ss->inter_close();
        }

        std::cout << "websocket send success" << std::endl;
        delete static_cast<WriteCtx*>(req->data);
    });
}


void WsSession::sendPong()
{
    auto *ctx = new WriteCtx();
    ctx->setMsg("");
    inter_send(ctx, WS_PONG);
}

void WsSession::sendFile(const std::string& path)
{
    auto reader = new FileReader(m_loop);
    auto self = shared_from_this();

    reader->onError([](FileReader* ins)
    {
        std::cerr << "FileReader: Error" << std::endl;
        delete ins;
    });
    reader->onClose([self](FileReader* ins)
    {
        auto ctx = new WriteCtx();
        ctx->setMsg(ins);
        self->inter_send(ctx, WS_TEXT);
    });
    reader->fileRead(path);
}



