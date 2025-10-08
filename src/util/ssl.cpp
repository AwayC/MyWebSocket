//
// Created by AWAY on 25-10-8.
//

#include "ssl.h"
#include <stdexcept>

namespace WsUtil {

std::string base64_encode(const unsigned char* input, int length) {
    if (length <= 0 || !input) {
        return "";
    }

    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *mem = BIO_new(BIO_s_mem());

    b64 = BIO_push(b64, mem);

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    if (BIO_write(b64, input, length) <= 0) {
        BIO_free_all(b64);
        throw std::runtime_error("BIO_write failed during Base64 encoding.");
    }
    BIO_flush(b64);

    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);

    std::string result(static_cast<const char*>(bptr->data), bptr->length);

    BIO_free_all(b64);

    return result;
}

std::string calculate_accept_key(const std::string& client_key) {
    std::string handshake_string = client_key + WS_GUID;

    unsigned char sha1_result[SHA_DIGEST_LENGTH];

    SHA1(
        reinterpret_cast<const unsigned char*>(handshake_string.c_str()),
        handshake_string.length(),
        sha1_result
    );

    return base64_encode(sha1_result, SHA_DIGEST_LENGTH);
}

} // namespace WsUtil
