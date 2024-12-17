#pragma once
#include "..\Libs\Header.h"

#ifndef CLSID_NullRenderer
// {C1F400A4-3F08-11D3-9F0B-006008039E37}
DEFINE_GUID(CLSID_NullRenderer,
    0xc1f400a4, 0x3f08, 0x11d3, 0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37);
#endif

// Add CLSID definitions
// {8E14549A-DB61-4309-AFA1-3578E927E933}
DEFINE_GUID(CLSID_MP4Muxer,
    0x8e14549a, 0xdb61, 0x4309, 0xaf, 0xa1, 0x35, 0x78, 0xe9, 0x27, 0xe9, 0x33);

// {A2E3074F-6C3D-11D3-B653-00C04F79498E}
DEFINE_GUID(CLSID_FileSinkFilter,
    0xa2e3074f, 0x6c3d, 0x11d3, 0xb6, 0x53, 0x00, 0xc0, 0x4f, 0x79, 0x49, 0x8e);

static const GUID CLSID_ColorSpaceConverter =
{ 0x1643E180, 0x90F5, 0x11CE, { 0x97, 0xD5, 0x00, 0xAA, 0x00, 0x55, 0x59, 0x5A } };

using namespace Gdiplus;

class RGBBuffer {
public:
    RGBBuffer(int size) : bufferSize(size) {
        data = new BYTE[size];
    }
    ~RGBBuffer() {
        delete[] data;
    }
    BYTE* getData() { return data; }
private:
    BYTE* data;
    int bufferSize;
};

class WebcamCapture {
public:
    WebcamCapture();
    ~WebcamCapture();

    bool captureImage(const char* filename);
	bool captureVideo(const char* filename);

private:
    IMFSourceReader* pReader;
    IMFMediaSource* pSource;
    IMFAttributes* pAttributes;
    IMFMediaType* pMediaType;
    HRESULT GetUnconnectedPin(IBaseFilter* pFilter, PIN_DIRECTION direction, IPin** ppPin);
    HRESULT ConnectFilters(IGraphBuilder* pGraph, IBaseFilter* pSource, IBaseFilter* pDest);
    IPin *GetPin(IBaseFilter* pFilter, PIN_DIRECTION PinDir);
    void DeleteMediaType(AM_MEDIA_TYPE* pmt);
    wstring StringToWString(const string& str);
};
