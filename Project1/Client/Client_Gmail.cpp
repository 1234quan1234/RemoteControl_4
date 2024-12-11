#include "Client_Gmail.h"

Client_Gmail::Client_Gmail(const std::string& client_id, const std::string& client_secret,
    const std::string& redirect_uri)
    : GmailAPI(client_id, client_secret, redirect_uri),
    curl(new MyCurlWrapper(3)),
    tokenManager(new MyTokenManager(client_id, client_secret, redirect_uri)),
    emailFetcher(*curl, *tokenManager),
    scope("https://www.googleapis.com/auth/gmail.send") {
    cout << "Initializing Client_Gmail..." << endl;
}

Client_Gmail::~Client_Gmail() {
    // Giải phóng tài nguyên được sử dụng bởi đối tượng
    delete curl;
    delete tokenManager;
}

string Client_Gmail::getAuthorizationUrl() const {
    return "https://accounts.google.com/o/oauth2/auth?"
        "response_type=code&"
        "client_id=" + tokenManager->getClientId() +
        "&redirect_uri=" + tokenManager->getRedirectUri() +
        "&scope=https://www.googleapis.com/auth/gmail.send&"
        "access_type=offline&"
        "prompt=consent";
}