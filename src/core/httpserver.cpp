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
HttpServer::Session::Session(uv_stream_t *client, HttpServer* owner) : m_client(client)
{
    m_resp = nullptr;
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
    std::cout << "session destroyed" << std::endl;
};

void HttpServer::Session::init()
{
    m_resp = httpResp::create(shared_from_this());
    m_resp->init();
}

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

void HttpServer::Session::close(bool isUpgrade)
{
    auto self = shared_from_this();
    m_owner->closeSession(self, isUpgrade);
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
        ep->close();
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
        upgradeToWs();

    } else if (nparsed != size)
    {
        std::cerr << "parse error" << std::endl;
    } else
    {
        if (m_req.method == HTTP_GET)
        {
            m_owner->handle_get(&m_req, m_resp.get());
        } else if (m_req.method == HTTP_POST)
        {
            m_owner->handle_post(&m_req, m_resp.get());
        } else
        {
            if (m_owner->onRequestCb)
                m_owner->onRequestCb(&m_req, m_resp.get());
            else
                m_owner->handle_errReq(&m_req, m_resp.get());
        }
        onRequest();
    }
}

void HttpServer::Session::onRequest()
{
    auto self = shared_from_this( );
    std::cout << "on request" << std::endl;
    std::cout << "session use count: " << self.use_count() << std::endl;
    m_resp->onCompleteAndSend([self](httpResp* resp){
        assert(self != nullptr);
        if (needKeepConnection(&self->m_req))
        {
            //keep alive
            std::cout << "keep alive connection" << std::endl;
            self->m_isKeepAlive = true;
            self->startKeepAliveTimer();
        } else
        {
            std::cout << "close connection" << std::endl;
            self->close();
        }
        resp->clearContent();
    });
    std::cout << "session use count after send: " << self.use_count() << std::endl;
}

void HttpServer::Session::keepAliveTimerCb(uv_timer_t* timer)
{
    Session* ep = (Session*)timer->data;
    if (ep != nullptr)
        ep->close();
}

void HttpServer::Session::startKeepAliveTimer()
{
    m_keepAliveTimer.data = this;
    uv_timer_start(&m_keepAliveTimer, keepAliveTimerCb, m_owner->m_keepAliveTimeout * 1000, 0);
    m_isKeepAlive = true;
}

void HttpServer::Session::stopKeepAliveTimer()
{
    m_isKeepAlive = false;
    m_keepAliveTimer.data = nullptr;
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

void HttpServer::Session::upgradeToWs()
{
    if (m_owner->onUpgradeCb)
        m_owner->onUpgradeCb(shared_from_this());
    else
    {
        std::cerr << "upgradeToWs: onUpgradeCb is not set" << std::endl;
        close(false);
        return ;
    }

    close(true);
}

/************* HttpServer *************/
void HttpServer::handle_connect(uv_stream_t* client)
{
    auto ss = Session::create(client, this);
    ss->init();

    std::cout << "handle request" << std::endl;
    client->data = ss.get();
    std::cout << "session use count: " << ss.use_count() << std::endl;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions.push_back(ss);
    }
    std::cout << "session use count: " << ss.use_count() << std::endl;
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

void HttpServer::post(const std::string& url, const std::function<void(httpReq*, httpResp*)>& callback)
{
    post_callbacks.insert(std::make_pair(url, callback));
}

void HttpServer::get(const std::string& url, const std::function<void(httpReq*, httpResp*)>& callback)
{
    get_callbacks.insert(std::make_pair(url, callback));
}

void HttpServer::handle_post(httpReq* req, httpResp* resp)
{
    auto it = post_callbacks.find(req->url);
    if (it != post_callbacks.end())
    {
        it->second(req, resp);
    } else
    {
        handle_errReq(req, resp);
    }
}

void HttpServer::handle_get(httpReq* req, httpResp* resp)
{
    auto it = get_callbacks.find(req->url);
    if (it != get_callbacks.end())
    {
        it->second(req, resp);
    } else
    {
        handle_errReq(req, resp);
    }
}

void HttpServer::handle_errReq(httpReq* req, httpResp* resp)
{
    resp->setStatus(httpStatus::NOT_FOUND);
    resp->sendStr("404 Not Found");
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

void HttpServer::closeSession(SessionPtr &session, bool isUpgrade)
{
    uv_read_stop(session->m_client);

    if (!isUpgrade)
    {
        uv_stream_t* client = session->m_client;
        uv_close((uv_handle_t*)client, [](uv_handle_t* client){
            delete client;
        });
    }

    std::cout << "session use count: " << session.use_count() << std::endl;
    removeSession(session);
    std::cout << "session use count: " << session.use_count() << std::endl;
    std::cout << "close session" << std::endl;

}

void HttpServer::removeSession(SessionPtr &session)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find(m_sessions.begin(), m_sessions.end(), session);

    if (it != m_sessions.end()) {
        // 2. 与最后一个元素交换
        // std::iter_swap 比 *it = std::move(sessions.back()) 更通用
        std::iter_swap(it, m_sessions.end() - 1);

        // 3. 弹出最后一个元素
        m_sessions.pop_back();
    }
}

void HttpServer::onUpgrade(const std::function<void(std::shared_ptr<Session>)>& callback) {
    onUpgradeCb = callback;
}

