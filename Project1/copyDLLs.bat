@echo off
echo Copying DLLs...

set OUT_PATH=%~dp0x64\Debug

if not exist "%OUT_PATH%" mkdir "%OUT_PATH%"

echo Copying OpenCV DLLs...
xcopy /y /d "C:\vcpkg\packages\opencv4_x64-windows\debug\bin\*.dll" "%OUT_PATH%"

echo Copying dependencies...
xcopy /y /d "C:\vcpkg\packages\zlib_x64-windows\debug\bin\*.dll" "%OUT_PATH%"
copy /y "C:\vcpkg\packages\libwebp_x64-windows\debug\bin\libwebp.dll" "%OUT_PATH%"
copy /y "C:\vcpkg\packages\libwebp_x64-windows\debug\bin\libwebpdecoder.dll" "%OUT_PATH%"
xcopy /y /d "C:\vcpkg\packages\libjpeg-turbo_x64-windows\debug\bin\*.dll" "%OUT_PATH%"
xcopy /y /d "C:\vcpkg\packages\libpng_x64-windows\debug\bin\*.dll" "%OUT_PATH%"
xcopy /y /d "C:\vcpkg\packages\tiff_x64-windows\debug\bin\*.dll" "%OUT_PATH%"
xcopy /y /d "C:\vcpkg\packages\libpng_x64-windows\debug\bin\*.dll" "%OUT_PATH%"

echo DLL copy complete.
if errorlevel 1 echo Error copying files!