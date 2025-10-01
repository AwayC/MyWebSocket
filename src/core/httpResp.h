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

class FileReader;

class FileClient
{
public:
    virtual uv_loop_t* getLoop() = 0;
    virtual void onOpen(FileReader* reader) = 0;
    virtual void onRead(FileReader* reader) = 0;
    virtual void onWrite(FileReader* reader) = 0;
    virtual void onClose(FileReader* reader) = 0;

    FileClient() = delete;
    ~FileClient() = delete;
};

class FileReader
{
    uv_fs_t m_open_req;
    uv_fs_t m_read_req;
    uv_fs_t m_close_req;
    std::vector<uv_buf_t> m_bufferVec;
    int m_buffUsed;
    FileClient *m_client;
    int m_result;
    std::string m_path;


    void fileRead(std::string path);
    int getResult()
    {
        return m_result;
    }
    FileReader(FileClient *client);
    ~FileReader()();
private:
    static void onOpen(uv_fs_t *req);
    static void onRead(uv_fs_t *req);
    static void onClose(uv_fs_t *req);
};

class httpResp : public FileClient {
public:
    httpResp(uv_stream_t *client);
    ~httpResp();
    void sendStr(const std::string& str);
    void sendJson(lept_value& json);
    void sendFile(const std::string& path);

    void setHeader(const std::string& key, const std::string& value);
    void setStatus(httpStatus status);

    void onOpen(FileReader* reader);
    void onRead(FileReader* reader);
    void onWrite(FileReader* reader);

private:
    std::string m_body;
    httpStatus m_status;
    headerMap m_headers;
    std::string m_version;
    int m_content_length;
    uv_stream_t *m_client;
    std::string m_url;

    FileReader m_reader;

    void addToBuffer(char* data, size_t size);
    void sendBuffer();


};


