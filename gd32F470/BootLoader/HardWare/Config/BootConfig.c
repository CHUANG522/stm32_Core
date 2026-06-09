/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：BootConfig.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2026/2/4     V0.01    original
************************************************************/

/************************* 头文件 *************************/
#include "BootConfig.h"
/************************************************************
 * Function :       bootloader_config_init
 * Comment  :       初始化BootLoader参数默认值
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2026-02-04 V0.01 original
************************************************************/
void bootloader_config_init(BootParam_t* param,
    UpdateLog_t* mylog,
    UserConfig_t* userconfig,
    CalibData_t* calibdata)
{
    BootParam_t  tmp_param = { 0 };
    UpdateLog_t  tmp_log = { 0 };
    UserConfig_t tmp_usercfg = { 0 };
    CalibData_t  tmp_calib = { 0 };

    tmp_param.magicWord = APP_MAGIC_WORD;
    tmp_param.version = BOOT_PARAM_VERSION;
    tmp_param.structSize = BOOT_PARAM_STRUCT_SIZE;
    tmp_param.updateMode = UPDATE_MODE_UART;
    tmp_param.appStartAddr = APP_START_ADDR;
    tmp_param.appEntryAddr = *(__IO uint32_t*)(tmp_param.appStartAddr + 4);
    tmp_param.appStackAddr = *(__IO uint32_t*)(tmp_param.appStartAddr + 0);
    tmp_param.bootVersion = BOOTLOADER_VERSION;
    tmp_param.bootSize = BOOTLOADER_SIZE;
    tmp_param.backupAddr = BACKUP_APP_ADDR;
    tmp_param.backupSize = BACKUP_APP_SIZE;

    memcpy(tmp_param.deviceID, "202601301528", 12);
    memcpy(tmp_param.productModel, "000000000001", 12);
    memcpy(tmp_param.serialNumber, "100000000000", 12);

    tmp_param.hwVersion = 1;
    tmp_param.cpuID = 1;
    tmp_param.flashSize = DEVICE_FLASH_SIZE_KB;
    tmp_param.ramSize = DEVICE_RAM_SIZE_KB;
    tmp_param.clockFreq = DEVICE_CLOCK_FREQ_HZ;
    tmp_param.tailMagic = BOOT_PARAM_TAIL_MAGIC;

    tmp_log.mode = UPDATE_MODE_UART;

    tmp_usercfg.uart_baudrate = DEFAULT_UART_BAUDRATE;
    tmp_usercfg.uart_stopbit = 1;

    tmp_calib.calib_magic = 0xCAC0FFEE;

    *param = tmp_param;
    *mylog = tmp_log;
    *userconfig = tmp_usercfg;
    *calibdata = tmp_calib;
}
