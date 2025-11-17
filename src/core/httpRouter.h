//
// Created by AWAY on 25-10-19.
//

#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <regex> // 正则表达式
#include <vector>
#include "httpReq.h"
#include "httpResp.h"




class httpRouter
{
public:
    using RouterHandler = std::function<void(httpReq*, httpRespPtr)>;

    struct Route {
        std::regex pattern;
        RouterHandler handler;
        std::vector<std::string> paramNames;
    };

    void addRoute(http_method method,
                const std::string& pattern_str,
                RouterHandler handler);

    RouterHandler dispatch(httpReq* req);

private:
    // 解析路径模板为正则表达式
    static std::pair<std::string, std::vector<std::string>>
    parse_path_to_regex(const std::string& path_template);

    /*
     * 路由表
     * key: method, "GET", "POST", etc.
     * value: vector of Route structs
     */
    std::unordered_map<http_method, std::vector<Route>> routes;
};