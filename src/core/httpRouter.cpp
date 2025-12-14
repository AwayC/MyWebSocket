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
        auto [regex_str, param_names] = parse_path_to_regex(pattern_template);

        std::regex pattern(regex_str, std::regex::optimize);
        routes[method].push_back({pattern, std::move(handler), param_names});
    } catch (const std::regex_error& e) {
        std::cerr << "regex error for path '" << pattern_template << "' : " << e.what() << std::endl;
    }
}

httpRouter::RouterHandler httpRouter::dispatch(httpReq* req)
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
            size_t param_len = std::min(match_results.size(), route.paramNames.size());
            for (size_t i = 1;i < param_len;i++)
            {
                req->params[route.paramNames[i - 1]] = match_results[i].str();
            }
            return route.handler;
        }
    }

    req->params.clear();
    std::cerr << "404 Not Found" << std::endl;
    return nullptr;
}

std::pair<std::string, std::vector<std::string>>
httpRouter::parse_path_to_regex(const std::string& path_template)
{
    std::string regex_str = path_template;
    std::vector<std::string> param_names;


    // 提取参数名
    std::regex param_name_extract_re(R"(:(\w+))");

    auto names_begin = std::sregex_iterator(path_template.begin(), path_template.end(), param_name_extract_re);
    auto names_end = std::sregex_iterator();

    for (std::sregex_iterator i = names_begin; i != names_end; ++i) {
        const std::smatch& match = *i;
        param_names.push_back(match[1].str());
    }

    /**
     * 替换路径参数为正则表达式
     * "/users/:id" => "^/users/([^/]+)$"
     * "/products/:id(\d+)" => "^/products/(\d+)$"
     * "/" => "^/$"
     */
    std::regex param_with_regex_re(R"(:(\w+)\((.+?)\))");
    regex_str = std::regex_replace(regex_str, param_with_regex_re, "($2)");

    std::regex param_re(R"(:(\w+))");
    regex_str = std::regex_replace(regex_str, param_re, "([^/]+)");


    return {"^" + regex_str + "$", param_names};
}