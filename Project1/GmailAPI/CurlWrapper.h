#pragma once
#include "..\Libs\Header.h"
#include "..\GmailAPI\TokenManager.h"


class CurlWrapper : public HttpClient {
public:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    const int MAX_RETRIES;
    CurlWrapper(int maxRetries = 3);

    // Sử dụng inheritance
    using HttpClient::performRequest;
    string performRequestWithRetry(const string& url, const string& method, const string& postFields,
        const vector<string>& headers, int retryCount = 0);
};

class MyCurlWrapper : public CurlWrapper {
public:
    MyCurlWrapper(int max_connections) : CurlWrapper(max_connections) {}

    // Override hàm makeRequest()
    virtual void makeRequest(const string& url, const string& postFields)  {}
    // Override hàm authenticate()
    virtual void authenticate(const string& authCode)  {}
    // Override hàm refreshToken()
    virtual void refreshToken()  {}
};