@echo off
REM ================================================================
REM  PinyinTSF Uninstall Script
REM  Must be run as Administrator
REM ================================================================

echo ============================================
echo   PinyinTSF - Pinyin Tone Input Method
echo   Uninstallation Script
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
    echo Cannot unregister without the DLL file.
    pause
    exit /b 1
)

echo Found DLL: %DLL_PATH%
echo.

REM Unregister the DLL
echo Unregistering PinyinTSF.dll...
regsvr32 /u /s "%DLL_PATH%"
if %errorLevel% neq 0 (
    echo [WARNING] Unregistration may have encountered issues.
) else (
    echo Unregistration successful.
)

echo.
echo ============================================
echo   Uninstallation complete!
echo ============================================
echo.
echo The input method has been removed from the system registry.
echo.

REM Existing applications may still have PinyinTSF.dll loaded.
REM COM/TSF unregistration only prevents future loads; it cannot unload
REM the DLL from already-running host processes such as Explorer or apps
REM where the input method was previously active.
tasklist /m PinyinTSF.dll 2^>nul | find /i "PinyinTSF.dll" >nul
if %errorLevel% equ 0 (
    echo [INFO] PinyinTSF.dll is still loaded by running process(es):
    echo.
    tasklist /m PinyinTSF.dll
    echo.
    echo Close the listed applications, or sign out/restart Windows,
    echo to remove any remaining tray icon and unload the old DLL.
    echo.

    tasklist /m PinyinTSF.dll 2^>nul | findstr /I /B "explorer.exe" >nul
    if %errorLevel% equ 0 (
        echo Windows Explorer is still using PinyinTSF.dll.
        choice /M "Restart Windows Explorer now to clear the tray icon"
        if %errorLevel% equ 1 (
            echo Restarting Windows Explorer...
            taskkill /f /im explorer.exe >nul 2>&1
            start explorer.exe
        )
    )
) else (
    echo No running process is currently using PinyinTSF.dll.
)
echo.
pause
