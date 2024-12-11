#include "..\Libs\Header.h"
#include "..\GmailAPI\CurlWrapper.h"

size_t CurlWrapper::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    // Mã code để lưu trữ dữ liệu vào biến userp
    // Ví dụ:
    string* response = static_cast<string*>(userp);
    response->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

CurlWrapper::CurlWrapper(int maxRetries) : MAX_RETRIES(maxRetries) {}

string CurlWrapper::performRequestWithRetry(const string& url, const string& method,
    const string& postFields, const vector<string>& headers, int retryCount) {

    CURL* curl;
    CURLcode res;
    string readBuffer;
    struct curl_slist* headers_list = NULL;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        // SSL/TLS Options
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        // Basic setup
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Headers
        for (const auto& header : headers) {
            headers_list = curl_slist_append(headers_list, header.c_str());
        }
        if (headers_list) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);
        }

        // Post data handling
        if (!postFields.empty()) {
            // Add content length header
            string contentLength = "Content-Length: " + to_string(postFields.length());
            headers_list = curl_slist_append(headers_list, contentLength.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postFields.length());
        }

        // Debug output
        /*curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, [](CURL* handle, curl_infotype type,
            char* data, size_t size, void* userp) -> int {
                string text(data, size);
                switch (type) {
                case CURLINFO_TEXT:
                    cout << "* " << text;
                    break;
                case CURLINFO_HEADER_OUT:
                    cout << "> " << text;
                    break;
                case CURLINFO_HEADER_IN:
                    cout << "< " << text;
                    break;
                case CURLINFO_SSL_DATA_IN:
                case CURLINFO_SSL_DATA_OUT:
                    cout << "* SSL/TLS traffic\n";
                    break;
                default:
                    break;
                }
                return 0;
            });*/

        // Method setting
        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        }
        else if (method == "GET") {
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        }
        else if (method == "PUT") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        }
        else if (method == "DELETE") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }

        res = curl_easy_perform(curl);

        // Error handling
        if (res != CURLE_OK) {
            cout << "CURL error: " << curl_easy_strerror(res) << endl;
            if (headers_list) curl_slist_free_all(headers_list);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return performRequestWithRetry(url, method, postFields, headers, retryCount + 1);
        }

        if (headers_list) curl_slist_free_all(headers_list);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return readBuffer;
}