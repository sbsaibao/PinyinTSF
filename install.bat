@echo off
REM ================================================================
REM  PinyinTSF Install Script
REM  Must be run as Administrator
REM ================================================================

echo ============================================
echo   PinyinTSF - Pinyin Tone Input Method
echo   Installation Script
echo ============================================
echo.

REM Check for admin rights
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] This script must be run as Administrator!
    echo Right-click and select "Run as administrator".
    pause
    exit /b 1
)

REM Determine DLL path
set "DLL_PATH=%~dp0bin\Release\PinyinTSF.dll"
if not exist "%DLL_PATH%" (
    set "DLL_PATH=%~dp0bin\Debug\PinyinTSF.dll"
)
if not exist "%DLL_PATH%" (
    echo [ERROR] PinyinTSF.dll not found!
    echo Please build the project first in Visual Studio.
    echo Expected location: bin\Release\PinyinTSF.dll
    echo                 or bin\Debug\PinyinTSF.dll
    pause
    exit /b 1
)

echo Found DLL: %DLL_PATH%
echo.

REM Register the DLL
echo Registering PinyinTSF.dll...
regsvr32 /s "%DLL_PATH%"
if %errorLevel% neq 0 (
    echo [ERROR] Registration failed!
    echo Make sure you are running as Administrator.
    pause
    exit /b 1
)

echo.
echo ============================================
echo   Installation successful!
echo ============================================
echo.
echo To use the input method:
echo   1. Open Windows Settings ^> Time ^& Language ^> Language
echo   2. Click your language ^> Options
echo   3. Add input method: "Pinyin Tone Input Method"
echo   4. Switch using Win+Space or language bar
echo.
pause
