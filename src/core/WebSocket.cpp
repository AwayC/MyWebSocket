//
// Created by AWAY on 25-10-7.
//

#include "WebSocket.h"

#include <cassert>
#include <WsServer.h>

WsSession::WsSession(const HttpServer::SessionPtr& session, WsServer* owner)
{
    m_owner = owner;
    m_client = session->getClient();
    m_loop = session->getLoop();
    session->transferBufferToWsSession(&m_recvBuf);
    readyState = WsStatus::CLOSED;
    m_binaryType = BinaryType::blob;
}

WsSession::~WsSession()
{
    std::cout << "websocket session destroyed" << std::endl;
}


void WsSession::connect()
{
    readyState = WsStatus::CONNECTING;

    uv_read_start(m_client, recv_alloc_cb, recv_cb);

    readyState = WsStatus::OPEN;
    if (m_onConnectCb)
    {
        m_onConnectCb(this);
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
        m_onCloseCb(this);
    }

    std::cout << "close websocket Session" << std::endl;
}

void WsSession::recv_alloc_cb(uv_handle_t* handle,
                            size_t suggested_size,
                            uv_buf_t* buf)
{
    WsSession* self = static_cast<WsSession*>(handle->data);
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

    if (nread < 0)
    {
        if (nread != UV_EOF)
        {
            std::cerr << "recv erro: " << uv_err_name(nread) << std::endl;
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

void WsSession::handleMessage(int nread)
{
    if (nread == 0) return ;



    if (m_onMessageCb)
    {
        m_onMessageCb(this);
    }
}
