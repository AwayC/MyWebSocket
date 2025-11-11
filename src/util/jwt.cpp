//
// Created by AWAY on 25-11-10.
//
#include "jwt.h"

std::string JwtUtil::sign(const lept_value& payload,
                             const std::string& key,
                             int expires_seconds) {
    auto now = std::chrono::system_clock::now();

    auto builder = jwt::create()
        .set_type("JWT")
        .set_issued_at(now);

    if (expires_seconds > 0)
    {
        auto expires_at = now + std::chrono::seconds(expires_seconds);
        builder.set_expires_at(expires_at);
    }

    const auto& payload_obj = payload.get_object();
    for (const auto& [key, value] : payload_obj)
    {
        builder.set_payload_claim(key, transform(value));
    }

    auto token = builder.sign(jwt::algorithm::hs256(key));
    return token;
}

void JwtUtil::verify(const std::string& token,
                        const std::string& key,
                        int maxAge_hours) {
    auto decoded_token = jwt::decode(token);

    auto verifier = jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256(key));

    verifier.verify(decoded_token);

    if (maxAge_hours < 0)
        return ;

    if (!decoded_token.has_payload_claim("iat"))
    {
        throw std::runtime_error("Token is missing 'iat' claim");
    }

    auto iat_claim = decoded_token.get_payload_claim("iat");

    auto iat_time_point = iat_claim.as_date();

    auto now = std::chrono::system_clock::now();

    auto max_age = std::chrono::hours(maxAge_hours);

    if (iat_time_point + max_age < now)
    {
        throw std::runtime_error("Token is expired");
    }
}

picojson::value JwtUtil::transform(const lept_value& v)
{
    switch (v.get_type())
    {
    case lept_type::null:
        return {};
    case lept_type::lfalse:
        return picojson::value(false);
    case lept_type::ltrue:
        return picojson::value(true);
    case lept_type::number:
        return picojson::value(v.get_number());
    case lept_type::integer:
        return picojson::value(v.get_integer());
    case lept_type::string:
        return picojson::value(v.get_string());
    case lept_type::array:
        {
            picojson::array arr;
            size_t size = v.get_array_size();
            for (size_t i = 0;i < size;i ++)
            {
                arr.push_back(transform(v.get_element(i)));
            }
            return picojson::value(std::move(arr));
        }
    case lept_type::object:
        {
            picojson::object obj;
            for (const auto& [key, value] : v.get_object())
            {
                obj[key] = transform(value);
            }

            return picojson::value(std::move(obj));
        }
    }
}
