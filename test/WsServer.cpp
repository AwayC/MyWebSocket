//
// Created by AWAY on 25-10-8.
//
#include "httpserver.h"
#include "WebSocket.h"
#include "WsServer.h"


int main()
{
    auto httpSvr = std::make_shared<HttpServer>("127.0.0.1", 8080);
    WsServer svr(httpSvr);

    svr.onConnect([](WsSessionPtr session)
    {
        std::cout << "websocket session connected" << std::endl;
        session->onMessage([](WsSessionPtr ws)
        {
            std::string msg = ws->getStrMessage();
            std::cout << msg << std::endl;

            std::cout << "websocket session message received" << std::endl;
            ws->send(msg);
        });
    });


    httpSvr->start();
    return 0;
}
