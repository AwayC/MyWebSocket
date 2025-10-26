// my_app/main.c
#include <cstdio>
#include <uv.h>

#include "httpserver.h"
#include <string>
#include <iostream>
#include "leptjson.h"
#include "leptjson.h"


int main() {
    auto svr = HttpServer::create("127.0.0.1");
    std::cout << "create http server" << std::endl;
    svr->onConnect([](uv_tcp_t* client)
    {
        std::cout << "onConnect" << std::endl;
    });
    svr->get("/", [](httpReq* req, httpRespPtr resp)
    {
        static int cnt = 0;
        std::cout << "onRequest" << std::endl;
        std::cout << "method " << http_method_str(req->method) << std::endl;
        std::cout << "version " << req->version << std::endl;
        std::cout << "onRequest url: " << req->url << std::endl;
        std::cout << "onRequest body: " << req->body << std::endl;

        switch (cnt)
        {
        case 2:
            resp->sendStr("hello world");
            break;
        case 1:
            {
                lept_value json = {
                    {"port", 8080},
                    {"ip", "127.0.0.1"},
                    {"author", "away"},
                };
                resp->sendJson(json);
            }
            break;
        case 0:
            resp->sendFile("./index.html");
            break;
        default:
            resp->sendStr("hello world");
            break;
        }

        cnt ++;
    });

    svr->get("/id/:id", [](httpReq* req, httpRespPtr resp)
    {
        std::cout << req->params["id"] << std::endl;
        for (auto& param : req->query())
        {
            std::cout << param.first << " " << param.second << std::endl;
        }
        resp->sendStr("connect");
    });
    svr->start();

    return 0;
}
