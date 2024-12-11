#pragma once
#include "..\Libs\Header.h"
#include "..\GmailAPI\TokenInfo.h"
#include "..\GmailAPI\CurlWrapper.h"
#include "..\GmailAPI\TokenManager.h"
#include "..\Functions\EmailFetcher.h"

class GmailAPI {
private:
    CurlWrapper* curl;
    HttpClient* http_client;
    TokenManager* tokenManager;
    EmailFetcher emailFetcher;
    string scope;

    void refreshToken();

public:
    GmailAPI(const std::string& client_id="", const std::string& client_secret="", const std::string& redirect_uri="");
    ~GmailAPI();  
    static Json::Value ReadClientSecrets(const std::string& path);
    std::string getAuthorizationUrl() const;
    void authenticate(const std::string& authCode);
    std::vector<std::string> getEmailNow();
    std::vector<std::string> getRecentEmails();
    bool hasValidToken() const;
    void loadSavedTokens();
    bool sendEmail(const string& to, const string& subject, const string& body, const string& attachmentPath);
	bool sendSimpleEmail(const string& to, const string& subject, const string& body);
};
