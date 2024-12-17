#pragma once
#include "..\Libs\Header.h"
#include "..\GmailAPI\CurlWrapper.h"
#include "..\GmailAPI\TokenManager.h"

class EmailFetcher {
private:
    CurlWrapper& curl;
    TokenManager& tokenManager;
    time_t serverStartTime;
    time_t lastFetchedTime;
    time_t lastCheckTime;
    const int CHECK_INTERVAL = 5;  // 5 seconds

    string getMyEmail();
    string decodeBase64(const string& encoded);
    string parseEmailContent(const Json::Value& emailData);
    bool readAttachmentFile(const string& path, string& content);
    string base64EncodeContent(const string& content);
    string createEmailContent(const string& to, const string& subject, const string& body,
        const string& attachmentPath, const string& encodedAttachment);
    string createSimpleEmailContent(const string& to, const string& subject, const string& body);
    string createEmailContentMultipleAttachments(
        const string& to,
        const string& subject,
        const string& body,
        const vector<pair<string, string>>& attachments 
    );

public:
    EmailFetcher(CurlWrapper& curl, TokenManager& tokenManager);
    vector<string> getEmailNow();
    vector<string> getRecentEmails();
    string getEmailDetails(const string& messageId);
    bool sendEmail(const string& to, const string& subject, const string& body, const string& attachmentPath);
    bool sendEmailWithAttachments(const string& to, const string& subject, const string& body, const vector<string>& attachmentPaths);
    bool sendSimpleEmail(const string& to, const string& subject, const string& body);
};