#include "..\Client\HttpClient.h"
#include "..\GmailAPI\CurlWrapper.h"

void HttpClient::makeRequest(const string& url, const string& postFields) {
    // Hàm này phải được triển khai bởi các lớp dẫn xuất
    throw std::runtime_error("makeRequest không được triển khai");
}

void HttpClient::authenticate(const string& authCode) {
    // Hàm này phải được triển khai bởi các lớp dẫn xuất
    throw std::runtime_error("authenticate không được triển khai");
}

void HttpClient::refreshToken() {
    // Hàm này phải được triển khai bởi các lớp dẫn xuất
    throw std::runtime_error("refreshToken không được triển khai");
}

string HttpClient::performRequest(const string& url, const string& postFields, const vector<string>& headers, const string& method) {
    string response;

    // Triển khai gửi yêu cầu HTTP
    // Ví dụ sử dụng curl
    CURL* curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        struct curl_slist* headerList = nullptr;
        for (const auto& header : headers) {
            headerList = curl_slist_append(headerList, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrapper::WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "Error sending request: " << curl_easy_strerror(res) << endl;
        }
        curl_slist_free_all(headerList);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return response;
}