#pragma once
#include "..\Libs\Header.h"


class ScreenshotHandler {
private:
    ULONG_PTR gdiplusToken;
    const string screenshotDir = "screenshots";
    static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
    bool createDirectory(const string& path) const;

public:
    ScreenshotHandler();
    ~ScreenshotHandler();
    bool captureWindow(const string& filename);
};