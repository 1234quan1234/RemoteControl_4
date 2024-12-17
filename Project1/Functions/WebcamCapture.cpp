#include "WebcamCapture.h"

WebcamCapture::WebcamCapture() {
    pReader = NULL;
    pSource = NULL;
    pAttributes = NULL;
    pMediaType = NULL;

    // Initialize Media Foundation
    MFStartup(MF_VERSION);
}

WebcamCapture::~WebcamCapture() {
    if (pMediaType) pMediaType->Release();
    if (pReader) pReader->Release();
    if (pSource) pSource->Release();
    if (pAttributes) pAttributes->Release();

    MFShutdown();
}

HRESULT WebcamCapture::GetUnconnectedPin(IBaseFilter* pFilter, PIN_DIRECTION direction, IPin** ppPin) {
    std::cout << "[DEBUG] Looking for pin. Direction: "
        << (direction == PINDIR_OUTPUT ? "output" : "input") << "\n";

    if (!pFilter || !ppPin) {
        std::cout << "[ERROR] Invalid filter/pin pointer\n";
        return E_POINTER;
    }

    *ppPin = nullptr;
    IEnumPins* pEnum = nullptr;
    HRESULT hr = pFilter->EnumPins(&pEnum);

    if (FAILED(hr) || !pEnum) {
        std::cout << "[ERROR] EnumPins failed: " << hr << "\n";
        return hr;
    }

    // Find unconnected pin with matching direction
    IPin* pPin = nullptr;
    while (pEnum->Next(1, &pPin, nullptr) == S_OK) {
        PIN_DIRECTION pinDir;
        hr = pPin->QueryDirection(&pinDir);

        if (SUCCEEDED(hr) && pinDir == direction) {
            // Check if pin is connected
            IPin* pTmp = nullptr;
            HRESULT hrConnect = pPin->ConnectedTo(&pTmp);

            if (hrConnect == VFW_E_NOT_CONNECTED) {
                // Found unconnected pin with matching direction
                *ppPin = pPin;
                pEnum->Release();
                std::cout << "[DEBUG] Found unconnected "
                    << (direction == PINDIR_OUTPUT ? "output" : "input")
                    << " pin\n";
                return S_OK;
            }

            if (pTmp) {
                pTmp->Release();
            }
        }
        pPin->Release();
    }

    pEnum->Release();
    std::cout << "[DEBUG] No unconnected pins found for specified direction\n";
    return E_FAIL;
}

HRESULT WebcamCapture::ConnectFilters(IGraphBuilder* pGraph, IBaseFilter* pSource, IBaseFilter* pDest) {
    IPin* pOut = NULL, * pIn = NULL;
    HRESULT hr = GetUnconnectedPin(pSource, PINDIR_OUTPUT, &pOut);
    if (SUCCEEDED(hr)) {
        hr = GetUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
        if (SUCCEEDED(hr)) {
            hr = pGraph->Connect(pOut, pIn);
            pIn->Release();
        }
        pOut->Release();
    }
    return hr;
}

// Helper function to get pin
IPin* WebcamCapture::GetPin(IBaseFilter* pFilter, PIN_DIRECTION PinDir) {
    IEnumPins* pEnum = NULL;
    IPin* pPin = NULL;

    if (FAILED(pFilter->EnumPins(&pEnum))) {
        return NULL;
    }

    while (pEnum->Next(1, &pPin, NULL) == S_OK) {
        PIN_DIRECTION PinDirThis;
        pPin->QueryDirection(&PinDirThis);
        if (PinDir == PinDirThis) {
            pEnum->Release();
            return pPin;
        }
        pPin->Release();
    }
    pEnum->Release();
    return NULL;
}

void WebcamCapture::DeleteMediaType(AM_MEDIA_TYPE* pmt) {
    if (pmt == NULL) {
        return;
    }

    if (pmt->cbFormat != 0) {
        CoTaskMemFree(pmt->pbFormat);
        pmt->cbFormat = 0;
        pmt->pbFormat = NULL;
    }

    if (pmt->pUnk != NULL) {
        pmt->pUnk->Release();
        pmt->pUnk = NULL;
    }

    CoTaskMemFree(pmt);
}

// Helper function to get encoder CLSID
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) return -1;

    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}

wstring WebcamCapture::StringToWString(const string& str) {
    if (str.empty()) return wstring();

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);

    return wstrTo;
}

bool WebcamCapture::captureImage(const char* filename) {
    std::cout << "[DEBUG] Starting webcam capture using Media Foundation...\n";

    if (!filename) {
        std::cout << "[ERROR] Invalid filename\n";
        return false;
    }

    HRESULT hr = S_OK;
    bool success = false;
    IMFSourceReader* pReader = NULL;
    IMFMediaSource* pSource = NULL;
    IMFAttributes* pAttributes = NULL;
    IMFMediaType* pMediaType = NULL;

    // Initialize Media Foundation
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        std::cout << "[ERROR] MFStartup failed\n";
        return false;
    }

    // Add preview code here ↓
    IGraphBuilder* pPreviewGraph = NULL;
    ICaptureGraphBuilder2* pPreviewBuilder = NULL;
    IMediaControl* pPreviewControl = NULL;
    IBaseFilter* pCam = NULL;

    // Create preview components
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
        IID_IGraphBuilder, (void**)&pPreviewGraph);
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
            IID_ICaptureGraphBuilder2, (void**)&pPreviewBuilder);
        if (SUCCEEDED(hr)) {
            hr = pPreviewBuilder->SetFiltergraph(pPreviewGraph);
            if (SUCCEEDED(hr)) {
                // Create system device enumerator
                ICreateDevEnum* pSysDevEnum = NULL;
                hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                    IID_ICreateDevEnum, (void**)&pSysDevEnum);
                if (SUCCEEDED(hr)) {
                    // Create video input device enumerator
                    IEnumMoniker* pEnumMoniker = NULL;
                    hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);
                    if (SUCCEEDED(hr)) {
                        // Get first video device
                        IMoniker* pMoniker = NULL;
                        if (pEnumMoniker->Next(1, &pMoniker, NULL) == S_OK) {
                            // Bind to camera
                            hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pCam);
                            if (SUCCEEDED(hr)) {
                                // Add to graph and render preview
                                pPreviewGraph->AddFilter(pCam, L"WebCam");
                                pPreviewBuilder->RenderStream(&PIN_CATEGORY_PREVIEW,
                                    &MEDIATYPE_Video, pCam, NULL, NULL);

                                // Start preview
                                hr = pPreviewGraph->QueryInterface(IID_IMediaControl, (void**)&pPreviewControl);
                                if (SUCCEEDED(hr)) {
                                    pPreviewControl->Run();
                                    std::cout << "[DEBUG] Preview started - waiting 3 seconds...\n";
                                    Sleep(3000);
                                    pPreviewControl->Stop();
                                }
                            }
                            pMoniker->Release();
                        }
                        pEnumMoniker->Release();
                    }
                    pSysDevEnum->Release();
                }
            }
        }
    }

    // Cleanup preview
    if (pPreviewControl) pPreviewControl->Release();
    if (pCam) pCam->Release();
    if (pPreviewBuilder) pPreviewBuilder->Release();
    if (pPreviewGraph) pPreviewGraph->Release();

    do {
        // Create attributes for webcam
        hr = MFCreateAttributes(&pAttributes, 1);
        if (FAILED(hr)) break;

        // Request video capture devices
        hr = pAttributes->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        if (FAILED(hr)) break;

        // Enumerate devices
        IMFActivate** ppDevices = NULL;
        UINT32 deviceCount = 0;
        hr = MFEnumDeviceSources(pAttributes, &ppDevices, &deviceCount);
        if (FAILED(hr) || deviceCount == 0) {
            std::cout << "[ERROR] No webcam found\n";
            break;
        }
        std::cout << "[DEBUG] Found " << deviceCount << " webcam device(s)\n";

        // Get device name
        WCHAR* friendlyName = NULL;
        UINT32 nameLength = 0;
        hr = ppDevices[0]->GetAllocatedString(
            MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
            &friendlyName,
            &nameLength);
        if (SUCCEEDED(hr)) {
            std::wcout << L"[DEBUG] Using device: " << friendlyName << std::endl;
            CoTaskMemFree(friendlyName);
        }

        // Activate first device
        hr = ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pSource));
        std::cout << "[DEBUG] Device activation: 0x" << std::hex << hr << std::dec << "\n";
        if (FAILED(hr)) break;

        // Create source reader
        hr = MFCreateSourceReaderFromMediaSource(pSource, pAttributes, &pReader);
        std::cout << "[DEBUG] Reader creation: 0x" << std::hex << hr << std::dec << "\n";
        if (FAILED(hr)) break;

        // Enumerate and try available formats
        DWORD mediaTypeIndex = 0;
        bool formatFound = false;

        while (!formatFound && SUCCEEDED(hr)) {
            if (pMediaType) {
                pMediaType->Release();
                pMediaType = NULL;
            }

            hr = pReader->GetNativeMediaType(
                MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                mediaTypeIndex,
                &pMediaType);

            if (hr == MF_E_NO_MORE_TYPES) {
                std::cout << "[DEBUG] No more media types\n";
                break;
            }

            if (SUCCEEDED(hr)) {
                UINT32 width = 0, height = 0;
                hr = MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height);
                if (SUCCEEDED(hr)) {
                    std::cout << "[DEBUG] Trying format " << width << "x" << height << "\n";

                    // Try to set this format
                    hr = pReader->SetCurrentMediaType(
                        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                        NULL,
                        pMediaType);

                    if (SUCCEEDED(hr)) {
                        std::cout << "[DEBUG] Successfully set format\n";
                        formatFound = true;
                        break;
                    }
                }
            }
            mediaTypeIndex++;
        }

        if (!formatFound) {
            std::cout << "[ERROR] No compatible format found\n";
            break;
        }

        std::cout << "[DEBUG] Starting frame capture...\n";
        // Read frame
        IMFSample* pSample = NULL;
        DWORD streamIndex, flags;
        LONGLONG timestamp;

        std::cout << "[DEBUG] Waiting for frame...\n";
        
        // Try multiple times if needed
        for (int attempts = 0; attempts < 3 && !pSample; attempts++) {
            hr = pReader->ReadSample(
                MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                0,                // No flags
                &streamIndex,     // Receives actual stream index
                &flags,          // Receives status flags
                &timestamp,      // Receives timestamp
                &pSample);       // Receives sample

            std::cout << "[DEBUG] ReadSample attempt " << attempts + 1 
                     << " result: 0x" << std::hex << hr 
                     << std::dec << ", Flags: " << flags
                     << ", Sample: " << (pSample ? "Valid" : "NULL") << "\n";

            if (SUCCEEDED(hr) && pSample) break;
            Sleep(100);  // Wait before retry
        }

        std::cout << "[DEBUG] Got sample, retrieving buffer...\n";
        // Get buffer from sample
        IMFMediaBuffer* pBuffer = NULL;
        hr = pSample->GetBufferByIndex(0, &pBuffer);
        std::cout << "[DEBUG] GetBuffer result: 0x" << std::hex << hr << std::dec << "\n";
        if (FAILED(hr)) break;

        // Initialize GDI+ at the beginning of the scope
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

        // Lock buffer and get data
        BYTE* pData = NULL;
        DWORD maxLength = 0, currentLength = 0;
        hr = pBuffer->Lock(&pData, &maxLength, &currentLength);
        std::cout << "[DEBUG] Buffer lock result: 0x" << std::hex << hr
            << " Size: " << std::dec << currentLength << " bytes\n";

        if (SUCCEEDED(hr) && pData) {
            UINT32 width = 0, height = 0;
            hr = MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height);
            if (SUCCEEDED(hr)) {
                int rgbStride = ((width * 3 + 3) & ~3);
                int rgbSize = rgbStride * height;

                // Tạo buffer với lifetime dài hơn bitmap
                std::shared_ptr<RGBBuffer> rgbBuffer = std::make_shared<RGBBuffer>(rgbSize);
                BYTE* rgbData = rgbBuffer->getData();

                // Convert NV12 to RGB
                BYTE* yPlane = pData;
                BYTE* uvPlane = pData + (width * height);

                for (UINT32 y = 0; y < height; y++) {
                    for (UINT32 x = 0; x < width; x++) {
                        int yIndex = y * width + x;
                        int uvIndex = (y / 2) * (width / 2) * 2 + (x / 2) * 2;

                        int Y = yPlane[yIndex];
                        int U = uvPlane[uvIndex];
                        int V = uvPlane[uvIndex + 1];

                        int C = Y - 16;
                        int D = U - 128;
                        int E = V - 128;

                        int R = (298 * C + 409 * E + 128) >> 8;
                        int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
                        int B = (298 * C + 516 * D + 128) >> 8;

                        R = R < 0 ? 0 : (R > 255 ? 255 : R);
                        G = G < 0 ? 0 : (G > 255 ? 255 : G);
                        B = B < 0 ? 0 : (B > 255 ? 255 : B);

                        int rgbIndex = y * rgbStride + x * 3;
                        rgbData[rgbIndex] = (BYTE)B;
                        rgbData[rgbIndex + 1] = (BYTE)G;
                        rgbData[rgbIndex + 2] = (BYTE)R;
                    }
                }

                // Initialize GDI+
                Gdiplus::GdiplusStartupInput gdiplusStartupInput;
                ULONG_PTR gdiplusToken;
                Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

                {
                    // Tạo và sử dụng bitmap
                    Gdiplus::Bitmap bitmap(width, height, rgbStride,
                        PixelFormat24bppRGB, rgbData);

                    CLSID encoderClsid;
                    if (GetEncoderClsid(L"image/jpeg", &encoderClsid) != -1) {
                        std::wstring wFilename(filename, filename + strlen(filename));
                        if (bitmap.Save(wFilename.c_str(), &encoderClsid) == Gdiplus::Ok) {
                            success = true;
                        }
                    }
                }

                Gdiplus::GdiplusShutdown(gdiplusToken);
            }
            pBuffer->Unlock();
        }

        // Cleanup
        if (pBuffer) pBuffer->Release();
        if (pSample) pSample->Release();

    } while (false);

    // Cleanup
    if (pMediaType) pMediaType->Release();
    if (pReader) pReader->Release();
    if (pSource) pSource->Release();
    if (pAttributes) pAttributes->Release();

    MFShutdown();
    return success;
}

bool WebcamCapture::captureVideo(const char* filename) {
    if (!filename) return false;
    cout << "[DEBUG] Starting video capture...\n";

    HRESULT hr = S_OK;
    IMFSinkWriter* pSinkWriter = NULL;
    IMFMediaSession* pSession = NULL;
    IMFTopology* pTopology = NULL;
    DWORD streamIndex = 0;
    PROPVARIANT varStart;
    PropVariantInit(&varStart);

    try {
        // Create media session for preview
        hr = MFCreateMediaSession(NULL, &pSession);
        if (FAILED(hr)) throw "Failed to create media session";

        // Create attributes
        hr = MFCreateAttributes(&pAttributes, 2);
        if (FAILED(hr)) throw "Failed to create attributes";

        hr = pAttributes->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        
        // Enumerate devices
        IMFActivate** ppDevices = NULL;
        UINT32 deviceCount = 0;
        hr = MFEnumDeviceSources(pAttributes, &ppDevices, &deviceCount);
        if (FAILED(hr) || deviceCount == 0) throw "No webcam found";

        // Activate webcam
        hr = ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pSource));
        if (FAILED(hr)) throw "Failed to activate webcam";

        // Create preview topology
        hr = MFCreateTopology(&pTopology);
        if (FAILED(hr)) throw "Failed to create topology";

        // Create EVR (Enhanced Video Renderer)
        IMFActivate* pActivate = NULL;
        hr = MFCreateVideoRendererActivate(GetDesktopWindow(), &pActivate);

        // Add source and sink to topology
        IMFTopologyNode* pSourceNode = NULL, * pOutputNode = NULL;
        hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pSourceNode);
        hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pOutputNode);

        // Configure source
        hr = pSourceNode->SetUnknown(MF_TOPONODE_SOURCE, pSource);
        hr = pOutputNode->SetObject(pActivate);

        hr = pTopology->AddNode(pSourceNode);
        hr = pTopology->AddNode(pOutputNode);
        hr = pSourceNode->ConnectOutput(0, pOutputNode, 0);

        // Start preview
        hr = pSession->Start(&GUID_NULL, &varStart);
        if (FAILED(hr)) throw "Failed to start preview";

        // Create media type for recording
        IMFMediaType* pVideoOutType = NULL;
        hr = MFCreateMediaType(&pVideoOutType);
        hr = pVideoOutType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        hr = pVideoOutType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
        hr = MFSetAttributeSize(pVideoOutType, MF_MT_FRAME_SIZE, 320, 240);
        hr = MFSetAttributeRatio(pVideoOutType, MF_MT_FRAME_RATE, 15, 1);
        hr = pVideoOutType->SetUINT32(MF_MT_AVG_BITRATE, 250000);

        // Create sink writer
        hr = MFCreateSinkWriterFromURL(
            StringToWString(filename).c_str(),
            NULL,
            NULL,
            &pSinkWriter);
        if (FAILED(hr)) throw "Failed to create sink writer";

        hr = pSinkWriter->AddStream(pVideoOutType, &streamIndex);
        hr = pSinkWriter->BeginWriting();

        // Record frames (15fps * 7 seconds = 105 frames)
        LONGLONG rtStart = 0;
        for (int i = 0; i < 105; i++) {
            IMFSample* pSample = NULL;
            DWORD flags = 0;
            hr = pReader->ReadSample(
                MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                0,
                NULL,
                &flags,
                NULL,
                &pSample);

            if (SUCCEEDED(hr) && pSample) {
                pSample->SetSampleTime(rtStart);
                rtStart += 666666; // 1/15 second
                hr = pSinkWriter->WriteSample(streamIndex, pSample);
                pSample->Release();
            }
        }

        // Cleanup
        hr = pSinkWriter->Finalize();
        hr = pSession->Stop();

        PropVariantClear(&varStart);
        if (pSession) pSession->Release();
        if (pTopology) pTopology->Release();
        if (pSinkWriter) pSinkWriter->Release();
        if (pVideoOutType) pVideoOutType->Release();
        if (pSourceNode) pSourceNode->Release();
        if (pOutputNode) pOutputNode->Release();
        if (pActivate) pActivate->Release();

        return true;
    }
    catch (const char* error) {
        cout << "Error: " << error << endl;
        return false;
    }
}

// Helper function to convert string to wstring
std::wstring StringToWString(const std::string& str) {
    std::wstring wstr(str.length(), L' ');
    std::copy(str.begin(), str.end(), wstr.begin());
    return wstr;
}