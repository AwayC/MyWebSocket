//
// Created by AWAY on 25-9-21.
//

#pragma once

#include <string>
#include <cstring>
#include <functional>
#include <map>

#include "uv.h"
#include "http_parser.h"
#include "leptjson.h"
#include "../src/core/httpResp.h"

#define HTTP_DEFAULT_PORT 8081
#define HTTP_DEFAULT_KEEP_ALIVE_TIMEOUT 20
#define HTTP_DEFAULT_RECV_BUF_SIZE 20

struct httpReq
{
    std::string url ;
    headerMap headers;
    std::string body;
    http_method method;

    std::string version;

};

class HttpServer {
public:
    HttpServer(const std::string& ip, int port = HTTP_DEFAULT_PORT);
    ~HttpServer();

    void start();
    void post(const std::string& url, std::function<void(httpReq*, httpResp*)>& callback);
    void get(const std::string& url, std::function<void(httpReq*, httpResp* )>& callback);
    void onRequest(const std::function<void(httpReq*, httpResp*)>& callback);
    void onConnect(const std::function<void(uv_tcp_t* client)>& callback);
    void setKeepAliveTimeout(int timeout);

private:
    uv_loop_t *m_loop;
    uv_tcp_t *m_tcp_svr;
    std::string m_ip;
    int m_port;

    int m_keepAliveTimeout;

    std::function<void(uv_tcp_t* client)> onConnectCb;
    std::function<void(httpReq*, httpResp*)> onRequestCb;

    struct Session
    {
        uv_stream_t *m_client;
        http_parser m_parser;
        http_parser_settings m_settings{};

        std::string m_tmpKey;
        httpReq m_req;
        httpResp m_resp;
        HttpServer* m_owner;

        char* m_recvBuf;
        size_t m_recvBufSize;

        uv_timer_t m_keepAliveTimer;
        bool m_isKeepAlive;

        Session(uv_stream_t *client, HttpServer* owner);
        ~Session();
        static void recv_alloc_cb(uv_handle_t* handle,
                            size_t suggested_size,
                            uv_buf_t* buf);

        static void recv_cb(uv_stream_t* client,
                           ssize_t nread,
                           const uv_buf_t* buf);

        static int onReqHeaderComplete(http_parser* parser);
        static int onReqMessageBegin(http_parser* parser);
        static int onReqMessageComplete(http_parser* parser);
        static int onReqURL(http_parser* parser, const char* at, size_t length);
        static int onReqHeaderField(http_parser* parser, const char* at, size_t length);
        static int onReqHeaderValue(http_parser* parser, const char* at, size_t length);
        static int onReqBody(http_parser* parser, const char* at, size_t length);

        static bool needKeepConnection (httpReq* req);
        static void closeTcp(uv_stream_t* client);

        void handle_request(char* data, size_t size, uv_stream_t* client);
        void startKeepAliveTimer();
        void stopKeepAliveTimer();
        static void keepAliveTimerCb(uv_timer_t* timer);
    };

    std::unordered_map<std::string, std::function<void(httpReq*, httpResp*)>> post_callbacks;
    std::unordered_map<std::string, std::function<void(httpReq*, httpResp*)>> get_callbacks;


    static void inter_on_connect(uv_stream_t *server, int status);
    void handle_connect(uv_stream_t *client);
    void handle_post(httpReq* req, httpResp* resp);
    void handle_get(httpReq* req, httpResp* resp);

};



