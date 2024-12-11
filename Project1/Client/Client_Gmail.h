#pragma once
#include "..\GmailAPI\GmailAPI.h"

class Client_Gmail : public GmailAPI {
private:
    CurlWrapper* curl;
    HttpClient* http_client;
    TokenManager* tokenManager;
    EmailFetcher emailFetcher;
    string scope;

    void refreshToken();
public:
    Client_Gmail(const std::string& client_id, const std::string& client_secret,
        const std::string& redirect_uri);

    ~Client_Gmail();

    // Sử dụng hàm authenticate của lớp GmailAPI
    void authenticate(const std::string& authCode) {
        GmailAPI::authenticate(authCode);
    }
    string getAuthorizationUrl() const;

    string GetClientEmail() { return emailFetcher.getMyEmail(); }
};
