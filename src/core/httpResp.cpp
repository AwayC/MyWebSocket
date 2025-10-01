//
// Created by AWAY on 25-9-29.
//

#include "httpResp.h"

#include <cassert>


std::string httpStatus_str(httpStatus status)
{

#define STATUS_STR(status, str) case httpStatus::status: return str
    switch (status)
    {
        STATUS_STR(OK, "200 OK");
        STATUS_STR(CREATED, "201 Created");
        STATUS_STR(ACCEPTED, "202 Accepted");
        STATUS_STR(NO_CONTENT, "204 No Content");
        STATUS_STR(MOVED_PERMANENTLY, "301 Moved Permanently");
        STATUS_STR(FOUND, "302 Found");
        STATUS_STR(NOT_MODIFIED, "304 Not Modified");
        STATUS_STR(BAD_REQUEST, "400 Bad Request");
        STATUS_STR(UNAUTHORIZED, "401 Unauthorized");
        STATUS_STR(FORBIDDEN, "403 Forbidden");
        STATUS_STR(NOT_FOUND, "404 Not Found");
        STATUS_STR(INTERNAL_SERVER_ERROR, "500 Internal Server Error");
        STATUS_STR(NOT_IMPLEMENTED, "501 Not Implemented");
        STATUS_STR(BAD_GATEWAY, "502 Bad Gateway");
        STATUS_STR(SERVICE_UNAVAILABLE, "503 Service Unavailable");
        STATUS_STR(GATEWAY_TIMEOUT, "504 Gateway Timeout");
    default:
        return "Unknown Status";
    }

#undef STATUS_STR
}

void httpResp::sendFile(const std::string& path)
{

}

void httpResp::sendJson(lept_value& json)
{
    std::string jsonStr = json.stringify();
}

void httpResp::sendStr(const std::string& str)
{

}

void httpResp::setHeader(const std::string& key, const std::string& value)
{
    m_headers[key] = value;
}

void httpResp::setStatus(httpStatus status)
{
    m_status = status;
}

void sendBuffer()
{

}

/********* FileCxt *********/

FileReader::FileReader(FileClient *client)
{
    m_client = client;
    m_bufferVec.push_back(uv_buf_init(new char[DEFAULT_FILE_READER_BUFF_SIZE],
        DEFAULT_FILE_READER_BUFF_SIZE));
    m_buffUsed = 0;

}

FileReader::~FileReader()
{
    for (auto &buf: m_bufferVec)
    {
        delete [] buf.base;
    }

    m_bufferVec.clear();
}


void FileReader::fileRead(std::string path)
{
    m_open_req.data = this;
    m_read_req.data = this;
    m_close_req.data = this;

    m_path = path;
    m_buffUsed = 0;

    uv_fs_open(this->m_client->getLoop(),
                &m_open_req,
                m_path.c_str(),
                O_RDONLY, 0, onOpen);
}

void FileReader::onOpen(uv_fs_t* req)
{
    FileReader* self = static_cast<FileReader*>(req->data);
    assert(self != nullptr);
    self->m_result = req->result;

    self->m_client->onOpen(self);

    if (req->result < 0)
    {
        std::cerr << "open file failed" << std::endl;
    } else
    {
        std::cout << "file open success" << std::endl;
        if (self->m_bufferVec.size() == 0)
        {
            self->m_bufferVec.push_back(uv_buf_t(new char[DEFAULT_FILE_READER_BUFF_SIZE],
                DEFAULT_FILE_READER_BUFF_SIZE));
        }
        self->m_buffUsed = 0;
        uv_fs_read(self->m_client->getLoop(),
                    &self->m_read_req,
                    req->result, self->m_bufferVec.data(),
                    1, -1,
                    FileReader::onRead);
    }

    uv_fs_req_cleanup(req);
}

void FileReader::onClose(uv_fs_t* req)
{
    std::cerr << "file close success" << std::endl;
    uv_fs_req_cleanup(req);
    FileReader* self = static_cast<FileReader*>(req->data);
    assert(self != nullptr);
    uv_fs_req_cleanup(&self->m_read_req);
    self->m_client->onClose(self);
}

void FileReader::onRead(uv_fs_t* req)
{
    FileReader* self = static_cast<FileReader*>(req->data);
    assert(self != nullptr);
    self->m_result = req->result;
    if (req->result < 0)
    {
        std::cerr << "read file fail" << std::endl;
        uv_fs_req_cleanup(req);
        uv_fs_req_cleanup(&self->m_close_req);
        uv_fs_req_cleanup(&self->m_read_req);

        self->m_client->onRead(self);
    } else
    {
        if (req->result == 0)
        {
            std::cout << "file read end" << std::endl;

            self->m_client->onRead(self);

            uv_fs_close(self->m_client->getLoop(),
                        &self->m_read_req,
                        req->result,
                        FileReader::onClose);
            uv_fs_req_cleanup(req);
        } else
        {
            std::cout << "file read " << req->result << " bytes" << std::endl;
            self->m_buffUsed ++;
            if (self->m_buffUsed == self->m_bufferVec.size())
            {
                self->m_bufferVec.push_back(uv_buf_t(new char[DEFAULT_FILE_READER_BUFF_SIZE],
                    DEFAULT_FILE_READER_BUFF_SIZE));
            }
            uv_fs_read(self->m_client->getLoop(),
                        &self->m_read_req,
                        req->result, self->m_bufferVec.data() + self->m_buffUsed,
                        1, -1,
                        FileReader::onRead);
        }
    }
}