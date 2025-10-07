//
// Created by AWAY on 25-9-29.
//

#pragma once

#include "http_parser.h"
#include <cstring>
#include <iostream>
#include <map>
#include "uv.h"
#include "leptjson.h"
#include <functional>
#include "FileReader.h"


class httpResp;

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

static std::string httpStatus_str(httpStatus status);

using headerMap = std::map<std::string, std::string>;
using httpRespPtr = std::shared_ptr<httpResp>;

class httpResp : public std::enable_shared_from_this<httpResp> {
public:
    template<typename T>
    httpResp(std::shared_ptr<T> ctx);
    ~httpResp();
public:
    /*
     * 工厂模式
     */
    template<typename T>
    static httpRespPtr create(std::shared_ptr<T> ctx)
    {
        return std::make_shared<httpResp>(ctx);
    }

    void init();

    /*
     * 用户发送方法
     */
    void sendStr(const std::string& str);
    void sendJson(lept_value& json);
    void sendFile(const std::string& path);

    void setHeader(const std::string& key, const std::string& value);
    void setStatus(httpStatus status);

    /*
     * 完成回调，发送请求
     */
    void onCompleteAndSend(const std::function<void(httpResp*)>&& cb);
    void onCompleteAndSend(const std::function<void(httpResp*)>& cb)
    {
        onCompleteAndSend(std::move(cb));
    }
    void clearContent();
private:
    /*
     * 写入上下文
     */
    class WriteContext
    {
    public:
        uv_write_t req;
        httpRespPtr resp;
    };

    std::string m_body;
    httpStatus m_status;
    headerMap m_headers;
    std::string m_version;
    int m_content_length;
    uv_stream_t *m_client;
    std::string m_url;
    std::string m_filePath;

    std::function<void(httpResp*)> m_onComplete;
    enum class SendType
    {
        NONE = 0,
        STR = 1,
        JSON = 2,
        FILE = 3,
    };

    SendType m_sendType;
    FileReader m_reader;
    std::string m_head;

    std::vector<uv_buf_t> m_buffers;

    /*
     * 写入完成回调
     */
    static void onWriteEnd(uv_write_t *req, int status);

    /*
     * 内部发送请求
     */
    void send();


};


