#include "..\Libs\Header.h"
#include "..\Functions\EmailFetcher.h"

EmailFetcher::EmailFetcher(CurlWrapper& curl, TokenManager& tokenManager)
    : curl(curl), tokenManager(tokenManager), serverStartTime(time(nullptr)), lastFetchedTime(time(nullptr))
{
    static bool isFirstCall = true;
    if (isFirstCall) {
        char timeStr[26];
        ctime_s(timeStr, sizeof(timeStr), &serverStartTime);
        cout << "Server started at: " << timeStr;
        isFirstCall = false;
    }
}

bool EmailFetcher::readAttachmentFile(const string& path, string& content) {
    ifstream file(path, ios::binary);
    if (!file.is_open()) {
        cout << "Error: Unable to open attachment file" << endl;
        return false;
    }
    content = string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
    return true;
}

string EmailFetcher::base64EncodeContent(const string& content) {
    // Disable newlines in base64 output
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    BIO* bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    // Write content
    BIO_write(bio, content.c_str(), content.length());
    BIO_flush(bio);

    // Get result
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);

    // Copy to string and ensure proper padding
    string encoded(bufferPtr->data, bufferPtr->length);

    // Remove any whitespace
    encoded.erase(remove_if(encoded.begin(), encoded.end(), ::isspace), encoded.end());

    // Add padding if needed
    while (encoded.length() % 4) {
        encoded += '=';
    }

    // Cleanup
    BIO_free_all(bio);

    return encoded;
}

string EmailFetcher::createEmailContent(const string& to, const string& subject, const string& body,
    const string& attachmentPath, const string& encodedAttachment) {
    return "From: qyanbyi23@gmail.com\r\n"
        "To: " + to + "\r\n"
        "Subject: " + subject + "\r\n"
        "Content-Type: multipart/mixed; boundary=boundary\r\n\r\n"
        "--boundary\r\n"
        "Content-Type: text/plain; charset=UTF-8\r\n\r\n"
        + body + "\r\n\r\n"
        "--boundary\r\n"
        "Content-Type: application/octet-stream; name=\"" + attachmentPath + "\"\r\n"
        "Content-Transfer-Encoding: base64\r\n\r\n"
        + encodedAttachment + "\r\n\r\n"
        "--boundary--\r\n";
}

bool EmailFetcher::sendEmail(const string& to, const string& subject, const string& body, const string& attachmentPath) {
    cout << "\n=== Starting email send process ===\n";

    // 1. Token validation
    cout << "Current token status: " << (tokenManager.hasValidToken() ? "Valid" : "Invalid") << endl;
    if (!tokenManager.hasValidToken()) {
        tokenManager.refreshToken();
    }

    // 2. Read attachment file
    cout << "\nReading attachment: " << attachmentPath << endl;
    string attachmentContent;
    if (!readAttachmentFile(attachmentPath, attachmentContent)) {
        cout << "Failed to read attachment file\n";
        return false;
    }
    cout << "Attachment size: " << attachmentContent.length() << " bytes\n";

    // 3. Encode attachment
    cout << "\nEncoding attachment to base64...\n";
    string encodedAttachment = base64EncodeContent(attachmentContent);
    cout << "Encoded attachment size: " << encodedAttachment.length() << " bytes\n";

    // 4. Create email content
    cout << "\nCreating email content...\n";
    cout << "To: " << to << "\nSubject: " << subject << endl;
    string emailContent = createEmailContent(to, subject, body, attachmentPath, encodedAttachment);
    cout << "Email content size: " << emailContent.length() << " bytes\n";

    // 5. Encode email
    cout << "\nEncoding email to base64...\n";
    string encodedEmail = base64EncodeContent(emailContent);
    cout << "Encoded email size: " << encodedEmail.length() << " bytes\n";

    // 6. Create request body
    cout << "\nPreparing request...\n";
    Json::Value requestBody;
    requestBody["raw"] = encodedEmail;

    // 7. Send request
    vector<string> headers = {
        "Authorization: Bearer " + tokenManager.getCurrentToken().access_token,
        "Content-Type: application/json",
        "Accept: application/json"  // Add Accept header
    };

    cout << "Request headers:\n";
    for (const auto& header : headers) {
        cout << "  " << header.substr(0, header.find(':')) << ": [REDACTED]\n";
    }
    cout << "Request body size: " << requestBody.toStyledString().length() << " bytes\n";

    cout << "\nSending request to Gmail API...\n";
    string response = curl.performRequestWithRetry(
        "https://gmail.googleapis.com/gmail/v1/users/me/messages/send",
        "POST",
        requestBody.toStyledString(),
        headers
    );

    cout << "\n=== Response ===\n";
    cout << "Response length: " << response.length() << " bytes\n";
    cout << "Response content: " << response << "\n";
    cout << "=== End of email send process ===\n\n";

    return !response.empty();
}

string EmailFetcher::getMyEmail() {
    if (!tokenManager.hasValidToken()) {
        tokenManager.refreshToken();
    }

    vector<string> headers = {
        "Authorization: Bearer " + tokenManager.getCurrentToken().access_token
    };

    string response = curl.performRequestWithRetry(
        "https://gmail.googleapis.com/gmail/v1/users/me/profile",
        "GET",
        "",
        headers
    );

    Json::Value profile;
    Json::CharReaderBuilder reader;
    istringstream(response) >> profile;

    return profile["emailAddress"].asString();
}

vector<string> EmailFetcher::getEmailNow() {
    if (!tokenManager.hasValidToken()) {
        tokenManager.refreshToken();
    }

    vector<string> headers = {
        "Authorization: Bearer " + tokenManager.getCurrentToken().access_token
    };

    string query = "https://www.googleapis.com/gmail/v1/users/me/messages?q=subject:Command::+after:"
        + to_string(lastFetchedTime)
        + "&format=full";

    string response = curl.performRequestWithRetry(query, "GET", "", headers);

    vector<string> emails;
    Json::Value jsonData;
    Json::CharReaderBuilder reader;
    string errors;
    istringstream responseStream(response);

    if (Json::parseFromStream(reader, responseStream, &jsonData, &errors)) {
        // Kiểm tra xem có lỗi nào xảy ra hay không
        if (!errors.empty()) {
            cout << "Error parsing JSON: " << errors << endl;
            return vector<string>();
        }

        // Kiểm tra xem dữ liệu JSON có chứa mảng messages hay không
        if (jsonData.isMember("messages")) {
            // Lấy dữ liệu
            for (const auto& message : jsonData["messages"]) {
                Json::Value emailData;
                emailData["id"] = message["id"].asString();
                emailData["internalDate"] = message["internalDate"].asInt();
                string emailDetails = getEmailDetails(message["id"].asString());

                // Tách emailDetails thành key-value pairs
                vector<string> emailDetailsParts;
                size_t pos = 0;
                string token;
                while ((pos = emailDetails.find("\n")) != string::npos) {
                    token = emailDetails.substr(0, pos);
                    emailDetailsParts.push_back(token);
                    emailDetails.erase(0, pos + 1);
                }
                emailDetailsParts.push_back(emailDetails);

                Json::Value emailDetailsObject;
                for (const auto& part : emailDetailsParts) {
                    size_t colonPos = part.find(":");
                    if (colonPos != string::npos) {
                        string key = part.substr(0, colonPos);
                        string value = part.substr(colonPos + 1);
                        value.erase(0, value.find_first_not_of(" ")); // Xóa dấu cách thừa
                        emailDetailsObject[key] = value;
                    }
                }

                emailData["emailDetails"] = emailDetailsObject;

                emails.push_back(emailData.toStyledString());
                cout << emailData.toStyledString() << endl;
                lastFetchedTime = max(lastFetchedTime, emailData["internalDate"].asInt());
            }
            lastFetchedTime = 0;
            return emails;
        }
        else {
            if (jsonData.isMember("error")) {
                if (jsonData["error"]["code"].asInt() == 401) {
                    cout << "Gặp lỗi 401, refresh token" << endl;
                    tokenManager.refreshToken();
                    return getEmailNow();
                }
            }
            cout << "Không phải messages, nội dung là: " << endl;
            cout << jsonData << endl;
            cout << "Dữ liệu JSON không chứa mảng messages" << endl;
            return vector<string>();
        }
    }
    else {
        cout << "Error parsing JSON: " << errors << endl;
        return vector<string>();
    }
}

vector<string> EmailFetcher::getRecentEmails() {
    if (!tokenManager.hasValidToken()) {
        tokenManager.refreshToken();
    }

    // Calculate date 7 days ago
    time_t now = time(nullptr);
    time_t weekAgo = now - (7 * 24 * 60 * 60);
    tm weekAgoTm;
    localtime_s(&weekAgoTm, &weekAgo);

    char dateStr[11];
    strftime(dateStr, sizeof(dateStr), "%Y/%m/%d", &weekAgoTm);
    string dateQuery = "after:" + string(dateStr);

    vector<string> headers = {
        "Authorization: Bearer " + tokenManager.getCurrentToken().access_token
    };

    string response = curl.performRequestWithRetry(
        "https://www.googleapis.com/gmail/v1/users/me/messages?q=" + dateQuery,
        "POST",
        "",
        headers
    );

    vector<string> emails;
    Json::Value jsonData;
    Json::CharReaderBuilder reader;
    string errors;
    istringstream responseStream(response);

    if (Json::parseFromStream(reader, responseStream, &jsonData, &errors)) {
        if (jsonData.isMember("messages")) {
            const Json::Value& messages = jsonData["messages"];
            for (const auto& message : messages) {
                try {
                    emails.push_back(getEmailDetails(message["id"].asString()));
                }
                catch (const exception& e) {
                    cerr << "Error getting email details: " << e.what() << endl;
                }
            }
        }
    }

    return emails;
}

string EmailFetcher::decodeBase64(const string& encoded) {
    string decoded;
    BIO* bio = BIO_new_mem_buf(encoded.c_str(), -1);
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    char buffer[1024];
    int length;
    while ((length = BIO_read(bio, buffer, sizeof(buffer))) > 0) {
        decoded.append(buffer, length);
    }

    BIO_free_all(bio);
    return decoded;
}

string EmailFetcher::parseEmailContent(const Json::Value& emailData) {
    stringstream result;

    // Get headers
    if (emailData.isMember("payload") && emailData["payload"].isMember("headers")) {
        const Json::Value& headers = emailData["payload"]["headers"];
        for (const auto& header : headers) {
            string name = header["name"].asString();
            string value = header["value"].asString();
            if (name == "From") {
                size_t pos = value.find("<");
                if (pos != string::npos) {
                    value = value.substr(pos + 1);
                    pos = value.find(">");
                    if (pos != string::npos) {
                        value = value.substr(0, pos);
                    }
                }
            }
            value.erase(0, value.find_first_not_of(" "));
            if (name == "Subject" || name == "From" || name == "Date") {
                result << name << ": " << value << "\n";
            }
        }
    }

    // Get body
    if (emailData.isMember("payload") && emailData["payload"].isMember("body")) {
        const Json::Value& body = emailData["payload"]["body"];
        if (body.isMember("data")) {
            string encoded = body["data"].asString();
            result << "\nContent:\n" << decodeBase64(encoded) << "\n";
        }
    }

    return result.str();
}

string EmailFetcher::getEmailDetails(const string& messageId) {
    vector<string> headers = {
        "Authorization: Bearer " + tokenManager.getCurrentToken().access_token
    };

    string response = curl.performRequestWithRetry(
        "https://www.googleapis.com/gmail/v1/users/me/messages/" + messageId,
        "GET",
        "",
        headers
    );

    Json::Value emailData;
    Json::CharReaderBuilder reader;
    string errors;
    istringstream responseStream(response);

    if (!Json::parseFromStream(reader, responseStream, &emailData, &errors)) {
        throw runtime_error("Failed to parse email details");
    }

    return parseEmailContent(emailData);
}