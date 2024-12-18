﻿#include "..\Libs\Header.h"
#include "..\GmailAPI\GmailAPI.h"

GmailAPI::GmailAPI(const std::string& client_id, const std::string& client_secret,
    const std::string& redirect_uri)
    : curl(new MyCurlWrapper(3)),
    tokenManager(new MyTokenManager(client_id, client_secret, redirect_uri)),
    emailFetcher(*curl, *tokenManager),
    scope("https://www.googleapis.com/auth/gmail.modify") {
    cout << "Initializing GmailAPI..." << endl;
}

GmailAPI::~GmailAPI() {
    delete curl;
    delete tokenManager;
}

Json::Value GmailAPI::ReadClientSecrets(const string& path) {
    cout << "Reading client secrets from: " << path << endl;
    ifstream file(path);
    if (!file.is_open()) {
        throw runtime_error("Cannot open client secrets file: " + path);
    }

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