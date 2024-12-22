#pragma once
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "quartz.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfplay.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "PowrProf.lib")  
#pragma comment(lib, "Shcore.lib")

#define CURL_STATICLIB
#define _CRTDBG_MAP_ALLOC
#define ZLIB_WINAPI
#define DEBUG_LOG(msg) cout << "[DEBUG] " << msg << endl

using namespace std;


#include <json/json.h>
#include <curl/curl.h>
#include <iostream>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <tlhelp32.h>
#include <psapi.h>
#include <algorithm>

#include <limits>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include <eh.h>

#include <gdiplus.h>
#include <vector>
#include <filesystem>
#include <direct.h> 

#include <fstream>
#include <string>
#include <thread>

#include <chrono>
#include <cstdlib>

#include <locale>
#include <codecvt>
#include <sstream>

#include <stdlib.h>
#include <crtdbg.h>

#include <ctime>

#include <dshow.h>
#include <atlbase.h>
#include <atlconv.h>
#include <strmif.h>

#include <mferror.h>  
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <shlwapi.h>

#include <filesystem>

#include <winsvc.h>

#include <shlobj.h>
#include <system_error>

#include <iomanip>

#include <powrprof.h> 

#include <Guiddef.h>

#include <ShlObj.h>

#include <ShellScalingApi.h>