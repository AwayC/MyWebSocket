//
// Created by AWAY on 25-9-21.
//

#include "httpserver.h"
#include <cassert>
#include <iostream>
#include <unordered_map>
#include <sys/proc.h>
#include <fstream>
#include <filesystem>


/************* Session *************/
HttpServer::Session::Session(uv_stream_t *client, HttpServer* owner) : m_client(client), m_resp(client)
{
    m_owner = owner;
    m_recvBufSize = HTTP_DEFAULT_RECV_BUF_SIZE;
    m_recvBuf = new char[m_recvBufSize];
    memset(&m_settings, 0, sizeof(http_parser_settings));
    m_settings.on_message_begin = onReqMessageBegin;
    m_settings.on_url = onReqURL;
    m_settings.on_header_field = onReqHeaderField;
    m_settings.on_header_value = onReqHeaderValue;
    m_settings.on_headers_complete = onReqHeaderComplete;
    m_settings.on_body = onReqBody;
    m_settings.on_message_complete = onReqMessageComplete;

    //keep alive
    m_isKeepAlive = false;
    uv_timer_init(owner->m_loop, &m_keepAliveTimer);
    m_keepAliveTimer.data = this;

    //http parser
    http_parser_init(&m_parser, HTTP_REQUEST);
    m_parser.data = this;
};
HttpServer::Session::~Session()
{
    delete[] m_recvBuf;
};

bool HttpServer::Session::needKeepConnection (httpReq* req)
{
    bool ret = false;
    if (req->headers.find("Connection") != req->headers.end())
    {
        ret = req->headers["Connection"] == "keep-alive";
    } else
    {
        std::cout << req->version << std::endl;
    }
    return ret;
}

void HttpServer::Session::closeTcp(uv_stream_t* client)
{
    uv_read_stop(client);
    uv_close((uv_handle_t*)client, [](uv_handle_t* client){
        Session* self = static_cast<Session*>(client->data);
        delete self;
        delete client;
    });
}

void HttpServer::Session::recv_alloc_cb(uv_handle_t* handle,
                        size_t suggested_size,
                        uv_buf_t* buf)
{
    Session* self = static_cast<Session*>(handle->data);
    int bufSize = self->m_recvBufSize;
    if (bufSize < suggested_size)
    {
        while (bufSize < suggested_size)
        {
            bufSize += (bufSize << 1);
        }
        self->m_recvBufSize = bufSize;
        self->m_recvBuf = static_cast<char*>(realloc(self->m_recvBuf, bufSize));
    }

    buf->base = self->m_recvBuf;
    buf->len = suggested_size;
}

void HttpServer::Session::recv_cb(uv_stream_t* client,
                       ssize_t nread,
                       const uv_buf_t* buf)
{
    Session* ep = (Session*)(client->data);

    if (nread < 0)
    {
        if (nread != UV_EOF) {
            std::cerr << "recv error: " << uv_err_name(nread) << std::endl;
        } else {
            std::cout << "Connection closed by peer" << std::endl;
        }
        ep->stopKeepAliveTimer();
        closeTcp(client);
        return;
    }

    if (ep->m_isKeepAlive)
    {
        ep->stopKeepAliveTimer();
    }

    if (buf->base)
    {
        ep->handle_request(buf->base, nread, client);
    }
}

void HttpServer::Session::handle_request(char* data, size_t size, uv_stream_t* client)
{
    size_t nparsed = http_parser_execute(&m_parser, &m_settings, data, size);
    if (m_parser.upgrade)
    {
        //todo: upgrade to websocket

    } else if (nparsed != size)
    {
        std::cerr << "parse error" << std::endl;
    } else
    {
        if (m_req.method == HTTP_GET)
        {
            m_owner->handle_get(&m_req, &m_resp);
        } else if (m_req.method == HTTP_POST)
        {
            m_owner->handle_post(&m_req, &m_resp);
        }
        m_owner->onRequestCb(&m_req, &m_resp);

        if (!needKeepConnection(&m_req))
        {
            std::cout << "close connection" << std::endl;
            closeTcp(client);
        } else
        {
            //keep alive
            std::cout << "keep alive connection" << std::endl;
            m_isKeepAlive = true;
            startKeepAliveTimer();
        }
    }
}

void HttpServer::Session::keepAliveTimerCb(uv_timer_t* timer)
{
    Session* ep = (Session*)timer->data;
    closeTcp(ep->m_client);
}

void HttpServer::Session::startKeepAliveTimer()
{
    uv_timer_start(&m_keepAliveTimer, keepAliveTimerCb, m_owner->m_keepAliveTimeout * 1000, 0);
    m_isKeepAlive = true;
}

void HttpServer::Session::stopKeepAliveTimer()
{
    m_isKeepAlive = false;
    uv_timer_stop(&m_keepAliveTimer);
}


int HttpServer::Session::onReqMessageBegin(http_parser* parser)
{
    std::cout << "onReqMessageBegin" << std::endl;
    return 0;
}

int HttpServer::Session::onReqHeaderComplete(http_parser* parser)
{
    Session* ep = (Session*)parser->data;
    assert(ep != nullptr);
    ep->m_req.method = parser->method;
    ep->m_req.version = std::to_string(parser->http_major) + "." + std::to_string(parser->http_minor);
    std::cout << "onReqHeaderComplete" << std::endl;
    return 0;
}

int HttpServer::Session::onReqMessageComplete(http_parser* parser)
{
    std::cout << "onReqMessageComplete" << std::endl;
    return 0;
}

int HttpServer::Session::onReqURL(http_parser* parser, const char* at, size_t length)
{
    Session* ep = (Session*)parser->data;
    assert(ep != nullptr);
    ep->m_req.url.assign(at, length);
    return 0;
}

int HttpServer::Session::onReqHeaderField(http_parser* parser, const char* at, size_t length)
{
    Session* ep = (Session*)parser->data;
    assert(ep != nullptr);
    ep->m_tmpKey.assign(at, length);
    return 0;
}

int HttpServer::Session::onReqHeaderValue(http_parser* parser, const char* at, size_t length)
{
    std::string value(at, length);
    Session* ep = (Session*)parser->data;
    assert(ep != nullptr);
    ep->m_req.headers.insert(std::make_pair(ep->m_tmpKey, value));
    return 0;
}

int HttpServer::Session::onReqBody(http_parser* parser, const char* at, size_t length)
{
    Session* ep = (Session*)parser->data;
    assert(ep != nullptr);
    ep->m_req.body.append(at, length);
    return 0;
}

/************* HttpServer *************/

void HttpServer::handle_connect(uv_stream_t* client)
{
    Session* ep = new Session(client, this);
    std::cout << "handle request" << std::endl;
    client->data = ep;
    uv_read_start(client, Session::recv_alloc_cb, Session::recv_cb);
}


HttpServer::HttpServer(const std::string& ip, int port)
    : m_ip(ip),
    m_port(port),
    onConnectCb(nullptr) ,onRequestCb(nullptr)
{
    m_keepAliveTimeout = HTTP_DEFAULT_KEEP_ALIVE_TIMEOUT;
    m_loop = uv_default_loop();
    m_tcp_svr = new uv_tcp_t();
    int ret = uv_tcp_init(m_loop, m_tcp_svr);
    if (ret != 0)
    {
        std::cerr << "uv_tcp_init failed" << std::endl;
    }
    assert(ret == 0);

    m_tcp_svr->data = this;

    struct sockaddr_in addr;
    uv_ip4_addr(ip.c_str(), port, &addr);
    //bind
    ret = uv_tcp_bind(m_tcp_svr, (const struct sockaddr *)&addr, 0);
    if (ret != 0)
    {
        std::cerr << "uv_tcp_bind failed" << std::endl;
    }
    assert(ret == 0);

    //listen
    ret = uv_listen((uv_stream_t*)m_tcp_svr, SOMAXCONN, inter_on_connect);
    if (ret != 0)
    {
        std::cerr << "uv_listen failed" << std::endl;
    }
    assert(ret == 0);
}

void HttpServer::start() {
    std::cout << "server start" << std::endl;
    uv_run(m_loop, UV_RUN_DEFAULT);
}

HttpServer::~HttpServer() {
    delete m_tcp_svr;
    uv_loop_close(m_loop);
}

void HttpServer::inter_on_connect(uv_stream_t *server, int status)
{
    HttpServer* self = static_cast<HttpServer*>(server->data);
    assert(self != nullptr);
    if (status < 0)
    {
        std::cerr << "inter_on_connect failed: " << uv_err_name(status) << std::endl;
        //error
        return ;
    }

    uv_tcp_t *client = new uv_tcp_t();
    assert(client != nullptr);
    uv_tcp_init(self->m_loop, client);

    if (uv_accept(server, (uv_stream_t*)client) == 0)
    {
        std::cout << "new connection" << std::endl;
        if (self->onConnectCb)
        {
            self->onConnectCb(client);
        }
        self->handle_connect((uv_stream_t*)client);

    } else
    {
        std::cerr << "uv_accept failed" << std::endl;
        uv_close((uv_handle_t*)client, [](uv_handle_t* client){
            delete client;
        });
    }
}

void HttpServer::post(const std::string& url, std::function<void(httpReq*, httpResp*)>& callback)
{
    post_callbacks.insert(std::make_pair(url, callback));
}

void HttpServer::get(const std::string& url, std::function<void(httpReq*, httpResp*)>& callback)
{
    get_callbacks.insert(std::make_pair(url, callback));
}

void HttpServer::handle_post(httpReq* req, httpResp* resp)
{
    auto it = post_callbacks.find(req->url);
    if (it != post_callbacks.end())
    {
        it->second(req, resp);
    }
}

void HttpServer::handle_get(httpReq* req, httpResp* resp)
{
    auto it = get_callbacks.find(req->url);
    if (it != get_callbacks.end())
    {
        it->second(req, resp);
    }
}

void HttpServer::onConnect(const std::function<void(uv_tcp_t* client)>& callback)
{
    onConnectCb = callback;
}

void HttpServer::onRequest(const std::function<void(httpReq*, httpResp*)>& callback)
{
    onRequestCb = callback;
}

void HttpServer::setKeepAliveTimeout(int timeout)
{
    m_keepAliveTimeout = timeout;
}
