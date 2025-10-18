//
// Created by AWAY on 25-10-11.
//

#pragma once
#include <string>
#include <cstring>
#include <iostream>
#include "uv.h"

/**
Frame format:

      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+
*/

enum WsCode
{
    WS_CONTINUATION = 0x0,
    WS_TEXT = 0x1,
    WS_BINARY = 0x2,
    WS_CLOSE = 0x8,
    WS_PING = 0x9,
    WS_PONG = 0xA,
};


enum class WsParseErr : uint8_t {
    SUCCESS,
    NOT_COMPLETED,
    INVALID_FRAME,
    INVALID_OPCODE,
    INVALID_PAYLOAD_LEN,
    INVALID_MASK,
};

std::string WsErr_Str(WsParseErr err);

using WsParseBuf = std::vector<char>;

using WsParseComplete = std::function<void()>;

struct WsFrame
{
    bool fin; // FIN 位

    /**
     * RSV 位 1, 2, 3
     * m_rsv = (r1 << 2) | (r2 << 1) | r3
     */
    uint8_t rsv{};
    uint8_t opcode{}; // 操作码
    bool mask{}; // MASK 位
    size_t payload_len{}; // 有效载荷长度


    std::vector<uint8_t> mask_key; // 掩码密钥
    WsParseBuf payload; // 解析缓冲区

};



class websocket_parser {
public:

    websocket_parser();
    ~websocket_parser();
    WsParseErr parse(const uv_buf_t& data, size_t len, WsFrame* frame);

    void onComplete(const WsParseComplete& cb)
    {
        m_onComplete = cb;
    }

private:

    enum class Status
    {
        finAndOpcode,
        payloadLenAndMask,
        payloadLen,
        maskKey,
        payload,
        complete,
    };

    Status m_status; // 当前解析状态
    size_t m_byteNeed; // 还需要多少字节
    WsFrame* m_frame;
    bool m_isContinuation; // 是否是继续帧
    size_t m_maskIndex; // 掩码索引

    WsParseComplete m_onComplete;

};

