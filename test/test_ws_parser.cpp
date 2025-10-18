//
// Created by AWAY on 25-10-18.
//


#include "WebSocket.h"
#include <iostream>

int main() {

    websocket_parser parser;
    WsFrame frame;
    parser.onComplete([&]()
    {
        std::cout << "onComplete" << std::endl;
        std::string_view content = std::string_view(frame.payload.data(), frame.payload.size());
        std::cout << content << std::endl;
    });
    /**
     * fin: 1, opcode: 0x01,
     * mask: 1, payload_len: 5
     * mask_key: 0x00000000
     * payload: "Hello"
     */
    std::string buf;
    buf.push_back(0x81);
    buf.push_back(0x85);
    buf.push_back(0x00);
    buf.push_back(0x00);
    buf.push_back(0x00);
    buf.push_back(0x00);
    buf.append("Hello");

    uv_buf_t uvBuf = uv_buf_init(buf.data(), buf.size());
    WsParseErr err = parser.parse(uvBuf, buf.size(), &frame);

    std::cout << "fin: " << (int)frame.fin << std::endl;
    std::cout << "opcode: " << (int)frame.opcode << std::endl;
    std::cout << "mask: " << (int)frame.mask << std::endl;
    std::cout << "rsv: " << (int)frame.rsv << std::endl;
    std::cout << "payload_len: " << frame.payload_len << std::endl;

    for (auto it : frame.mask_key)
    {
        std::cout << (int)it << " " ;
    }
    std::cout << std::endl;
    std::string_view content = std::string_view(frame.payload.data(), frame.payload.size());
    std::cout << "payload: " << content << std::endl;

}