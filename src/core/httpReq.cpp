//
// Created by AWAY on 25-10-20.
//

#include "httpReq.h"

/************* httpReq *************/
const HttpParamMap& httpReq::query()
{
    if (!is_queryParams_parsed)
    {
        queryParams.clear();
        urlDecode(url);
        is_queryParams_parsed = true;
    }

   return queryParams;
}

void httpReq::urlDecode(std::string_view str) {
    // /**/**?val=xxx&val=xxx
    //find '?'
    size_t pos = str.find('?');
    pos = pos == std::string_view::npos ? str.size() : pos + 1;

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
        size_t eq = pair_str.find('=', 0);
        std::string key;
        std::string value;

        if (eq == std::string::npos)
        {
            key = pair_str;
            value = "";
        } else {
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

