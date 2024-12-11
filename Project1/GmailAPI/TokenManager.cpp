#include "..\GmailAPI\TokenManager.h"
#include "..\Client\HttpClient.h"

TokenManager::TokenManager(const string& clientId, const string& clientSecret,
    const string& redirectUri)
    : client_id(clientId), client_secret(clientSecret), redirect_uri(redirectUri) {
}

void TokenManager::authenticate(const string& authCode) {
    current_token = TokenLogic::getInitialTokens(authCode, client_id, client_secret, redirect_uri, *this);
    TokenLogic::saveTokens("token_storage.json", current_token); // Lưu trữ token vào file
}

void TokenManager::refreshToken() {
    string postFields = "grant_type=refresh_token&refresh_token=" + current_token.refresh_token +
        "&client_id=" + client_id + "&client_secret=" + client_secret;
    string response = performRequest("https://oauth2.googleapis.com/token", postFields, {}, "POST");

    TokenInfo new_token = TokenLogic::parseAndValidateToken(response);
    current_token = new_token;
    TokenLogic::saveTokens("token_storage.json", current_token); // Lưu trữ token vào file
}

void TokenManager::loadSavedTokens(const string& path) {
    ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    Json::Value tokens;
    file >> tokens;

    // Ví dụ: đọc access token và refresh token
    current_token.access_token = tokens["access_token"].asString();
	cout << "Access token: " << current_token.access_token << endl;
    current_token.refresh_token = tokens["refresh_token"].asString();
	cout << "Refresh token: " << current_token.refresh_token << endl;
    current_token.expires_in = tokens["expires_in"].asInt();
	cout << "Expires in: " << current_token.expires_in << endl;
    current_token.created_at = tokens["created_at"].asInt64();
	cout << "Created at: " << current_token.created_at << endl;
}

bool TokenManager::hasValidToken() const {
    return !current_token.access_token.empty();
}

const TokenInfo& TokenManager::getCurrentToken() const {
    return current_token;
}

TokenInfo TokenManager::TokenLogic::getInitialTokens(const string& authCode, const string& clientId, const string& clientSecret, const string& redirectUri, HttpClient& httpClient) {
    string url = "https://oauth2.googleapis.com/token";
    string body = "code=" + authCode + "&client_id=" + clientId + "&client_secret=" + clientSecret + "&redirect_uri=" + redirectUri + "&grant_type=authorization_code";

    string response = httpClient.sendRequest(url, body, "POST");

    return parseAndValidateToken(response);
}

TokenInfo TokenManager::TokenLogic::parseAndValidateToken(const string& response) {
    Json::Value tokens;
    Json::CharReaderBuilder builder;
    std::istringstream ss(response);
    std::string errors;
    if (!Json::parseFromStream(builder, ss, &tokens, &errors)) {
        throw std::runtime_error("Failed to parse token response: " + errors);
    }

    TokenInfo token;
    token.access_token = tokens["access_token"].asString();
    token.refresh_token = tokens["refresh_token"].asString();
    token.expires_in = tokens["expires_in"].asInt();
    token.created_at = std::time(nullptr);
    token.expires_at = token.created_at + token.expires_in;

    return token;
}

void TokenManager::TokenLogic::saveTokens(const string& path, const TokenInfo& token) {
    Json::Value tokens = token.toJson();
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    file << tokens.toStyledString();
}
