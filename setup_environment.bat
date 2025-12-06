@echo off
echo Setting up VS Code environment for MSVC development...

REM Create build directory if not exists
if not exist "build" mkdir build

REM Setup Visual Studio 2022 Environment
echo Setting up Visual Studio 2022 Environment...
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    echo Found VS 2022 Community
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" >nul 2>&1
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" (
    echo Found VS 2022 Professional
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" >nul 2>&1
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" (
    echo Found VS 2022 Enterprise
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" >nul 2>&1
) else (
    echo ERROR: Visual Studio 2022 not found!
    echo Please install Visual Studio 2022 with C++ development tools
    pause
    exit /b 1
)

REM Check for OpenCV installation - Extended paths
set OPENCV_FOUND=0
set "OPENCV_PATH="

echo Checking for OpenCV installation...

REM Check all common OpenCV installation paths
if exist "C:\Evironment_Code\opencv\build\include" (
    echo Found OpenCV at C:\Evironment_Code\opencv
    set "OPENCV_PATH=C:\Evironment_Code\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "C:\opencv\build\include" (
    echo Found OpenCV at C:\opencv
    set "OPENCV_PATH=C:\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "C:\Program Files\opencv\build\include" (
    echo Found OpenCV at C:\Program Files\opencv
    set "OPENCV_PATH=C:\Program Files\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "C:\Program Files (x86)\opencv\build\include" (
    echo Found OpenCV at C:\Program Files (x86)\opencv
    set "OPENCV_PATH=C:\Program Files (x86)\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "D:\opencv\build\include" (
    echo Found OpenCV at D:\opencv
    set "OPENCV_PATH=D:\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "E:\opencv\build\include" (
    echo Found OpenCV at E:\opencv
    set "OPENCV_PATH=E:\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "F:\opencv\build\include" (
    echo Found OpenCV at F:\opencv
    set "OPENCV_PATH=F:\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "C:\tools\opencv\build\include" (
    echo Found OpenCV at C:\tools\opencv
    set "OPENCV_PATH=C:\tools\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "C:\dev\opencv\build\include" (
    echo Found OpenCV at C:\dev\opencv
    set "OPENCV_PATH=C:\dev\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "C:\libs\opencv\build\include" (
    echo Found OpenCV at C:\libs\opencv
    set "OPENCV_PATH=C:\libs\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "C:\sdk\opencv\build\include" (
    echo Found OpenCV at C:\sdk\opencv
    set "OPENCV_PATH=C:\sdk\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "C:\external\opencv\build\include" (
    echo Found OpenCV at C:\external\opencv
    set "OPENCV_PATH=C:\external\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "C:\thirdparty\opencv\build\include" (
    echo Found OpenCV at C:\thirdparty\opencv
    set "OPENCV_PATH=C:\thirdparty\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "C:\dependencies\opencv\build\include" (
    echo Found OpenCV at C:\dependencies\opencv
    set "OPENCV_PATH=C:\dependencies\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "%USERPROFILE%\opencv\build\include" (
    echo Found OpenCV at %USERPROFILE%\opencv
    set "OPENCV_PATH=%USERPROFILE%\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "%LOCALAPPDATA%\opencv\build\include" (
    echo Found OpenCV at %LOCALAPPDATA%\opencv
    set "OPENCV_PATH=%LOCALAPPDATA%\opencv"
    set OPENCV_FOUND=1
    goto :opencv_setup
)
if exist "C:\vcpkg\installed\x64-windows\include\opencv2" (
    echo Found OpenCV via vcpkg
    set "OPENCV_PATH=C:\vcpkg\installed\x64-windows"
    set OPENCV_FOUND=1
    goto :opencv_setup
)

:opencv_setup
if "%OPENCV_FOUND%"=="0" (
    echo WARNING: OpenCV not found in standard locations
    echo Please install OpenCV or update the path in this script
    echo Supported paths:
    echo   - C:\Evironment_Code\opencv
    echo   - C:\opencv, D:\opencv, E:\opencv, F:\opencv
    echo   - C:\Program Files\opencv
    echo   - C:\Program Files ^(x86^)\opencv
    echo   - C:\tools\opencv, C:\dev\opencv, C:\libs\opencv
    echo   - C:\sdk\opencv, C:\external\opencv, C:\thirdparty\opencv
    echo   - C:\dependencies\opencv
    echo   - %USERPROFILE%\opencv
    echo   - %LOCALAPPDATA%\opencv
    echo   - C:\vcpkg\installed\x64-windows ^(via vcpkg^)
) else (
    echo OpenCV configured successfully
    if defined OPENCV_PATH (
        echo Setting OpenCV environment variables...
        if exist "%OPENCV_PATH%\build\x64\vc16\bin" (
            set "PATH=%OPENCV_PATH%\build\x64\vc16\bin;%PATH%"
            echo OpenCV DLLs added to PATH
        ) else if exist "%OPENCV_PATH%\bin" (
            set "PATH=%OPENCV_PATH%\bin;%PATH%"
            echo OpenCV DLLs added to PATH
        )
    )
)

echo Environment setup complete!
echo You can now use VS Code to build the project
