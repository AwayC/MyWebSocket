// my_app/main.c
#include <stdio.h>
#include <uv.h> // 编译器需要知道去哪里找这个文件

#include "src/include/httpserver.h"
#include <string>
#include <iostream>


int main() {
    httpserver svr("127.0.0.1");
    std::cout << "create http server" << std::endl;
    svr.onConnect([](uv_tcp_t* client)
    {
        std::cout << "onConnect" << std::endl;
    });
    svr.onRequest([](httpReq* req, httpResp* resp)
    {
        std::cout << "onRequest" << std::endl;
        std::cout << "method " << http_method_str(req->_method) << std::endl;
        std::cout << "version " << req->_version << std::endl;
        std::cout << "onRequest url: " << req->_url << std::endl;
        std::cout << "onRequest body: " << req->_body << std::endl;

    });
    svr.start();

    svr.~httpserver();
    return 0;
}
