#pragma once
#include "..\Libs\Header.h"
#include "..\GmailAPI\TokenInfo.h"

class HttpClient {
public:
    virtual ~HttpClient() {}
    virtual void makeRequest(const string& url, const string& postFields) = 0;
    virtual void authenticate(const string& authCode) = 0;
    virtual void refreshToken() = 0;

    // Hàm cơ sở
    string performRequest(const string& url, const string& postFields = "", 
        const vector<string>& headers = {}, const string& method = "GET");
    string sendRequest(const string& url, const string& postFields, const string& method) {
        return performRequest(url, postFields, {}, method);
    }

};
