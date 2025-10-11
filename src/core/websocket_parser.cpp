//
// Created by AWAY on 25-10-11.
//

#include "websocket_parser.h"

std::string WsErr_Str(WsParseErr err)
{
#define WsErr_Str(err) case WsParseErr::err : return #err

    switch (err)
    {
        WsErr_Str(SUCCESS);
        WsErr_Str(INVALID_FRAME);
        WsErr_Str(INVALID_OPCODE);
        WsErr_Str(INVALID_PAYLOAD_LEN);
        WsErr_Str(INVALID_MASK);
        default:
            return "UNKNOWN";
    }

#undef WsErr_Str
}


WsParseErr websocket_parser::parse(const uv_buf_t& frame)
{
    char *data = frame.base;
    int len = frame.len;

    // 检查帧是否为空
    if (len < 2)
    {
        return WsParseErr::INVALID_FRAME;
    }

    // FIN (bit 0)
    m_fin = data[0] & 0x80;
    // RSV (bit 1:3)
    m_rsv = (data[0] & 0x70) >> 4;  //0111 0000
    // OPCODE (bit 4:7)
    if (last_completed)
        m_opcode = (data[0] & 0x0F);

    // MASK (bit 0)
    m_mask = data[1] & 0x80;
    // PAYLOAD LEN (bit 1:7)
    if (m_parsed_len = parse_payload_len() < 0)
    {
        return WsParseErr::INVALID_PAYLOAD_LEN;
    }

    if (last_completed)
    {
        m_buf.clear();
    }
    // MASK KEY (4 bytes)
    if (m_mask)
    {
        if (len < m_parsed_len + 4)
            return WsParseErr::INVALID_MASK;
        for (int i = 0;i < 4;i ++)
        {
            m_mask_key[i] = data[m_parsed_len + i];
        }
        m_parsed_len += 4;
    }

    // PAYLOAD DATA
    if (len != m_parsed_len + m_payload_len)
        return WsParseErr::INVALID_PAYLOAD_LEN;

    // 对 PAYLOAD DATA 进行解密
    if (m_mask)
    {
        for (int i = 0;i < m_payload_len;i ++)
        {
            m_buf.emplace_back(data[m_parsed_len + i] ^ m_mask_key[i % 4]);
        }
    }
    else
    {
        m_buf.insert(m_buf.end(), data + m_parsed_len, data + m_parsed_len + m_payload_len);
    }


    if (m_fin == false)
    {
        last_completed = false;
        return WsParseErr::NOT_COMPLETED;
    }

    last_completed = true;
    return WsParseErr::SUCCESS;
}

int websocket_parser::parse_payload_len()
{
    char *data = m_frame.base;
    size_t len = m_frame.len;

    int payload_len = data[1] & 0x7F;
    m_parsed_len += 2;

    if (payload_len == 126)
    {
        if (len < 4)
            return -1;
        payload_len = (data[2] << 8) | data[3];

        m_parsed_len += 2;
    }
    else if (payload_len == 127)
    {
        if (len < 10)
            return -1;

        payload_len = 0;
        for (int i = 2;i < 2 + 8;i ++)
        {
            payload_len = payload_len << 8 | data[i];
        }

        m_parsed_len += 8;
    }

    return payload_len;
}

websocket_parser::websocket_parser(WsParseBuf& buf) : m_buf(buf)
{
    last_completed = true;
}

websocket_parser::~websocket_parser()
{

}

