#include "..\Libs\Header.h"
#include "..\GmailAPI\GmailAPI.h"

GmailAPI::GmailAPI(const std::string& client_id, const std::string& client_secret, const std::string& redirect_uri)
    : curl(new MyCurlWrapper(3)),
    tokenManager(new MyTokenManager(client_id, client_secret, redirect_uri)),
    emailFetcher(*curl, *tokenManager),
    scope("https://www.googleapis.com/auth/gmail.modify") 
{};

GmailAPI::~GmailAPI() {
    delete curl;
    delete tokenManager;
};

string GmailAPI::getServerName() {
	return emailFetcher.getMyEmail();
}

Json::Value GmailAPI::ReadClientSecrets(const string& path) {

    // Mother folder directory
    char buffer[100];
    GetCurrentDirectoryA(100, buffer);

    // Buffer to string
    std::string fix_path(buffer);
    fix_path += path; 

    ifstream file(fix_path);
    if (!file.is_open()) {
        throw runtime_error("Cannot open client secrets file: " + fix_path);
    }
    cout << "Reading client secrets from: " << fix_path << endl;

    Json::Value root;
    Json::CharReaderBuilder reader;
    string errors;

    if (!Json::parseFromStream(reader, file, &root, &errors)) {
        throw runtime_error("Failed to parse client secrets: " + errors);
    }

    if (!root.isMember("installed") ||
        !root["installed"].isMember("client_id") ||
        !root["installed"].isMember("client_secret") ||
        !root["installed"].isMember("redirect_uris")) {
        throw runtime_error("Invalid client secrets format");
    }
    
    return root;
}

bool GmailAPI::sendEmail(const string& to, const string& subject, const string& body, const string& attachmentPath) {
	return emailFetcher.sendEmail(to, subject, body, attachmentPath);
}

bool GmailAPI::sendSimpleEmail(const string& to, const string& subject, const string& body) {
	return emailFetcher.sendSimpleEmail(to, subject, body);
}   

std::string GmailAPI::getAuthorizationUrl() const {
    return "https://accounts.google.com/o/oauth2/auth?"
        "response_type=code&"
        "client_id=" + tokenManager->getClientId() +
        "&redirect_uri=" + tokenManager->getRedirectUri() +
        "&scope=https://www.googleapis.com/auth/gmail.modify&"
        "access_type=offline&"
        "prompt=consent";
}

void GmailAPI::authenticate(const std::string& authCode) {
    cout << "Starting authentication process..." << endl;
    tokenManager->authenticate(authCode);
    cout << "Authentication completed successfully" << endl;
}

std::vector<std::string> GmailAPI::getRecentEmails() {
    return emailFetcher.getRecentEmails();
}

std::vector<std::string> GmailAPI::getEmailNow() {
    return emailFetcher.getEmailNow();
}

bool GmailAPI::hasValidToken() const {
    return tokenManager->hasValidToken();
}

void GmailAPI::loadSavedTokens() {
    tokenManager->loadSavedTokens("token_storage.json");
}


bool GmailAPI::authenticateAutomatically() {
    // Open the default browser with the auth URL
    std::string authUrl = getAuthorizationUrl();
    ShellExecuteA(NULL, "open", authUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);

    // Wait for and capture the auth code
    std::string authCode = waitForAuthCode();
    if (authCode.empty()) {
        std::cerr << "Failed to receive authentication code" << std::endl;
        return false;
    }

    // Use the existing authenticate method
    try {
        authenticate(authCode);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Authentication failed: " << e.what() << std::endl;
        return false;
    }
}

std::string GmailAPI::waitForAuthCode() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("Failed to initialize WinSock");
    }

    // Create socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Failed to create socket");
    }

    // Setup server address
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(LOCAL_PORT);

    // Bind
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(serverSocket);
        WSACleanup();
        throw std::runtime_error("Failed to bind socket");
    }

    // Listen
    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        closesocket(serverSocket);
        WSACleanup();
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "Waiting for OAuth2 callback on port " << LOCAL_PORT << "..." << std::endl;

    // Accept connection
    SOCKET clientSocket = accept(serverSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        closesocket(serverSocket);
        WSACleanup();
        throw std::runtime_error("Failed to accept connection");
    }

    // Receive HTTP request
    char buffer[4096] = { 0 };
    recv(clientSocket, buffer, sizeof(buffer), 0);

    // Parse the code parameter
    std::string response(buffer);
    std::string authCode;
    size_t codePos = response.find("?code=");
    if (codePos != std::string::npos) {
        size_t codeEnd = response.find(" ", codePos);
        if (codeEnd != std::string::npos) {
            authCode = response.substr(codePos + 6, codeEnd - (codePos + 6));
        }
    }

    // Send success response to browser
    const char* successResponse = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<!DOCTYPE html>\r\n"
        "<html lang=\"en\">\r\n"
        "<head>\r\n"
        "    <meta charset=\"UTF-8\">\r\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n"
        "    <title>Authorization Success</title>\r\n"
        "    <style>\r\n"
        "        body {\r\n"
        "            font-family: Arial, sans-serif;\r\n"
        "            background-color: #f0f2f5;\r\n"
        "            display: flex;\r\n"
        "            justify-content: center;\r\n"
        "            align-items: center;\r\n"
        "            height: 100vh;\r\n"
        "            margin: 0;\r\n"
        "        }\r\n"
        "        .container {\r\n"
        "            background: white;\r\n"
        "            padding: 40px;\r\n"
        "            border-radius: 10px;\r\n"
        "            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);\r\n"
        "            text-align: center;\r\n"
        "        }\r\n"
        "        h1 {\r\n"
        "            color: #1a73e8;\r\n"
        "            margin-bottom: 20px;\r\n"
        "        }\r\n"
        "        p {\r\n"
        "            color: #5f6368;\r\n"
        "            font-size: 18px;\r\n"
        "        }\r\n"
        "        .success-icon {\r\n"
        "            font-size: 64px;\r\n"
        "            margin-bottom: 20px;\r\n"
        "            color: #4caf50; /* Green color for the success icon */\r\n"
        "        }\r\n"
        "    </style>\r\n"
        "</head>\r\n"
        "<body>\r\n"
        "    <div class=\"container\">\r\n"
        "        <div class=\"success-icon\">&#x2705;</div> <!-- HTML entity for a green check mark -->\r\n"
        "        <h1>Authorization Successful!</h1>\r\n"
        "        <p>Your account has been successfully connected.</p>\r\n"
        "        <p>You can now close this window and return to the application.</p>\r\n"
        "    </div>\r\n"
        "</body>\r\n"
        "</html>\r\n"
        ;
    send(clientSocket, successResponse, strlen(successResponse), 0);

    // Cleanup
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    return authCode;
}