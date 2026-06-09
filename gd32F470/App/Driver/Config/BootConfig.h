/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：BootConfig.h
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2026/2/4     V0.01    original
************************************************************/
#ifndef __BOOTCONFIG_H__
#define __BOOTCONFIG_H__

/************************* 头文件 *************************/

#include "HeaderFiles.h"
/************************* 宏定义 *************************/
#define BOOT_CONFIG_ADDR BOOT_PARAM_ADDR
/************************ 全局变量定义 ************************/

/* BootLoader 参数页，固定 256 字节。 */
typedef struct __attribute__((packed))
{
	// [0-15] 基础标识
	uint32_t magicWord;         // [0-3]   参数有效标识。BootLoader 只认可 APP_MAGIC_WORD
	uint16_t version;           // [4-5]   参数版本: BOOT_PARAM_VERSION
	uint16_t structSize;        // [6-7]   结构体大小: BOOT_PARAM_STRUCT_SIZE
	uint32_t buildDate;         // [8-11]  参数创建日期(BCD) 			        
	uint32_t reserved0;         // [12-15] 保留 						

	// [16-31] 升级控制
	uint8_t  updateFlag;        // [16]    升级标志: UPDATE_REQUEST_FLAG=请求升级, 0=正常运行
	uint8_t  updateMode;        // [17]    升级模式: UPDATE_MODE_UART=串口，后续可扩展 CAN/USB/网络
	uint8_t  updateStatus;      // [18]    升级状态: UPDATE_STATUS_WAIT_BOOTLOADER=等待搬运, UPDATE_STATUS_DONE=完成
	uint8_t  updateProgress;    // [19]    升级进度: 						
	uint32_t updateCount;       // [20-23] 升级次数累计						
	uint32_t lastUpdateTime;    // [24-27] 最后升级时间戳					
	uint32_t reserved1;         // [28-31] 保留								

	// [32-63] App固件信息
	uint32_t appSize;           // [32-35] 新 App 数据长度，由 App 串口接收完成后写入
	uint32_t appCRC32;          // [36-39] 新 App 数据 CRC32，BootLoader 搬运后用它校验
	uint32_t appVersion;        // [40-43] 新 App 版本号，也是触发备份判断的依据
	uint32_t appBuildDate;      // [44-47] App编译日期					
	uint32_t appStartAddr;      // [48-51] App起始地址，由 APP_START_ADDR 定义
	uint32_t appEntryAddr;      // [52-55] Reset_Handler 地址，从 App 向量表第 2 个字读取
	uint32_t appStackAddr;      // [56-59] MSP 初值，从 App 向量表第 1 个字读取
	uint32_t reserved2;         // [60-63] 保留							

	// [64-79] Bootloader信息
	uint32_t bootVersion;       // [64-67] Bootloader版本，默认 BOOTLOADER_VERSION
	uint32_t bootCRC32;         // [68-71] Bootloader CRC32				
	uint32_t bootSize;          // [72-75] Bootloader大小，默认 BOOTLOADER_SIZE
	uint32_t reserved3;         // [76-79] 保留 						

	// [80-111] 系统状态
	uint32_t runTimestamp;      // [80-83]  最后运行时间				
	uint32_t resetCount;        // [84-87]  总复位次数					
	uint16_t lastResetReason;   // [88-89]  最后复位原因				
	uint16_t bootFailCount;     // [90-91]  启动失败次数，达到阈值后 BootLoader 执行回退
	uint32_t totalRuntime;      // [92-95]  总运行时间(秒)				
	uint32_t wdtResetCount;     // [96-99]  看门狗复位次数				
	uint32_t hardFaultCount;    // [100-103] HardFault次数				
	uint32_t lastErrorCode;     // [104-107] 最后错误码						
	uint32_t reserved4;         // [108-111] 保留 						

	// [112-143] 备份固件信息
	uint32_t backupFlag;        // [112-115] 备份标志: 					
	uint32_t backupAddr;        // [116-119] 备份 App 地址，默认 BACKUP_APP_ADDR
	uint32_t backupSize;        // [120-123] 备份 App 大小，需与 Flash 分区规划保持一致
	uint32_t backupCRC32;       // [124-127] 当前可运行 App 的 CRC32，用于备份前校验
	uint32_t backupVersion;     // [128-131] 备份版本号
	uint32_t backupDate;        // [132-135] 备份时间  				
	uint32_t reserved5[2];      // [136-143] 保留 					

	// [144-159] 安全相关
	uint32_t securityFlag;      // [144-147] 安全标志 				
	uint32_t encryptKey;        // [148-151] 加密密钥索引 				
	uint32_t authCode;          // [152-155] 认证码 				
	uint32_t reserved6;         // [156-159] 保留 						

	// [160-207] 设备信息
	uint8_t  deviceID[16];      // [160-175] 设备唯一ID			"202601301528"
	uint8_t  productModel[16];  // [176-191] 产品型号			"000000000001"
	uint8_t  serialNumber[16];  // [192-207] 序列号				"100000000000"

	// [208-239] 硬件配置
	uint32_t hwVersion;         // [208-211] 硬件版本	  			1
	uint32_t cpuID;             // [212-215] CPU ID					1
	uint16_t flashSize;         // [216-217] Flash容量(KB)，默认 DEVICE_FLASH_SIZE_KB
	uint16_t ramSize;           // [218-219] RAM容量(KB)，默认 DEVICE_RAM_SIZE_KB
	uint32_t clockFreq;         // [220-223] 时钟频率(Hz)，默认 DEVICE_CLOCK_FREQ_HZ
	uint32_t reserved7[4];      // [224-239] 保留						

	// [240-255] 校验与结束
	uint32_t reserved8[2];      // [240-247] 保留 					
	uint32_t paramCRC32;        // [248-251] 整个参数区CRC32 ★ 			
	uint32_t tailMagic;         // [252-255] 尾部魔术字: BOOT_PARAM_TAIL_MAGIC

} BootParam_t;  // 总大小: BOOT_PARAM_STRUCT_SIZE 字节

/* 升级日志区，1024 字节。 */
typedef struct __attribute__((packed))
{
	uint32_t timestamp;         // 升级时间戳 							
	uint32_t oldVersion;        // 旧版本 								
	uint32_t newVersion;        // 新版本 								
	uint32_t newSize;           // 新固件大小 							
	uint32_t newCRC32;          // 新固件CRC 							
	uint8_t  status;            // 状态: 0x00=成功, 0xFF=失败 			
	uint8_t  mode;              // 模式: UPDATE_MODE_UART=串口, 0x02=CAN
	uint16_t duration;          // 耗时(秒) 							
	uint32_t errorCode;         // 错误码(失败时) 						
	uint8_t  expand[996];		// 保留 	
} UpdateLog_t;  //1024字节

/* 用户配置区，512 字节。 */
typedef struct __attribute__((packed))
{
	// [0-31] 通信配置
	uint32_t uart_baudrate;     // 串口波特率，默认 DEFAULT_UART_BAUDRATE
	uint8_t  uart_parity;       // 校验位 						0
	uint8_t  uart_stopbit;      // 停止位 						1
	uint16_t reserved_uart; 	//								0
	uint32_t can_baudrat;      // CAN波特率 - 保留 				0
	uint32_t eth_ip;            // 以太网IP - 保留 					0
	uint32_t eth_mask;          // 子网掩码 - 保留 					0
	uint32_t eth_gateway;       // 网关 - 保留 						0
	uint32_t reserved_comm[2];

	// [32-63] 功能开关
	uint32_t feature_flags;     // 功能标志位 - 保留 				0
	uint32_t debug_level;       // 调试级别 - 保留 					0
	uint32_t watchdog_timeout;  // 看门狗超时(ms) - 保留 			0
	uint32_t reserved_feat[5];

	// [64-127] 定时器配置
	uint32_t timer_intervals[16]; // 16个定时器周期 				0

	// [128-255] IO配置
	uint8_t  gpio_config[128];   // GPIO配置表 - 保留  				0

	// [256-383] 自定义参数
	uint8_t  user_data[128];     // 用户自定义数据 - 保留 				0

	// [384-511] 校验与保留
	uint8_t  reserved[124];		// 0
	uint32_t configCRC32;        // 配置区CRC32 					0	
} UserConfig_t;  // 512字节


/* 出厂校准区，512 字节。 */
typedef struct __attribute__((packed))
{
	// [0-15] 标识
	uint32_t calib_magic;       // 校准魔术字: 					0xCAC0FFEE
	uint32_t calib_date;        // 校准日期						0
	uint32_t calib_version;     // 校准版本						0
	uint32_t reserved0;

	// [16-47] ADC校准
	uint16_t adc_offset[16];    // ADC偏移校准 					0

	// [48-79] DAC校准 
	uint16_t dac_gain[16];      // DAC增益校准 					0

	// [80-143] 温度校准
	int16_t  temp_curve[32];    // 温度曲线校准点 				0

	// [144-207] 电压校准
	float    voltage_k[16];     // 电压斜率 					0

	// [208-271] 电流校准
	float    current_k[16];     // 电流斜率 					0

	// [272-507] 保留
	uint8_t  reserved[236]; 	//							    0

	// [508-511] 校验
	uint32_t calibCRC32;        // 校准数据CRC32 				0

} CalibData_t;  // 512字节

/************************ 函数定义 ************************/
void bootloader_config_init(BootParam_t* param , UpdateLog_t* mylog , UserConfig_t* userconfig , CalibData_t* calibdata);

#endif // !__BOOTCONFIG_H__

/****************************End*****************************/

