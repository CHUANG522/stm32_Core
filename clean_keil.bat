@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion
title STM32 Keil MDK 工程一键清理（增强版）

cls
echo ======================================================
echo          STM32CubeMX + Keil MDK 终极清理脚本
echo ======================================================
echo 【功能】清理编译文件、临时文件、日志、缓存
echo 【安全】不会删除 .c/.h/.ioc/.s 等工程源码
echo ======================================================
echo.

:: ===================== 清理文件 =====================
echo 正在删除编译中间文件...
del /s /q *.bak         >nul 2>&1
del /s /q *.tmp         >nul 2>&1
del /s /q *.obj         >nul 2>&1
del /s /q *.o           >nul 2>&1
del /s /q *.d           >nul 2>&1
del /s /q *.crf         >nul 2>&1
del /s /q *.lst         >nul 2>&1
del /s /q *.map         >nul 2>&1
del /s /q *.dep         >nul 2>&1
del /s /q *.lnp         >nul 2>&1
del /s /q *.plg         >nul 2>&1
del /s /q *.rpt         >nul 2>&1
del /s /q *.htm         >nul 2>&1
del /s /q JLinkLog.txt  >nul 2>&1

:: 保留工程配置，不删除调试/界面文件
:: del /s /q *.uvguix.*   >nul 2>&1
:: del /s /q *.uvoptx*    >nul 2>&1

:: ===================== 清理文件夹 =====================
echo 正在删除编译输出目录...
if exist "Listings" rmdir /s /q "Listings"
if exist "Objects" rmdir /s /q "Objects"
if exist "DebugConfig" rmdir /s /q "DebugConfig"
if exist "Output" rmdir /s /q "Output"
if exist "BUILD" rmdir /s /q "BUILD"

:: ===================== 清理生成文件 =====================
echo 正在删除可执行文件...
del /s /q *.axf         >nul 2>&1
del /s /q *.hex         >nul 2>&1
del /s /q *.bin         >nul 2>&1
del /s /q *.elf         >nul 2>&1

echo.
echo ======================================================
echo                ✅ 清理完成！工程已纯净
echo ======================================================
echo 提示：重新编译即可生成全新文件
pause >nul
exit