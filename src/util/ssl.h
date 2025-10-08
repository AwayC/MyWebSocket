//
// Created by AWAY on 25-10-8.
//
#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

namespace WsUtil {

    // WebSocket GUID (Magic String)
    const std::string WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    /**
     * @brief 对数据进行 Base64 编码 (使用 OpenSSL)
     * @param input 要编码的原始数据指针
     * @param length 原始数据长度
     * @return Base64 编码后的字符串
     */
    std::string base64_encode(const unsigned char* input, int length);

    /**
     * @brief 计算 Sec-WebSocket-Accept Key
     * @param client_key 客户端发送的 Sec-WebSocket-Key
     * @return 计算出的 Sec-WebSocket-Accept 字符串
     */
    std::string calculate_accept_key(const std::string& client_key);

} // namespace WsUtil

