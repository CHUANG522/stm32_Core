@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion
title 多型号单片机工程一键深度清理脚本 STM/GD32 Keil+VSCode
cls

echo ==========================================================
echo          STM32标准库/HAL + GD32 通用清理工具
echo          支持 Keil MDK / VSCode / CubeMX / CubeIDE
echo ==========================================================
echo 安全说明：仅删除编译产物、缓存、临时文件
echo 不会删除 .c .h .s .ioc .uvprojx 源码与工程配置
echo ==========================================================
echo.

:: ======================================
:: 1. 全局递归删除所有编译中间文件
:: ======================================
echo [1/4] 清理全部编译中间文件...
del /s /q *.bak        >nul 2>&1
del /s /q *.tmp        >nul 2>&1
del /s /q *.obj        >nul 2>&1
del /s /q *.o          >nul 2>&1
del /s /q *.d          >nul 2>&1
del /s /q *.crf        >nul 2>&1
del /s /q *.lst        >nul 2>&1
del /s /q *.map        >nul 2>&1
del /s /q *.dep        >nul 2>&1
del /s /q *.lnp        >nul 2>&1
del /s /q *.plg        >nul 2>&1
del /s /q *.rpt        >nul 2>&1
del /s /q *.htm        >nul 2>&1
del /s /q *.tra        >nul 2>&1
del /s /q *.sct        >nul 2>&1
del /s /q *.iex        >nul 2>&1
del /s /q *.browse     >nul 2>&1
del /s /q *.lin        >nul 2>&1
del /s /q *.admin      >nul 2>&1
del /s /q *.dbgconf    >nul 2>&1
del /s /q *.build_log.htm >nul 2>&1

:: 调试日志、烧录日志
del /s /q JLinkLog.txt >nul 2>&1
del /s /q STLinkLog.txt >nul 2>&1

:: ======================================
:: 2. 可烧录固件文件 axf/hex/bin/elf
:: ======================================
echo [2/4] 清理固件输出文件...
del /s /q *.axf        >nul 2>&1
del /s /q *.hex        >nul 2>&1
del /s /q *.bin        >nul 2>&1
del /s /q *.elf        >nul 2>&1
del /s /q *.out        >nul 2>&1
del /s /q *.srec       >nul 2>&1

:: ======================================
:: 3. 各类缓存文件夹删除（Keil/GD32/CubeIDE/VSCode）
:: ======================================
echo [3/4] 清理编译输出&缓存目录...
:: Keil 原生输出目录
if exist "Listings"     rmdir /s /q "Listings"
if exist "Objects"      rmdir /s /q "Objects"
if exist "DebugConfig"  rmdir /s /q "DebugConfig"
if exist "RTE"          rmdir /s /q "RTE"

:: GD32 兆易工程专属输出目录
if exist "MDK_Output"   rmdir /s /q "MDK_Output"
if exist "GCC_Output"   rmdir /s /q "GCC_Output"

:: CubeIDE / GCC 编译目录
if exist "Debug"        rmdir /s /q "Debug"
if exist "Release"      rmdir /s /q "Release"
if exist "build"        rmdir /s /q "build"
if exist "cmake-build-*" rmdir /s /q cmake-build-*

:: VSCode 插件缓存
if exist ".vscode"      rmdir /s /q ".vscode"

:: CubeIDE 配置缓存
if exist ".settings"    rmdir /s /q ".settings"

:: 其他自定义输出文件夹
if exist "Output"       rmdir /s /q "Output"
if exist "BUILD"        rmdir /s /q "BUILD"

:: ======================================
:: 4. 可选：清理Keil个人界面配置（默认注释，按需开启）
:: ======================================
echo [4/4] 跳过Keil窗口布局文件（如需清理请手动打开下方注释）
:: del /s /q *.uvguix.*  >nul 2>&1
:: del /s /q *.uvoptx*   >nul 2>&1

:: 清理系统垃圾文件
del /s /q Thumbs.db     >nul 2>&1
del /s /q Desktop.ini   >nul 2>&1

echo.
echo ==========================================================
echo                    ✅ 全部清理完成
echo ==========================================================
echo 1. 编译产物、固件、日志、缓存目录已全部删除
echo 2. .c/.h/.s/.ioc/.uvprojx 源码工程文件完整保留
echo 3. 如需删除Keil窗口布局，打开脚本末尾两行注释重新运行
echo ==========================================================
pause >nul
exit