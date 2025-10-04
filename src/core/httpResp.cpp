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

/********** httpResp *********/
httpResp::httpResp(uv_stream_t* client) : m_client(client), m_reader(client->loop)
{
    m_status = httpStatus::OK;
    m_sendType = SendType::NONE;
    m_reader.data = this;
    m_reader.onOpen([](FileReader* reader)
    {
        if (reader->getResult() < 0)
        {
            std::cerr << "send file error" << std::endl;
        }
    });
    m_reader.onRead([](FileReader* reader)
    {
        if (reader->getResult() < 0)
        {
            std::cerr << "send file error" << std::endl;
        }
    });
    m_reader.onClose([](FileReader* reader)
    {
        if (reader->getResult() < 0)
        {
            std::cerr << "send file error" << std::endl;
        }

        httpResp* resp = static_cast<httpResp*>(reader->data);
        resp->send();
    });
}

httpResp::~httpResp()
{


}

void httpResp::sendFile(const std::string& path)
{
    m_headers["Content-Type"] = "application/octet-stream";
    m_filePath = path;
    m_sendType = SendType::FILE;
}

void httpResp::sendJson(lept_value& json)
{
    m_body = json.stringify();
    m_headers["Content-Type"] = "application/json";
    m_sendType = SendType::JSON;
}

void httpResp::sendStr(const std::string& str)
{
    m_body = str;
    m_headers["Content-Type"] = "text/plain";
    m_sendType = SendType::STR;
}

void httpResp::setHeader(const std::string& key, const std::string& value)
{
    m_headers[key] = value;
}

void httpResp::setStatus(httpStatus status)
{
    m_status = status;
}

void httpResp::send()
{
    m_head = "HTTP/1.1 " + httpStatus_str(m_status) + "\r\n";
    for (auto header : m_headers)
    {
        m_head.append(header.first + ": " + header.second + "\r\n");
    }

    if (SendType::FILE == m_sendType)
        m_head.append("Content-Length: " + std::to_string(m_reader.getReadByte()) + "\r\n");
    else
        m_head.append("Content-Length: " + std::to_string(m_body.size()) + "\r\n");
    m_head.append("\r\n");

    m_buffers.clear();
    m_buffers.push_back(uv_buf_init(const_cast<char*>(m_head.c_str()), m_head.size()));

    auto req = new uv_write_t;
    req->data = this;

    if (SendType::FILE == m_sendType)
    {
        m_reader.appendToBuff(m_buffers);
    } else
    {
        m_buffers.push_back(uv_buf_init(const_cast<char*>(m_body.c_str()), m_body.size()));
    }

    uv_write(req, m_client,
            m_buffers.data(),
            m_buffers.size(),
            onWriteEnd);
}

void httpResp::onWriteEnd(uv_write_t *req, int status)
{
    if (status < 0)
    {
        std::cerr << "write error" << std::endl;
    } else
    {
        std::cout << "write success" << std::endl;
    }

    httpResp* resp = static_cast<httpResp*>(req->data);
    if (resp->m_onComplete)
        resp->m_onComplete(resp);
}

void httpResp::onCompleteAndSend(std::function<void(httpResp*)> cb)
{
    m_onComplete = cb;

    if (SendType::FILE == m_sendType)
        m_reader.fileRead(m_filePath);
    else
        send();
}

void httpResp::setData(void* data)
{
    this->data = data;
}

void* httpResp::getData()
{
    return data;
}

void httpResp::clearContent()
{
    m_body.clear();
    m_headers.clear();
    m_status = httpStatus::OK;
    m_sendType = SendType::NONE;
    m_filePath.clear();
}

/********* FileReader *********/

size_t FileReader::getBuff(uv_buf_t* buff)
{
    buff = m_bufferVec.data();
    return m_buffUsed;
}

size_t FileReader::getReadByte()
{
    return m_readByte;
}

void FileReader::appendToBuff(std::vector<uv_buf_t>& buff)
{
    for (size_t i = 0;i < m_buffUsed;i ++)
    {
        buff.push_back(m_bufferVec[i]);
    }
}

FileReader::FileReader(uv_loop_t* loop) : m_loop(loop)
{
    m_bufferVec.push_back(uv_buf_init(new char[DEFAULT_FILE_READER_BUFF_SIZE],
        DEFAULT_FILE_READER_BUFF_SIZE));
    m_buffUsed = 0;
    m_readByte = 0;
    data = nullptr;
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

    m_path = std::move(path);
    m_buffUsed = 0;
    m_readByte = 0;

    uv_fs_open(m_loop,
                &m_open_req,
                m_path.c_str(),
                O_RDONLY, 0, onOpen);
}

void FileReader::onOpen(uv_fs_t* req)
{
    FileReader* self = static_cast<FileReader*>(req->data);
    assert(self != nullptr);
    self->m_result = req->result;

    if (self->m_onOpen)
        self->m_onOpen(self);

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
        self->m_fd = req->result;
        uv_fs_read(self->m_loop,
                    &self->m_read_req,
                    self->m_fd, self->m_bufferVec.data(),
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
    if (self->m_onClose)
        self->m_onClose(self);
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

        if (self->m_onRead)
            self->m_onRead(self);
    } else
    {
        if (req->result == 0)
        {
            std::cout << "file read end" << std::endl;
            if (self->m_onRead)
                self->m_onRead(self);

            uv_fs_close(self->m_loop,
                        &self->m_read_req,
                        req->result,
                        FileReader::onClose);
            uv_fs_req_cleanup(req);
        } else
        {
            std::cout << "file read " << req->result << " bytes" << std::endl;
            self->m_buffUsed ++;
            self->m_readByte += req->result;
            if (self->m_buffUsed == self->m_bufferVec.size())
            {
                self->m_bufferVec.push_back(uv_buf_t(new char[DEFAULT_FILE_READER_BUFF_SIZE],
                    DEFAULT_FILE_READER_BUFF_SIZE));
            }
            uv_fs_read(self->m_loop,
                        &self->m_read_req,
                        self->m_fd, self->m_bufferVec.data() + self->m_buffUsed,
                        1, -1,
                        FileReader::onRead);
        }
    }
}

void FileReader::onOpen(std::function<void(FileReader*)> cb)
{
        m_onOpen = cb;
}

void FileReader::onClose(std::function<void(FileReader*)> cb)
{
        m_onClose = cb;
}

void FileReader::onRead(std::function<void(FileReader*)> cb)
{
        m_onRead = cb;
}