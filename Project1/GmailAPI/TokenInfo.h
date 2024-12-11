#pragma once
#include "..\Libs\Header.h"

struct TokenInfo {
    string access_token;
    string refresh_token;
    time_t created_at;
    time_t expires_at;
    int expires_in;

    Json::Value toJson() const {
        Json::Value json;
        json["access_token"] = access_token;
        json["refresh_token"] = refresh_token;
        json["created_at"] = (Json::Int64)created_at;
        json["expires_in"] = expires_in;
        return json;
    }

    static TokenInfo fromJson(const Json::Value& json) {
        TokenInfo token;
        token.access_token = json["access_token"].asString();
        token.refresh_token = json["refresh_token"].asString();
        token.created_at = json["created_at"].asInt64();
        token.expires_in = json["expires_in"].asInt();
        return token;
    }
};