//
// Created by AWAY on 25-11-10.
//

#include "websocket.h"
#include <iostream>
#include <thread>

const std::string JWT_KEY = "secret";

void test()
{
    auto token = JwtUtil::sign({{"uid", 123}, {"username", "away"}}, JWT_KEY, 2);
    std::cout << "generated token: " << token << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    lept_value payload;
    try
    {
        payload = JwtUtil::verify(token, JWT_KEY, 1);
        std::cout << "verify token passed" << std::endl;
        std::cout << "payload: " << payload.stringify() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "verify token failed: " << e.what() << std::endl;
    }
}

int main() {
    test();
}