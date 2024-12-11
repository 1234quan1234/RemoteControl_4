#pragma once
#include "..\Libs\Header.h"
#include "..\GmailAPI\TokenInfo.h"
#include "..\Client\HttpClient.h"

class TokenManager : public HttpClient {
private:
    TokenInfo current_token;
    string client_id;
    string client_secret;
    string redirect_uri;

public:
    TokenManager(const string& clientId, const string& clientSecret,
        const string& redirectUri);

    void authenticate(const string& authCode);
    void refreshToken();
    bool hasValidToken() const;
    void loadSavedTokens(const string& path);
    const TokenInfo& getCurrentToken() const;


    string getClientId() const { return client_id; }
    string getRedirectUri() const { return redirect_uri; }

    // Tách riêng logic token
    class TokenLogic {
    public:
        static TokenInfo getInitialTokens(const string& authCode, const string& clientId, const string& clientSecret, const string& redirectUri, HttpClient& httpClient);
        static TokenInfo parseAndValidateToken(const string& response);
        static void saveTokens(const string& path, const TokenInfo& token);
    };
};

class MyTokenManager : public TokenManager {
public:
    MyTokenManager(const std::string& client_id, const std::string& client_secret, const std::string& redirect_uri)
        : TokenManager(client_id, client_secret, redirect_uri) {}

    // Override hàm makeRequest()
    virtual void makeRequest(const string& url, const string& postFields)  {}
};