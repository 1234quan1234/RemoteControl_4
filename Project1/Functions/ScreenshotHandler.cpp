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
    // Get screen dimensions
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    // Create DC and bitmap
    HDC screenDC = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, width, height);
    SelectObject(memDC, hBitmap);

    // Capture screen to bitmap
    BitBlt(memDC, 0, 0, width, height, screenDC, 0, 0, SRCCOPY);

    // Create GDI+ bitmap
    Gdiplus::Bitmap* screenshot = Gdiplus::Bitmap::FromHBITMAP(hBitmap, NULL);

    // Get JPEG encoder
    CLSID jpgClsid;
    GetEncoderClsid(L"image/jpeg", &jpgClsid);

    // Convert string to wstring for GDI+
    wstring wfilename(filename.begin(), filename.end());

    // Save to file
    Gdiplus::Status status = screenshot->Save(wfilename.c_str(), &jpgClsid, NULL);

    // Cleanup
    delete screenshot;
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, screenDC);

    return status == Gdiplus::Ok;
}