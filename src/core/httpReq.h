//
// Created by AWAY on 25-10-20.
//

#pragma once

#include <cstring>
#include <iostream>
#include <http_parser.h>
#include <regex>
#include <unordered_map>
#include "uv.h"


using HttpParamMap = std::unordered_map<std::string, std::string>;
using HeaderMap = std::unordered_map<std::string, std::string>;

class httpReq
{
public:
    std::string url ;
    HeaderMap headers;
    std::string body;
    http_method method;

    std::string version;
    HttpParamMap params;
    bool is_queryParams_parsed;
    std::string ip;
    int port;

    httpReq() : is_queryParams_parsed(false) {};
    const HttpParamMap& query();
    void set_ip(uv_tcp_t* client);

private:
    HttpParamMap queryParams;

    void urlDecode(std::string_view str);
};


