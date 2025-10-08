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
#include "../src/util/ssl.h"

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

class HttpServer : public std::enable_shared_from_this<HttpServer> {
public:
    explicit HttpServer(const std::string& ip, int port = HTTP_DEFAULT_PORT);
    ~HttpServer();

    static std::shared_ptr<HttpServer> create(const std::string& ip, int port = HTTP_DEFAULT_PORT)
    {
        return std::make_shared<HttpServer>(ip, port);
    }

    /*
     * 启动服务器
     */
    void start();

    /*
     * post和get请求的回调设置
     */
    void post(const std::string& url, const std::function<void(httpReq*, httpResp*)>& callback);
    void get(const std::string& url, const std::function<void(httpReq*, httpResp* )>& callback);

    /*
     * 设置长连接时间
     */
    void setKeepAliveTimeout(int timeout);

    /*
     * 其他回调设置
     */
    void onRequest(const std::function<void(httpReq*, httpResp*)>& callback);
    void onConnect(const std::function<void(uv_tcp_t* client)>& callback);

    struct Session;
    using SessionPtr = std::shared_ptr<Session>;
    void onUpgrade(const std::function<void(SessionPtr)>& callback);

    uv_loop_t* getLoop() { return m_loop; }
    /*
     * 连接处理会话
     */


    struct Session : public std::enable_shared_from_this<Session>
    {
        uv_stream_t *m_client;
        HttpServer* m_owner;

        /*
         * http解析器
         */
        http_parser m_parser{};
        http_parser_settings m_settings{};
        std::string m_tmpKey;

        /*
         * 请求响应
         */
        httpReq m_req;
        std::shared_ptr<httpResp> m_resp;

        uv_buf_t m_recvBuf;

        uv_timer_t m_keepAliveTimer{};
        bool m_isKeepAlive;

        Session(uv_stream_t *client, HttpServer* owner);

        ~Session();

        static std::shared_ptr<Session> create(uv_stream_t *client, HttpServer* owner)
        {
            return std::make_shared<Session>(client, owner);
        }

        static void recv_alloc_cb(uv_handle_t* handle,
                            size_t suggested_size,
                            uv_buf_t* buf);

        static void recv_cb(uv_stream_t* client,
                           ssize_t nread,
                           const uv_buf_t* buf);

        /*
         * http解析回调
         */
        static int onReqHeaderComplete(http_parser* parser);
        static int onReqMessageBegin(http_parser* parser);
        static int onReqMessageComplete(http_parser* parser);
        static int onReqURL(http_parser* parser, const char* at, size_t length);
        static int onReqHeaderField(http_parser* parser, const char* at, size_t length);
        static int onReqHeaderValue(http_parser* parser, const char* at, size_t length);
        static int onReqBody(http_parser* parser, const char* at, size_t length);

        static bool needKeepConnection (httpReq* req);
        void close(bool isUpgrade = false);

        /*
         * 初始化会话
         */
        void init();
        void handle_request(char* data, size_t size, uv_stream_t* client);

        /*
         * 长连接操作
         */
        void startKeepAliveTimer();
        void stopKeepAliveTimer();

        /*
         * 处理回调
         */
        void onRequest();

        uv_loop_t* getLoop() const
        {
            return m_owner->m_loop;
        }
        uv_stream_t* getClient() const
        {
            return m_client;
        }

        static void onRequestComplete(httpReq* req);
        static void keepAliveTimerCb(uv_timer_t* timer);

        /*
         * 升级到websocket
         */
        void upgradeToWs();

        /*
         * 转移buffer到websocket会话
         */
        void transferBufferToWsSession(uv_buf_t* buf);
    };

private:
    uv_loop_t *m_loop;
    uv_tcp_t *m_tcp_svr;
    std::string m_ip;
    int m_port;

    int m_keepAliveTimeout;

    /*
     * 会话管理互斥锁
     */
    mutable std::mutex m_mutex;
    std::vector<SessionPtr> m_sessions;

    std::function<void(uv_tcp_t* client)> onConnectCb;
    std::function<void(httpReq*, httpResp*)> onRequestCb;
    std::function<void(std::shared_ptr<Session>)> onUpgradeCb;

    std::unordered_map<std::string, std::function<void(httpReq*, httpResp*)>> post_callbacks;
    std::unordered_map<std::string, std::function<void(httpReq*, httpResp*)>> get_callbacks;

    /*
     * 内部事件处理
     */
    static void inter_on_connect(uv_stream_t *server, int status);
    void handle_connect(uv_stream_t *client);
    void handle_post(httpReq* req, httpResp* resp);
    void handle_get(httpReq* req, httpResp* resp);
    size_t getSessionCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_sessions.size();
    }
    void handle_errReq(httpReq* req, httpResp* resp);

    /*
     * 关闭会话
     */
    void closeSession(SessionPtr &session, bool isUpgrade = false);
    void removeSession(SessionPtr &session);
};



