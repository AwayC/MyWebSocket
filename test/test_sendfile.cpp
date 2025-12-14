//
// Created by AWAY on 25-12-14.
//

#include "MyWebSocket.h"

int main()
{
    auto svr = HttpServer::create("127.0.0.1");
    svr->get("/(.*)", [](httpReq* req, httpRespPtr resp)
    {
        if (req->url != "/")
        {
            resp->setStatus(httpStatus::NOT_FOUND);
            resp->sendStr("404 Not Found");
            return;
        }
        std::cout << "send file index.html" << std::endl;
        resp->sendFile("./index.html");
    });


    svr->start();
    return 0;
}