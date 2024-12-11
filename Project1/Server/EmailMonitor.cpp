#include "..\Server\EmailMonitor.h"
#include "..\Server\ServerManager.h"
#include "..\GmailAPI\GmailAPI.h"

EmailMonitor::EmailMonitor(GmailAPI& api, ServerConfig& cfg, ServerManager& mgr)
    : gmail(api), config(cfg), server(mgr) {
}


bool EmailMonitor::checkForCommands() {
    try {
        auto emails = gmail.getEmailNow();
        bool foundCommand = false;

        for (const auto& email : emails) {
            Json::Value emailData;
            Json::Reader reader;
            string errors;

            // Create string stream as lvalue
            string emailStr = email;
            istringstream emailStream(emailStr);  // Now using lvalue

            // Check if email is a valid JSON object
            if (!reader.parse(emailStream, emailData, &errors)) {
                cerr << "Failed to parse email JSON: " << errors << endl;
                continue;
            }

            // Kiểm tra thành viên "Subject" trong đối tượng JSON emailData
            if (emailData.isMember("emailDetails")) {
                // Lấy Subject trực tiếp từ đối tượng emailDetails
                string subject = emailData["emailDetails"]["Subject"].asString();

                // Xử lý Subject
                cout << "Subject: " << subject << endl;

                // Kiểm tra Subject có chứa lệnh hay không
                if (subject.find("Command") != string::npos) {
                    server.handleCommand(emailData["emailDetails"]);
                    foundCommand = true;
                    break;
                }
            }

        }
        return foundCommand;
    }
    catch (const exception& e) {
        cerr << "Error in checkForCommands: " << e.what() << endl;
        return false;
    }
}




