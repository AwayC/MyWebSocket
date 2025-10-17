//
// Created by AWAY on 25-10-11.
//

#include "websocket_parser.h"

#include <cassert>

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

#define SWITCH_TO_MASK(frame) do {\
    if (mask) \
    { \
        m_status = Status::maskKey; \
        m_byteNeed = 4; \
        frame->mask_key.clear(); \
    } else \
    { \
        m_status = Status::payload; \
        m_byteNeed = frame->payload_len; \
    } \
    }while (0)


#define GET_FIN(ch)             ((ch) & 0x80)
#define GET_RSV(ch)             (((ch) & 0x40) >> 4)
#define GET_OPCODE(ch)          ((ch) & 0x0F)
#define GET_MASK(ch)            ((ch) & 0x80)
#define GET_PAYLOAD_LEN(ch)     ((ch) & 0x7F)

WsParseErr websocket_parser::parse(const uv_buf_t& data, WsFrame* frame)
{
    const char* data_ = data.base;
    size_t len = data.len;
    m_frame = frame;
    const char* ch;
    bool mask = false;

    if (len == 0)
    {
        return WsParseErr::SUCCESS;
    }

    for (ch = data_; ch != data_ + len; ch ++)
    {
        switch (m_status)
        {
        case Status::finAndOpcode: {
            frame->fin = GET_FIN(*ch);
            frame->rsv = GET_RSV(*ch);
            int opcode = GET_OPCODE(*ch);
            if (!m_isContinuation && opcode)
            {
                frame->opcode = opcode;
                //reset
                frame->mask_key.clear();
                frame->payload.clear();
            }

            m_status = Status::payloadLenAndMask;
            break ;
        }

        case Status::payloadLenAndMask:
        {
            frame->mask = mask = GET_MASK(*ch);
            frame->payload_len = GET_PAYLOAD_LEN(*ch);

            if (frame->payload_len <= 125)
            {
                SWITCH_TO_MASK(frame);
            } else if (frame->payload_len == 126)
            {
                frame->payload_len = 0;
                m_status = Status::payloadLen;
                m_byteNeed = 2;
            } else
            {
                frame->payload_len = 0;
                m_status = Status::payloadLen;
                m_byteNeed = 8;
            }
            break ;
        }

        case Status::payloadLen:
        {
            if (m_byteNeed)
            {
                frame->payload_len = (frame->payload_len << 1) + (*ch);
                m_byteNeed --;
            }
            if (!m_byteNeed)
            {
                SWITCH_TO_MASK(frame);
            }
            break ;

            case Status::maskKey:
                if (m_byteNeed)
                {
                    frame->mask_key.push_back(*ch);
                    m_byteNeed --;
                }
            if (!m_byteNeed)
            {
                m_status = Status::maskKey;
            }
            break ;
        }

        case Status::payload:
        {
            size_t to_read = std::min(static_cast<size_t>(data_ + len - ch),
                                    frame->payload_len);
            if (mask)
            {
                for (size_t i = 0;i < to_read;i ++)
                {
                    frame->payload.emplace_back((*(ch + i)) ^ frame->mask_key[m_maskIndex]);
                    m_maskIndex = (m_maskIndex + 1) % 4;
                }
            } else
            {
                frame->payload.insert(frame->payload.end(),
                                        ch,
                                        ch + to_read);
            }

            m_byteNeed -= to_read;
            ch += to_read - 1;

            if (!m_byteNeed)
            {
                m_status = Status::complete;
                ch --;
            }
            break ;
        }

        case Status::complete:
        {
            m_maskIndex = 0;
            m_byteNeed = 0;
            m_status = Status::finAndOpcode;

            if (frame->fin)
            {
                m_isContinuation = false;
                assert(m_onComplete != nullptr);
                m_onComplete();
            } else
            {
                m_isContinuation = true;
            }
            break ;
        }

        }
    }

    return WsParseErr::SUCCESS;
}


websocket_parser::websocket_parser()
{
    m_status = Status::finAndOpcode;
    m_byteNeed = 1;
    m_maskIndex = 0;
}

websocket_parser::~websocket_parser()
{

}

