// my_app/main.c
#include <cstdio>
#include <uv.h>

#include "httpserver.h"
#include <string>
#include <iostream>


int main() {
    HttpServer svr("127.0.0.1");
    std::cout << "create http server" << std::endl;
    svr.onConnect([](uv_tcp_t* client)
    {
        std::cout << "onConnect" << std::endl;
    });
    svr.onRequest([](httpReq* req, httpResp* resp)
    {
        std::cout << "onRequest" << std::endl;
        std::cout << "method " << http_method_str(req->method) << std::endl;
        std::cout << "version " << req->version << std::endl;
        std::cout << "onRequest url: " << req->url << std::endl;
        std::cout << "onRequest body: " << req->body << std::endl;

    });
    svr.start();

    return 0;
}
