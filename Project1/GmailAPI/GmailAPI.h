#pragma once
#include "..\Libs\Header.h"
#include "..\GmailAPI\TokenInfo.h"
#include "..\GmailAPI\CurlWrapper.h"
#include "..\GmailAPI\TokenManager.h"
#include "..\Functions\EmailFetcher.h"

#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

class GmailAPI {
private:
    CurlWrapper* curl;
    HttpClient* http_client;
    TokenManager* tokenManager;
    EmailFetcher emailFetcher;
    string scope;

    void refreshToken();

    static const int LOCAL_PORT = 8080;
    std::string waitForAuthCode();

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
	bool sendEmailWithAttachments(const string& to, const string& subject, const string& body, const vector<string>& attachmentPaths);
    bool sendEmail(const string& to, const string& subject, const string& body, const string& attachmentPath);
	bool sendSimpleEmail(const string& to, const string& subject, const string& body);
	string getServerName();

    bool authenticateAutomatically();
};
