@echo off
chcp 65001 > nul
title STM32 Keil MDK 工程一键清理工具

:: ======================================
:: 配置区：可根据你的工程结构修改路径
:: ======================================
set "OBJ_DIR=.\Objects"
set "LIST_DIR=.\Listings"
set "BUILD_DIR=.\build"
set "CUBE_MX_GENERATED=.\STM32CubeMX"
set "PROJ_ROOT=."

:: ======================================
:: 主程序
:: ======================================
cls
echo.
echo ==================================================
echo          STM32 Keil MDK 工程一键清理工具
echo ==================================================
echo 功能：清理编译文件、临时文件、日志、缓存
echo ==================================================
echo.

:: 1. 删除 Objects 目录（目标文件、可执行文件）
echo [1/5] 正在清理编译目标文件...
if exist "%OBJ_DIR%" (
    rmdir /s /q "%OBJ_DIR%" > nul 2>&1
    echo      - 已删除 %OBJ_DIR% 目录
) else (
    echo      - %OBJ_DIR% 目录不存在，跳过
)

:: 2. 删除 Listings 目录（列表、调试文件）
echo [2/5] 正在清理编译列表文件...
if exist "%LIST_DIR%" (
    rmdir /s /q "%LIST_DIR%" > nul 2>&1
    echo      - 已删除 %LIST_DIR% 目录
) else (
    echo      - %LIST_DIR% 目录不存在，跳过
)

:: 3. 删除 build 目录（VSCode/EIDE 构建目录）
echo [3/5] 正在清理构建缓存目录...
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%" > nul 2>&1
    echo      - 已删除 %BUILD_DIR% 目录
) else (
    echo      - %BUILD_DIR% 目录不存在，跳过
)

:: 4. 删除根目录下的临时文件
echo [4/5] 正在清理根目录临时文件...
del /f /q *.bak *.dep *.lst *.map *.o *.axf *.hex *.bin *.crf *.d *.scr *.sct *.lnp *.plg *.uvgui.* *.uvoptx *.uvprojx.bak > nul 2>&1
echo      - 已清理所有根目录临时文件

:: 5. 删除 STM32CubeMX 生成的临时文件（可选）
echo [5/5] 正在清理 STM32CubeMX 临时文件...
if exist "%CUBE_MX_GENERATED%\*.ioc.backup" (
    del /f /q "%CUBE_MX_GENERATED%\*.ioc.backup" > nul 2>&1
    echo      - 已删除 CubeMX 备份文件
) else (
    echo      - 无 CubeMX 备份文件，跳过
)

echo.
echo ==================================================
echo ✅ 清理完成！工程已恢复纯净状态
echo ==================================================
echo 提示：请重新编译工程以生成全新文件
echo.
pause