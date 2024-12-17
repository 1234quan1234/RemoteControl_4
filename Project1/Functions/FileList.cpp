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

    // Get list of common directories
    std::vector<std::wstring> directories = {
        GetDocumentsPath(),
        GetDownloadsPath(),
        GetDesktopPath(),
        GetAppDataPath(),
        GetProgramFilesPath()
    };

    // Scan and write info for each directory
    for (const auto& dir : directories) {
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW((dir + L"\\*").c_str(), &findData);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    // Get file size
                    LARGE_INTEGER fileSize;
                    fileSize.LowPart = findData.nFileSizeLow;
                    fileSize.HighPart = findData.nFileSizeHigh;
                    double sizeMB = fileSize.QuadPart / (1024.0 * 1024.0);

                    // Convert wstring to string for output
                    std::wstring wFileName = findData.cFileName;
                    std::string fileName(wFileName.begin(), wFileName.end());
                    std::wstring wDirPath = dir;
                    std::string dirPath(wDirPath.begin(), wDirPath.end());

                    // Write formatted output
                    outFile << "File name: " << fileName << std::endl;
                    outFile << "Directory: " << dirPath << std::endl;
                    outFile << "Size: " << std::fixed << std::setprecision(2) << sizeMB << " MB" << std::endl;
                    outFile << "----------------------------------------" << std::endl;
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }

    outFile.close();
    return true;
}