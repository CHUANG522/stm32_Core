/************************************************************
 * 文件：flash_layout.h
 * 说明：BootLoader 侧 Flash 分区常量
************************************************************/

#ifndef __FLASH_LAYOUT_H__
#define __FLASH_LAYOUT_H__

/************************* 宏定义 *************************/

#define FLASH_PAGE_SIZE                 (4 * 1024)

#define APP_MAGIC_WORD                  0x5AA5C33C
#define BOOT_PARAM_VERSION              0x0001
#define BOOT_PARAM_TAIL_MAGIC           0xA5A5C3C3
#define UPDATE_MODE_UART                0x01
#define UPDATE_REQUEST_FLAG             0x5A
#define UPDATE_STATUS_WAIT_BOOTLOADER   0x01
#define UPDATE_STATUS_DONE              0x00

#define BOOT_PARAM_ADDR                 0x08010000
#define BOOT_PARAM_SIZE                 (4 * 1024)
#define BOOT_PARAM_RESERVED_ADDR        0x08010100
#define BOOT_PARAM_STRUCT_SIZE          256

#define BOOTLOADER_VERSION              0x01
#define BOOTLOADER_SIZE                 4096
#define DEVICE_FLASH_SIZE_KB            0x400
#define DEVICE_RAM_SIZE_KB              0x2F
#define DEVICE_CLOCK_FREQ_HZ            240000000
#define DEFAULT_UART_BAUDRATE           115200

#define APP_START_ADDR                  0x08011000
#define APP_MAX_SIZE                    (128 * 1024)
#define APP_PAGE_COUNT                  (APP_MAX_SIZE / FLASH_PAGE_SIZE)

#define BACKUP_APP_ADDR                 0x08031000
#define BACKUP_APP_SIZE                 (128 * 1024)
#define BACKUP_APP_PAGE_COUNT           (BACKUP_APP_SIZE / FLASH_PAGE_SIZE)

#define DOWNLOAD_AREA_ADDR              0x08051000
#define DOWNLOAD_AREA_SIZE              (128 * 1024)
#define DOWNLOAD_AREA_PAGE_COUNT        (DOWNLOAD_AREA_SIZE / FLASH_PAGE_SIZE)
#define BOOT_TRANSFER_BUFFER_SIZE       (20 * 1024)

#endif

/****************************End*****************************/
