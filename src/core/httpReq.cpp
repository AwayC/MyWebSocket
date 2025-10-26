//
// Created by AWAY on 25-10-20.
//

#include "httpReq.h"

/************* httpReq *************/
std::string httpReq::query(const std::string& key)
{
    if (!is_queryParams_parsed)
    {
        queryParams.clear();
        urlDecode(url);
        is_queryParams_parsed = true;
    }

    auto it = queryParams.find(key);
    if (it != queryParams.end())
    {
        return it->second;
    }

    return "";
}

void httpReq::urlDecode(std::string_view str) {
    // /**/**?val=xxx&val=xxx
    //find '?'
    size_t pos = str.find('?');
    pos = pos == std::string_view::npos ? 0 : str.size();

    while (pos < str.size())
    {
        //find '&'
        size_t end = str.find('&', pos);
        if (end == std::string::npos)
        {
            end = str.size();
        }

        std::string_view pair_str = str.substr(pos, end - pos);

        // find '='
        size_t eq = str.find('=', pos);
        std::string key;
        std::string value;

        if (eq == std::string::npos)
        {
            key = pair_str;
            value = "";
        } else
        {
            key = pair_str.substr(0, eq);
            value = pair_str.substr(eq + 1, end - pos - eq - 1);
        }

        if (!key.empty())
        {
            queryParams[key] = value;
        }

        pos = end + 1;

    }
}

