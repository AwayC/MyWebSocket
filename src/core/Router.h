//
// Created by AWAY on 25-10-19.
//

#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <regex> // 正则表达式
#include <vector>


using RouterHandler = std::function<void(const std::smatch&)>;

struct Route {
    std::regex pattern;
    RouterHandler handler;
};

class Router
{
public:
    void addRoute(const std::string& method,
                const std::string& pattern_str,
                RouterHandler handler);

    void dispatch(const std::string& method,
                const std::string& path);

private:
    // 解析路径模板为正则表达式
    std::string parse_path_to_regex(const std::string& path_template);

    /*
     * 路由表
     * key: method, "GET", "POST", etc.
     * value: vector of Route structs
     */
    std::unordered_map<std::string, std::vector<Route>> routes;
};