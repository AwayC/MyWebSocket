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

#define DEFAULT_FILE_READER_BUFF_SIZE 1024

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


class FileReader
{
public:
    void fileRead(std::string path);
    int getResult()
    {
        return m_result;
    }

    void onOpen(std::function<void(FileReader*)> cb);
    void onClose(std::function<void(FileReader*)> cb);
    void onRead(std::function<void(FileReader*)> cb);

    size_t getBuff(uv_buf_t* buff);
    size_t getReadByte();

    void appendToBuff(std::vector<uv_buf_t>& buff);

    void *data;
    FileReader(uv_loop_t* loop);
    ~FileReader();


private:
    uv_fs_t m_open_req;
    uv_fs_t m_read_req;
    uv_fs_t m_close_req;
    std::vector<uv_buf_t> m_bufferVec;
    int m_buffUsed;

    int m_result;
    int m_fd;
    std::string m_path;
    uv_loop_t* m_loop;
    size_t m_readByte;


    std::function<void(FileReader*)> m_onOpen;
    std::function<void(FileReader*)> m_onRead;
    std::function<void(FileReader*)> m_onClose;

    static void onOpen(uv_fs_t *req);
    static void onRead(uv_fs_t *req);
    static void onClose(uv_fs_t *req);
};


class httpResp {
public:
    httpResp(uv_stream_t *client);
    ~httpResp();
    void sendStr(const std::string& str);
    void sendJson(lept_value& json);
    void sendFile(const std::string& path);

    void setHeader(const std::string& key, const std::string& value);
    void setStatus(httpStatus status);

    void onCompleteAndSend(std::function<void(httpResp*)> cb);
    void setData(void* data);
    void* getData();
    void clearContent();
private:
    std::string m_body;
    httpStatus m_status;
    headerMap m_headers;
    std::string m_version;
    int m_content_length;
    uv_stream_t *m_client;
    std::string m_url;
    std::string m_filePath;
    void* data;

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

    static void onWriteEnd(uv_write_t *req, int status);

    void send();


};


