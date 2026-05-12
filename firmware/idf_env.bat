@echo off
:: ============================================================
:: ESP-IDF Environment Setup for ESP32-C3
:: Source this file in any cmd session: call idf_env.bat
:: ============================================================

set "IDF_PATH=%USERPROFILE%\esp\esp-idf-v5.4"
set "IDF_PYTHON_ENV_PATH=%USERPROFILE%\.espressif\python_env\idf5.4_py3.12_env"
set "IDF_TOOLS_PATH=%USERPROFILE%\.espressif"

set "TOOLS=%IDF_TOOLS_PATH%\tools"
set "PATH=%TOOLS%\riscv32-esp-elf\esp-14.2.0_20241119\riscv32-esp-elf\bin;%PATH%"
set "PATH=%TOOLS%\cmake\3.30.2\bin;%PATH%"
set "PATH=%TOOLS%\ninja\1.12.1;%PATH%"
set "PATH=%TOOLS%\idf-exe\1.0.3;%PATH%"
set "PATH=%IDF_PATH%\tools;%IDF_PATH%\components\esptool_py\esptool;%PATH%"

set "PYTHON=%IDF_PYTHON_ENV_PATH%\Scripts\python.exe"

echo ESP-IDF v5.4 - ESP32-C3
echo IDF_PATH=%IDF_PATH%
echo Python=%PYTHON%
echo.
