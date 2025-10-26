//
// Created by AWAY on 25-10-19.
//

#include "httpRouter.h"
#include <iostream>
#include <utility>

void httpRouter::addRoute(http_method method,
            const std::string& pattern_template,
            RouterHandler handler)
{
    try
    {
        std::string regex_str = parse_path_to_regex(pattern_template);
        //todo
        std::regex pattern(regex_str, std::regex::optimize);
        routes[method].push_back({pattern, std::move(handler)});
    } catch (const std::regex_error& e) {
        std::cerr << "regex error for path '" << pattern_template << "' : " << e.what() << std::endl;
    }
}

RouterHandler httpRouter::dispatch(httpReq* req)
{
    http_method method = req->method;
    size_t pos = req->url.find('?');
    std::string path = pos == std::string::npos ? req->url : req->url.substr(0, pos);

    auto it = routes.find(method);
    if (it == routes.end())
    {
        std::cerr << "no route found for method '" << http_method_str(method) << "' and path '" << path << "'" << std::endl;
        return nullptr;
    }

    const auto& method_routes = it->second;
    std::smatch match_results;

    for (const auto& route : method_routes)
    {
        if (std::regex_match(path, match_results, route.pattern))
        {
            req->params = match_results;
            return route.handler;
        }
    }

    req->params = std::smatch();
    std::cerr << "404 Not Found" << std::endl;
    return nullptr;
}

std::string httpRouter::parse_path_to_regex(const std::string& path_template)
{
    std::string  regex_str = path_template;
    /**
     * 替换路径参数为正则表达式
     * "/users/:id"	        =>   "^/users/([^/]+)$"
     * "/products/:id(\d+)"	=>   "^/products/(\d+)$"
     * "/"	                =>   "^/$"
     */

    std::regex param_with_regex_re(R"(:(\w+)\((.+?)\))");
    regex_str = std::regex_replace(regex_str, param_with_regex_re, "($2)");

    std::regex param_re(R"(:(\w+))");
    regex_str = std::regex_replace(regex_str, param_re, "([^/]+)");
    return "^" + regex_str + "$";
}