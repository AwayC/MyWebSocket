//
// Created by AWAY on 25-10-20.
//

#pragma once

#include <cstring>
#include <iostream>
#include <http_parser.h>
#include <regex>
#include <unordered_map>


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
    std::smatch params;
    bool is_queryParams_parsed;

    httpReq() : is_queryParams_parsed(false) {};
    std::string query(const std::string& key);

private:
    HttpParamMap queryParams;


    void urlDecode(std::string_view str);
};


