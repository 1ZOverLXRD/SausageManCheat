@echo off
REM 设置 ImGui 子模块到 vendor/imgui
set "VENDOR_DIR=%~dp0..\vendor"
set "IMGUI_DIR=%VENDOR_DIR%\imgui"

if not exist "%IMGUI_DIR%\imgui.cpp" (
    echo [setup] 下载 ImGui...
    if not exist "%VENDOR_DIR%" mkdir "%VENDOR_DIR%"
    cd /d "%VENDOR_DIR%"
    REM 用 curl 走代理下载 release
    curl -x http://127.0.0.1:7890 -L -o imgui.zip "https://github.com/ocornut/imgui/archive/refs/tags/v1.91.8.zip"
    tar -xf imgui.zip
    ren "imgui-1.91.8" "imgui"
    del imgui.zip
    cd /d "%~dp0"
    echo [setup] ImGui 下载完成
) else (
    echo [setup] ImGui 已存在，跳过
)

echo [setup] 完成！请用 CMake 生成项目：
echo   cd build
echo   cmake -G "Visual Studio 17 2022" -A x64 ..
echo [setup] 或者直接：
echo   cmake -B build -G "Visual Studio 17 2022" -A x64
pause