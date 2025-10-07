//
// Created by AWAY on 25-10-5.
//
#pragma once

#include <uv.h>
#include <iostream>
#include <functional>

#define DEFAULT_FILE_READER_BUFF_SIZE 1024

class FileReader
{
public:
    void fileRead(std::string path);
    int getResult() const
    {
        return m_result;
    }

    /*
     * 事件回调
     */
    void onOpen(std::function<void(FileReader*)> cb);
    void onClose(std::function<void(FileReader*)> cb);
    void onRead(std::function<void(FileReader*)> cb);

    size_t getBuff(uv_buf_t* buff);
    size_t getReadByte();

    void appendToBuff(std::vector<uv_buf_t>& buff);

    FileReader(uv_loop_t* loop);
    ~FileReader();


private:
    uv_fs_t m_open_req;
    uv_fs_t m_read_req;
    uv_fs_t m_close_req;

    /*
     * 缓冲区矢量
     */
    std::vector<uv_buf_t> m_bufferVec;
    int m_buffUsed;

    /*
     * 结果状态
     */
    int m_result;

    /*
     * 文件描述符
     */
    int m_fd;
    std::string m_path;
    uv_loop_t* m_loop;
    size_t m_readByte;


    std::function<void(FileReader*)> m_onOpen;
    std::function<void(FileReader*)> m_onRead;
    std::function<void(FileReader*)> m_onClose;

    /*
     * uv事件回调
     */
    static void onOpen(uv_fs_t *req);
    static void onRead(uv_fs_t *req);
    static void onClose(uv_fs_t *req);
};