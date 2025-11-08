//
// Created by AWAY on 25-10-20.
//

#include "httpReq.h"

/************* httpReq *************/
void httpReq::set_ip(uv_tcp_t* client) {
    struct sockaddr_storage addr;
    int len = sizeof(addr);
    int r = uv_tcp_getpeername(client, (struct sockaddr*)&addr, &len);

    ip.clear();
    port = 0;

    if (r == 0) {
        char ip_[INET6_ADDRSTRLEN];
        int port_ = 0;

        if (addr.ss_family == AF_INET) {
            struct sockaddr_in* s = (struct sockaddr_in*)&addr;
            uv_ip4_name(s, ip_, sizeof(ip_));
            port_ = ntohs(s->sin_port);
        } else if (addr.ss_family == AF_INET6) {
            struct sockaddr_in6* s = (struct sockaddr_in6*)&addr;
            uv_ip6_name(s, ip_, sizeof(ip_));
            port_ = ntohs(s->sin6_port);
        }

        ip = ip_;
        port = port_;
    }
}


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

