#include <iostream>
#include <fstream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>

using namespace std;

// Callback function for handling the response data from libcurl
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to read the client secrets from a JSON file
static Json::Value ReadClientSecrets(const string& path) {
    ifstream json_file(path, ifstream::binary);
    if (!json_file.is_open()) {
        cerr << "Failed to open JSON file: " << path << endl;
        throw Json::RuntimeError("Failed to open JSON file");
    }

    Json::Value root;
    Json::CharReaderBuilder reader;
    string errs;
    if (!Json::parseFromStream(reader, json_file, &root, &errs)) {
        cerr << "Failed to parse JSON: " << errs << endl;
        throw Json::RuntimeError("Failed to parse JSON");
    }
    return root;
}

// Function to get the authorization URL
static string GetAuthorizationUrl(const string& client_id, const string& redirect_uri, const string& scope) {
    return "https://accounts.google.com/o/oauth2/auth?response_type=code&client_id=" + client_id +
        "&redirect_uri=" + redirect_uri + "&scope=" + scope + "&access_type=offline";
}

// Function to exchange authorization code for access token
static string GetAccessToken(const string& client_id, const string& client_secret, const string& code, const string& redirect_uri) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        string post_fields = "code=" + code + "&client_id=" + client_id + "&client_secret=" + client_secret +
            "&redirect_uri=" + redirect_uri + "&grant_type=authorization_code";

        curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Specify the correct path to the CA certificate bundle
        // curl_easy_setopt(curl, CURLOPT_CAINFO, "C:\\Users\\GIGABYTE\\myCA.pem");

        // Disable SSL verification (for testing purposes only)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            curl_easy_cleanup(curl);
            return "";
        }

        curl_easy_cleanup(curl);
    }
    else {
        cerr << "Failed to initialize CURL." << endl;
        return "";
    }

    Json::Value root;
    Json::CharReaderBuilder reader;
    string errs;
    istringstream s(readBuffer);
    string access_token;

    if (Json::parseFromStream(reader, s, &root, &errs)) {
        if (root.isMember("access_token")) {
            access_token = root["access_token"].asString();
        }
        else {
            cerr << "Error: " << root.toStyledString() << endl;
        }
    }
    else {
        cerr << "Failed to parse JSON: " << errs << endl;
    }

    return access_token;
}

// Function to send an email
static void SendEmail(const string& access_token, const string& to, const string& subject, const string& body) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    string raw_message = "To: " + to + "\r\n" +
        "Subject: " + subject + "\r\n" +
        "\r\n" +
        body;

    curl = curl_easy_init();
    string encoded_message = "raw=" + string(curl_easy_escape(curl, raw_message.c_str(), static_cast<int>(raw_message.length())));

    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, "https://www.googleapis.com/gmail/v1/users/me/messages/send");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, encoded_message.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
        else {
            cout << "Email Sent: " << readBuffer << endl;
        }
    }
}

int main() {
    try {
        // Read client secrets
        Json::Value client_secrets = ReadClientSecrets("C:\\Users\\GIGABYTE\\Downloads\\Client.json");
        string client_id = client_secrets["installed"]["client_id"].asString();
        string client_secret = client_secrets["installed"]["client_secret"].asString();
        string redirect_uri = client_secrets["installed"]["redirect_uris"][0].asString();
        string scope = "https://www.googleapis.com/auth/gmail.send";

        // Generate the authorization URL
        string auth_url = GetAuthorizationUrl(client_id, redirect_uri, scope);
        cout << "Open the following URL in your browser to authorize the application:" << endl;
        cout << auth_url << endl;

        // Prompt user to enter the authorization code
        string authorization_code;
        cout << "Enter the authorization code: ";
        cin >> authorization_code;

        // Exchange the authorization code for an access token
        string access_token = GetAccessToken(client_id, client_secret, authorization_code, redirect_uri);

        if (access_token.empty()) {
            cerr << "Failed to get access token." << endl;
            return 1;
        }

        // Send an email with the command
        string to = "server@example.com"; // Replace with the server's email address
        string subject = "Remote Command";
        string body = "shutdown"; // Replace with the desired command
        SendEmail(access_token, to, subject, body);
    }
    catch (const Json::RuntimeError& e) {
        cerr << "Runtime error: " << e.what() << endl;
        return 1;
    }
    catch (const std::exception& e) {
        cerr << "Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}