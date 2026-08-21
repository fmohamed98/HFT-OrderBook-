@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
set "CONFIG=Debug"
set "ARCH=x64"
set "ACTION=build"

:parse_args
if "%~1"=="" goto configure
if /I "%~1"=="debug"   set "CONFIG=Debug"   & shift & goto parse_args
if /I "%~1"=="release" set "CONFIG=Release" & shift & goto parse_args
if /I "%~1"=="x64"     set "ARCH=x64"      & shift & goto parse_args
if /I "%~1"=="x86"     set "ARCH=Win32"    & shift & goto parse_args
if /I "%~1"=="configure" set "ACTION=configure" & shift & goto parse_args
if /I "%~1"=="open"      set "ACTION=open"      & shift & goto parse_args
if /I "%~1"=="--help" goto usage
if /I "%~1"=="-h" goto usage
echo ERROR: Unknown option "%~1".
goto usage_error

:configure
set "BUILD_DIR=%ROOT%Build\vs2022-%ARCH%"
set "SOLUTION=%BUILD_DIR%\HFTStuff.sln"
set "PROJECT_FILE=%BUILD_DIR%\HFTStuff.vcxproj"

where cmake >nul 2>nul
if errorlevel 1 (
    echo ERROR: CMake was not found on PATH.
    exit /b 1
)

echo.
echo Configuring Visual Studio 2022 project...
echo   Architecture: %ARCH%
echo   Build folder: %BUILD_DIR%
:: Appending '.' prevents ROOT's trailing backslash from escaping the closing quote.
cmake -S "%ROOT%." -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A %ARCH%
if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed.
    echo If this folder was generated with a different generator, delete "%BUILD_DIR%" and run this script again.
    exit /b 1
)

if /I "%ACTION%"=="configure" (
    echo.
    echo Solution created: "%SOLUTION%"
    exit /b 0
)

if /I "%ACTION%"=="open" goto open_project

echo.
echo Building %CONFIG%...
cmake --build "%BUILD_DIR%" --config %CONFIG% --target HFTStuff
if errorlevel 1 (
    echo.
    echo BUILD FAILED.
    exit /b 1
)

echo.
echo BUILD SUCCEEDED.
echo Executable: "%BUILD_DIR%\bin\%CONFIG%\HFTStuff.exe"
exit /b 0

:open_project
if not exist "%PROJECT_FILE%" (
    echo ERROR: Project was not created: "%PROJECT_FILE%"
    exit /b 1
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -property productPath`) do set "DEVENV=%%I"
)

if defined DEVENV (
    start "" "%DEVENV%" "%PROJECT_FILE%"
) else (
    start "" "%PROJECT_FILE%"
)
exit /b 0

:usage
echo.
echo Usage: Build.bat [debug^|release] [x64^|x86] [configure^|open]
echo.
echo   Build.bat                 Configure and build Debug x64.
echo   Build.bat release         Configure and build Release x64.
echo   Build.bat configure       Generate the VS2022 solution only.
echo   Build.bat open            Generate and open only the HFTStuff project.
echo   Build.bat release x86     Configure and build Release Win32.
exit /b 0

:usage_error
exit /b 1
