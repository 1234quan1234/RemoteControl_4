#include "..\Functions\ScreenshotHandler.h"

ScreenshotHandler::ScreenshotHandler() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

ScreenshotHandler::~ScreenshotHandler() {
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

int ScreenshotHandler::GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);

    if (size == 0) return -1;

    vector<BYTE> buffer(size);
    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)buffer.data();
    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            return j;
        }
    }
    return -1;
}

bool ScreenshotHandler::createDirectory(const string& path) const {
    if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return _mkdir(path.c_str()) == 0;
    }
    return true;
}

bool ScreenshotHandler::captureWindow(const string& filename) {
    // Set DPI awareness
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    // Get primary monitor
    HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);

    // Get monitor info
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, &monitorInfo);

    // Get actual dimensions
    int width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
    int height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

    // Create DC and bitmap
    HDC screenDC = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

    // Capture screen
    BitBlt(memDC, 0, 0, width, height,
        screenDC,
        monitorInfo.rcMonitor.left,
        monitorInfo.rcMonitor.top,
        SRCCOPY);

    // Save to file
    Gdiplus::Bitmap* screenshot = Gdiplus::Bitmap::FromHBITMAP(hBitmap, NULL);
    CLSID jpgClsid;
    GetEncoderClsid(L"image/jpeg", &jpgClsid);

    wstring wfilename(filename.begin(), filename.end());
    Gdiplus::Status status = screenshot->Save(wfilename.c_str(), &jpgClsid, NULL);

    // Cleanup
    delete screenshot;
    SelectObject(memDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, screenDC);

    return status == Gdiplus::Ok;
}