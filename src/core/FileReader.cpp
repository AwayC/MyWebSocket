//
// Created by AWAY on 25-10-5.
//

#include "FileReader.h"
#include <cassert>

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
    std::cout << "file close success" << std::endl;
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