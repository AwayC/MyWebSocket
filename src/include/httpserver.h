//
// Created by AWAY on 25-9-21.
//

#pragma once

#include "uv.h"
#include <string>
#include <cstring>
#include <functional>
#include <map>

#include "../util/http-parser/http_parser.h"



#define DEFAULT_PORT 8081

enum class httpStatus
{
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 204,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    NOT_MODIFIED = 304,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504
};

std::string httpStatus_str(httpStatus status);

using headerMap = std::map<std::string, std::string>;

struct httpReq
{
    std::string _url ;
    headerMap _headers;
    std::string _body;
    http_method _method;

    std::string _version;

};

struct httpResp
{
    std::string _url;
    std::string _body;
    httpStatus _status;
    headerMap _headers;

    void writeHead(httpStatus status, const headerMap& headers);
    void write(std::string data);
    void setHeader(const std::string& name, const std::string& value);
    void end(std::string data);

};


class httpserver {
public:
    httpserver(const std::string& ip, int port = DEFAULT_PORT);
    ~httpserver();

    void start();
    void post(const std::string& url, std::function<void(httpReq*, httpResp*)>& callback);
    void get(const std::string& url, std::function<void(httpReq*, httpResp* )>& callback);
    void onRequest(const std::function<void(httpReq*, httpResp*)>& callback);
    void onConnect(const std::function<void(uv_tcp_t* client)>& callback);

private:
    uv_loop_t *loop;
    uv_tcp_t *tcp_svr;
    std::string ip;
    int port;

    std::function<void(uv_tcp_t* client)> onConnectCb;
    std::function<void(httpReq*, httpResp*)> onRequestCb;

    struct endPoint
    {
        uv_stream_t *_client;
        http_parser *_parser;
        http_parser_settings _settings{};

        std::string _tmpKey;
        httpReq _req;
        httpResp _resp;
        httpserver* _owner;

        endPoint(uv_stream_t *client, httpserver* owner);
        ~endPoint();
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

        void handle_request(char* data, size_t size);
    };

    std::unordered_map<std::string, std::function<void(httpReq*, httpResp*)>> post_callbacks;
    std::unordered_map<std::string, std::function<void(httpReq*, httpResp*)>> get_callbacks;


    static void inter_on_connect(uv_stream_t *server, int status);
    void handle_connect(uv_stream_t *client);
    void handle_post(httpReq* req, httpResp* resp);
    void handle_get(httpReq* req, httpResp* resp);



};



