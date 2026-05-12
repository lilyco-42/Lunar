@echo off
:: ============================================================
:: Lunar — ESP-IDF Environment Setup
:: Auto-detects installation path, supports v5.3 / v5.4
:: ============================================================

:: Try standard ESP-IDF export script first
if exist "%USERPROFILE%\esp\esp-idf-v5.4\export.bat" (
    call "%USERPROFILE%\esp\esp-idf-v5.4\export.bat"
    goto :done
)
if exist "%USERPROFILE%\esp\esp-idf-v5.3\export.bat" (
    call "%USERPROFILE%\esp\esp-idf-v5.3\export.bat"
    goto :done
)

:: Fallback: manual path setup (if export.bat is unavailable)
if not defined IDF_PATH (
    if exist "%USERPROFILE%\esp\esp-idf-v5.4" set "IDF_PATH=%USERPROFILE%\esp\esp-idf-v5.4"
    if exist "%USERPROFILE%\esp\esp-idf-v5.3" set "IDF_PATH=%USERPROFILE%\esp\esp-idf-v5.3"
)

if not defined IDF_PATH (
    echo [ERROR] ESP-IDF not found. Install to %%USERPROFILE%%\esp\esp-idf-v5.4
    echo         https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/get-started/
    exit /b 1
)

:: Set Python from venv (may vary by version)
if not defined IDF_PYTHON_ENV_PATH (
    set "IDF_PYTHON_ENV_PATH=%USERPROFILE%\.espressif\python_env\idf5.4_py3.12_env"
)
if not defined IDF_TOOLS_PATH (
    set "IDF_TOOLS_PATH=%USERPROFILE%\.espressif"
)

set "PYTHON=%IDF_PYTHON_ENV_PATH%\Scripts\python.exe"
set "PATH=%IDF_TOOLS_PATH%\tools;%IDF_PATH%\tools;%IDF_PATH%\components\esptool_py\esptool;%PATH%"

:done
echo ESP-IDF ready: %IDF_PATH%
