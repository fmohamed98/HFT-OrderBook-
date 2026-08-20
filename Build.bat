@echo off
setlocal

set "SOLUTION=HFTStuff.sln"
set "CONFIG=Debug"
set "PLATFORM=x64"

:: Parse arguments
:parse_args
if "%~1"=="" goto find_vs
if /i "%~1"=="release" set "CONFIG=Release" & shift & goto parse_args
if /i "%~1"=="debug" set "CONFIG=Debug" & shift & goto parse_args
if /i "%~1"=="x86" set "PLATFORM=Win32" & shift & goto parse_args
if /i "%~1"=="x64" set "PLATFORM=x64" & shift & goto parse_args
if /i "%~1"=="open" goto open_vs
if /i "%~1"=="--help" goto usage
if /i "%~1"=="-h" goto usage
shift
goto parse_args

:find_vs
echo ============================================
echo  HFTStuff Build Script
echo ============================================
echo.
echo Configuration: %CONFIG%
echo Platform:      %PLATFORM%
echo.

:: Find vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found. Is Visual Studio installed?
    exit /b 1
)

:: Find VS installation path
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -property installationPath`) do set "VS_PATH=%%i"
if not defined VS_PATH (
    echo ERROR: Could not find a Visual Studio installation with MSBuild.
    exit /b 1
)

:: Find MSBuild
set "MSBUILD=%VS_PATH%\MSBuild\Current\Bin\MSBuild.exe"
if not exist "%MSBUILD%" (
    echo ERROR: MSBuild.exe not found at expected path.
    exit /b 1
)

echo Found Visual Studio at: %VS_PATH%
echo.

:: Build
echo Building %SOLUTION% ...
echo.
"%MSBUILD%" "%~dp0%SOLUTION%" /t:Build /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /m
if %errorlevel% neq 0 (
    echo.
    echo BUILD FAILED.
    exit /b 1
)

echo.
echo BUILD SUCCEEDED.
exit /b 0

:open_vs
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found. Is Visual Studio installed?
    exit /b 1
)
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property productPath`) do set "DEVENV=%%i"
if not defined DEVENV (
    echo ERROR: Could not find Visual Studio executable.
    exit /b 1
)
echo Opening %SOLUTION% in Visual Studio...
start "" "%DEVENV%" "%~dp0%SOLUTION%"
exit /b 0

:usage
echo.
echo Usage: Build.bat [options]
echo.
echo Options:
echo   debug       Build Debug configuration (default)
echo   release     Build Release configuration
echo   x64         Build for x64 platform (default)
echo   x86         Build for x86 platform
echo   open        Open the solution in Visual Studio
echo   --help, -h  Show this help message
echo.
echo Examples:
echo   Build.bat                 Build Debug x64
echo   Build.bat release         Build Release x64
echo   Build.bat release x86     Build Release x86
echo   Build.bat open            Open in Visual Studio
exit /b 0
