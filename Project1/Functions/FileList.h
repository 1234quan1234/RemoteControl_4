#pragma once
#include "..\Libs\Header.h"

class FileList {
private:
    std::wstring userProfilePath;

public:
    // Constructor to initialize user profile path
    FileList() {
        WCHAR path[MAX_PATH];
        SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, path);
        userProfilePath = path;
    }

    // Common folder access methods
    std::wstring GetDocumentsPath();
    std::wstring GetDownloadsPath();
    std::wstring GetDesktopPath();
    std::wstring GetAppDataPath();
    std::wstring GetProgramFilesPath();
    std::wstring GetRecentFilesPath();

    // Simple file writing method
    bool writeFilesToFile(const std::string& filename);
};