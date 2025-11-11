//
// Created by AWAY on 25-11-10.
//

#pragma once
#include <jwt-cpp/jwt.h>
#include "leptjson.h"
#include <iostream>

class JwtUtil {
public:

    static std::string sign(const lept_value& payload,
                            const std::string& key,
                            int expires);
    static lept_value verify(const std::string& token,
                        const std::string& key,
                        int maxAge);
private:

    static picojson::value transform(const lept_value& value);
};