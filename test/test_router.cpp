//
// Created by AWAY on 25-10-19.
//

#include "httpserver.h"
#include "../src/core/httpRouter.h"

int main()
{
    std::string url = "/test/123/address/123456?name=away&age=18&sex=male";

    auto router = httpRouter();

    std::string path = url.substr(0, url.find('?'));
    std::cout << "path: " << path << std::endl;
    router.addRoute(HTTP_GET, "/test/:test/address/:address", [](httpReq* req, httpRespPtr resp)
    {
        std::cout << "params size: " << req->params.size() << std::endl;
        for (auto it : req->params)
        {
            std::cout << it.second << std::endl;
        }
        std::cout << "params test: " << req->params["test"] << std::endl;

        for (auto it : req->query())
        {
            std::cout << it.second << std::endl;
        }
    });

    httpReq req;
    req.method = HTTP_GET;
    req.url = url;
    req.body = "";
    req.is_queryParams_parsed = false;

    auto handler = router.dispatch(&req);
    if (handler)
    {
        handler(&req, nullptr);
    }

}