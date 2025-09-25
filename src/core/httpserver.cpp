//
// Created by AWAY on 25-9-21.
//

#include "httpserver.h"
#include <cassert>
#include <iostream>
#include <unordered_map>

/************* endPoint *************/
httpserver::endPoint::endPoint(uv_stream_t *client, httpserver* owner) : _client(client)
{
    _owner = owner;
    _parser = new http_parser();
    memset(&_settings, 0, sizeof(http_parser_settings));
    _settings.on_message_begin = onReqMessageBegin;
    _settings.on_url = onReqURL;
    _settings.on_header_field = onReqHeaderField;
    _settings.on_header_value = onReqHeaderValue;
    _settings.on_headers_complete = onReqHeaderComplete;
    _settings.on_body = onReqBody;
    _settings.on_message_complete = onReqMessageComplete;

    http_parser_init(_parser, HTTP_REQUEST);
    _parser->data = this;
};
httpserver::endPoint::~endPoint()
{
    delete _parser;
};

void httpserver::endPoint::recv_alloc_cb(uv_handle_t* handle,
                        size_t suggested_size,
                        uv_buf_t* buf)
{
    buf->base = new char[suggested_size];
    buf->len = suggested_size;
}
void httpserver::endPoint::recv_cb(uv_stream_t* client,
                       ssize_t nread,
                       const uv_buf_t* buf)
{
    if (nread < 0)
    {
        if (nread != UV_EOF) {
            std::cerr << "recv error: " << uv_err_name(nread) << std::endl;
        } else {
            std::cout << "Connection closed by peer" << std::endl;
        }
        uv_read_stop(client);
        uv_close((uv_handle_t*)client, [](uv_handle_t* client){
            delete (endPoint*)client->data;
            delete client;
        });
        if (buf->base)
        {
            delete[] buf->base;
        }
        return ;
    }

    if (buf->base)
    {
        endPoint* ep = (endPoint*)(client->data);
        ep->handle_request(buf->base, nread);
        delete[] buf->base;
    }
}

void httpserver::endPoint::handle_request(char* data, size_t size)
{
    size_t nparsed = http_parser_execute(_parser, &_settings, data, size);

    if (_parser->upgrade)
    {
        //todo: upgrade to websocket

    } else if (nparsed != size)
    {
        std::cerr << "parse error" << std::endl;
    } else
    {
        if (_req._method == HTTP_GET)
        {
            _owner->handle_get(&_req, &_resp);
        } else if (_req._method == HTTP_POST)
        {
            _owner->handle_post(&_req, &_resp);
        }
        _owner->onRequestCb(&_req, &_resp);


        return;
    }
}


int httpserver::endPoint::onReqMessageBegin(http_parser* parser)
{
    std::cout << "onReqMessageBegin" << std::endl;
    return 0;
}

int httpserver::endPoint::onReqHeaderComplete(http_parser* parser)
{
    endPoint* ep = (endPoint*)parser->data;
    assert(ep != nullptr);
    ep->_req._method = parser->method;
    ep->_req._version = std::to_string(parser->http_major) + "." + std::to_string(parser->http_minor);
    std::cout << "onReqHeaderComplete" << std::endl;
    return 0;
}

int httpserver::endPoint::onReqMessageComplete(http_parser* parser)
{
    std::cout << "onReqMessageComplete" << std::endl;
    return 0;
}

int httpserver::endPoint::onReqURL(http_parser* parser, const char* at, size_t length)
{
    endPoint* ep = (endPoint*)parser->data;
    assert(ep != nullptr);
    ep->_req._url.assign(at, length);
    return 0;
}

int httpserver::endPoint::onReqHeaderField(http_parser* parser, const char* at, size_t length)
{
    endPoint* ep = (endPoint*)parser->data;
    assert(ep != nullptr);
    ep->_tmpKey.assign(at, length);
    return 0;
}

int httpserver::endPoint::onReqHeaderValue(http_parser* parser, const char* at, size_t length)
{
    std::string value(at, length);
    endPoint* ep = (endPoint*)parser->data;
    assert(ep != nullptr);
    ep->_req._headers.insert(std::make_pair(ep->_tmpKey, value));
    return 0;
}

int httpserver::endPoint::onReqBody(http_parser* parser, const char* at, size_t length)
{
    endPoint* ep = (endPoint*)parser->data;
    assert(ep != nullptr);
    ep->_req._body.append(at, length);
    return 0;
}

/************* httpserver *************/

void httpserver::handle_connect(uv_stream_t* client)
{
    endPoint* ep = new endPoint(client, this);
    std::cout << "handle request" << std::endl;
    client->data = ep;
    uv_read_start(client, endPoint::recv_alloc_cb, endPoint::recv_cb);
}


httpserver::httpserver(const std::string& ip, int port)
    : ip(ip), port(port) , onConnectCb(nullptr) ,onRequestCb(nullptr) {
    loop = uv_default_loop();
    tcp_svr = new uv_tcp_t();
    int ret = uv_tcp_init(loop, tcp_svr);
    if (ret != 0)
    {
        std::cerr << "uv_tcp_init failed" << std::endl;
    }
    assert(ret == 0);

    tcp_svr->data = this;

    struct sockaddr_in addr;
    uv_ip4_addr(ip.c_str(), port, &addr);
    //bind
    ret = uv_tcp_bind(tcp_svr, (const struct sockaddr *)&addr, 0);
    if (ret != 0)
    {
        std::cerr << "uv_tcp_bind failed" << std::endl;
    }
    assert(ret == 0);

    //listen
    ret = uv_listen((uv_stream_t*)tcp_svr, SOMAXCONN, inter_on_connect);
    if (ret != 0)
    {
        std::cerr << "uv_listen failed" << std::endl;
    }
    assert(ret == 0);
}

void httpserver::start() {
    std::cout << "server start" << std::endl;
    uv_run(loop, UV_RUN_DEFAULT);
}

httpserver::~httpserver() {
    delete tcp_svr;
    uv_loop_close(loop);
}

void httpserver::inter_on_connect(uv_stream_t *server, int status)
{
    httpserver* self = static_cast<httpserver*>(server->data);
    assert(self != nullptr);
    if (status < 0)
    {
        std::cerr << "inter_on_connect failed: " << uv_err_name(status) << std::endl;
        //error
        return ;
    }

    uv_tcp_t *client = new uv_tcp_t();
    assert(client != nullptr);
    uv_tcp_init(self->loop, client);

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

void httpserver::post(const std::string& url, std::function<void(httpReq*, httpResp*)>& callback)
{
    post_callbacks.insert(std::make_pair(url, callback));
}

void httpserver::get(const std::string& url, std::function<void(httpReq*, httpResp*)>& callback)
{
    get_callbacks.insert(std::make_pair(url, callback));
}

void httpserver::handle_post(httpReq* req, httpResp* resp)
{
    auto it = post_callbacks.find(req->_url);
    if (it != post_callbacks.end())
    {
        it->second(req, resp);
    }
}

void httpserver::handle_get(httpReq* req, httpResp* resp)
{
    auto it = get_callbacks.find(req->_url);
    if (it != get_callbacks.end())
    {
        it->second(req, resp);
    }
}

void httpserver::onConnect(const std::function<void(uv_tcp_t* client)>& callback)
{
    onConnectCb = callback;
}

void httpserver::onRequest(const std::function<void(httpReq*, httpResp*)>& callback)
{
    onRequestCb = callback;
}
