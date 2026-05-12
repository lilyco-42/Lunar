@echo off
setlocal
cd /d "%~dp0"

:: Source shared environment
call "%~dp0idf_env.bat"

if "%1"=="" goto :usage
if /I "%1"=="build"   goto :build
if /I "%1"=="flash"   goto :flash
if /I "%1"=="monitor" goto :monitor
if /I "%1"=="clean"   goto :clean
if /I "%1"=="menuconfig" goto :menuconfig
if /I "%1"=="size"    goto :size
if /I "%1"=="shell"   goto :shell
goto :usage

:build
"%PYTHON%" "%IDF_PATH%\tools\idf.py" build
goto :eof

:flash
if "%2"=="" (
    echo Usage: build.bat flash COMx
    echo Example: build.bat flash COM6
    goto :eof
)
"%PYTHON%" "%IDF_PATH%\tools\idf.py" -p %2 flash monitor
goto :eof

:monitor
if "%2"=="" (
    echo Usage: build.bat monitor COMx
    goto :eof
)
"%PYTHON%" "%IDF_PATH%\tools\idf.py" -p %2 monitor
goto :eof

:clean
"%PYTHON%" "%IDF_PATH%\tools\idf.py" fullclean
goto :eof

:menuconfig
"%PYTHON%" "%IDF_PATH%\tools\idf.py" menuconfig
goto :eof

:size
"%PYTHON%" "%IDF_PATH%\tools\idf.py" size-components
goto :eof

:shell
:: Open a persistent cmd shell with ESP-IDF environment
echo ESP-IDF v5.4 Environment
echo Project: %~dp0
echo.
echo   idf.py build          compile
echo   idf.py -p COM6 flash  flash + monitor
echo   exit                  quit
echo.
cmd /k "call "%~dp0idf_env.bat" && cd /d "%~dp0""
goto :eof

:usage
echo Lunar Voice - ESP32-C3 Build Tool
echo.
echo   build.bat build           编译固件
echo   build.bat flash COMx      烧录 + 串口监视
echo   build.bat monitor COMx    仅串口监视
echo   build.bat clean           清理编译产物
echo   build.bat menuconfig      配置菜单
echo   build.bat size            查看固件大小
echo   build.bat shell           打开 ESP-IDF 环境命令行
echo.
echo   示例: build.bat flash COM6
goto :eof
