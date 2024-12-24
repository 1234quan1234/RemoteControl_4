#include "FileList.h"

std::wstring FileList::GetDocumentsPath() {
    WCHAR path[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, path);
    return path;
}

std::wstring FileList::GetDownloadsPath() {
    return userProfilePath + L"\\Downloads";
}

std::wstring FileList::GetDesktopPath() {
    WCHAR path[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, path);
    return path;
}

std::wstring FileList::GetAppDataPath() {
    WCHAR path[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path);
    return path;
}

std::wstring FileList::GetProgramFilesPath() {
    WCHAR path[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, 0, path);
    return path;
}

std::wstring FileList::GetRecentFilesPath() {
    WCHAR path[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_RECENT, NULL, 0, path);
    return path;
}

bool FileList::writeFilesToFile(const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) return false;

    // Set fixed width for columns
    const int nameWidth = 30;
    const int pathWidth = 50;
    const int sizeWidth = 15;

    // Write table header
    outFile << std::left
        << std::setw(nameWidth) << "File Name" << " | "
        << std::setw(pathWidth) << "Directory" << " | "
        << std::setw(sizeWidth) << "Size (MB)" << std::endl;
    outFile << std::string(nameWidth + pathWidth + sizeWidth + 6, '-') << std::endl;

    // Define directories with labels
    struct DirInfo {
        std::wstring path;
        std::string label;
    };

    std::vector<DirInfo> directories = {
        {GetDocumentsPath(), "Documents"},
        {GetDownloadsPath(), "Downloads"},
        {GetDesktopPath(), "Desktop"},
        {GetAppDataPath(), "AppData"},
        {GetProgramFilesPath(), "Program Files"}
    };

    // Scan each directory
    for (const auto& dirInfo : directories) {
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW((dirInfo.path + L"\\*").c_str(), &findData);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    LARGE_INTEGER fileSize;
                    fileSize.LowPart = findData.nFileSizeLow;
                    fileSize.HighPart = findData.nFileSizeHigh;
                    double sizeMB = fileSize.QuadPart / (1024.0 * 1024.0);

                    std::wstring wFileName = findData.cFileName;
                    std::string fileName(wFileName.begin(), wFileName.end());
                    std::wstring wDirPath = dirInfo.path + L"\\" + findData.cFileName;
                    std::string fullPath(wDirPath.begin(), wDirPath.end());

                    outFile << std::left
                        << std::setw(nameWidth) << fileName << " | "
                        << std::setw(pathWidth) << (dirInfo.label + ": " + fullPath) << " | "
                        << std::setw(sizeWidth) << std::fixed << std::setprecision(2) << sizeMB << std::endl;
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }

    outFile.close();
    return true;
}

bool FileList::deleteFiles(const std::vector<std::string>& filePaths, std::string& logFileName) {
    // Generate log filename with timestamp
    time_t now = time(nullptr);
    logFileName = "D:\\file_deletion_" + to_string(now) + ".txt";
    ofstream logFile(logFileName, ios::app);

    char timeStr[26];
    ctime_s(timeStr, sizeof(timeStr), &now);
    logFile << "\n=== File Deletion Log " << timeStr << "===\n";

    bool allSuccess = true;

    for (const auto& path : filePaths) {
        std::wstring wpath(path.begin(), path.end());

        if (!DeleteFileW(wpath.c_str())) {
            DWORD error = GetLastError();
            logFile << "Failed to delete file: " << path
                << " Error code: " << error << "\n";
            allSuccess = false;
        }
        else {
            logFile << "Successfully deleted: " << path << "\n";
        }
    }

    logFile << "=== End of Log ===\n\n";
    logFile.close();

    return allSuccess;
}