//#ifndef __USART1_H
//#define __USART1_H

//#include "gd32f4xx.h"
//#include <stdarg.h>
//#include <stdio.h>
//#include <string.h>
//#include <stdbool.h>

///*================ 引脚与参数配置 ================*/
//#define RS485_GPIO_CLK      RCU_GPIOD
//#define RS485_USART_CLK     RCU_USART1
//#define RS485_DIR_CLK       RCU_GPIOE

//#define RS485_TX_PORT       GPIOD
//#define RS485_TX_PIN        GPIO_PIN_6
//#define RS485_RX_PORT       GPIOD
//#define RS485_RX_PIN        GPIO_PIN_5
//#define RS485_GPIO_AF       GPIO_AF_7

//#define RS485_DIR_PORT      GPIOE
//#define RS485_DIR_PIN       GPIO_PIN_8

//#define RS485_USART         USART1
//#define RS485_IRQn          USART1_IRQn
//#define RS485_BAUDRATE      115200

//#define RS485_TX_MODE()     gpio_bit_set(RS485_DIR_PORT, RS485_DIR_PIN)
//#define RS485_RX_MODE()     gpio_bit_reset(RS485_DIR_PORT, RS485_DIR_PIN)

///*================ 环形缓冲区 ================*/
//#define USART1_RX_BUF_SIZE  8192

//typedef struct {
//    uint8_t  buf[USART1_RX_BUF_SIZE];
//    volatile uint16_t head;
//    volatile uint16_t tail;
//    volatile uint16_t count;
//    volatile uint8_t  overflow;   /* 新增：兼容 upgrade.c 的溢出检测 */
//} Usart1_RingBuf_t;

//extern Usart1_RingBuf_t g_usart1_ring;

///*================ 初始化与发送 ================*/
//void RS485_Init(void);
//void RS485_SendByte(uint8_t byte);
//void RS485_SendData(const uint8_t* data, uint16_t len);
//void RS485_Printf(const char* fmt, ...);

///*================ 环形缓冲区操作（新增，兼容 upgrade.c） ================*/
//void Usart1Ring_PutChar(uint8_t ch);        /* 中断里调用 */
//uint16_t Usart1Ring_GetCount(void);
//void Usart1Ring_Peek(uint8_t* dst, uint16_t len);
//void Usart1Ring_Skip(uint16_t len);

///* 以下新增，与 usart.c 的 Uart_xxx 接口对齐，供 upgrade.c 直接调用 */
//uint8_t  Usart1_ReadByte(uint8_t* byte);
//uint8_t  Usart1_RxOverflow(void);
//void     Usart1_RxReset(void);

//#endif
#ifndef __USART1_H
#define __USART1_H

#include "gd32f4xx.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define RS485_GPIO_CLK      RCU_GPIOD
#define RS485_USART_CLK     RCU_USART1
#define RS485_DIR_CLK       RCU_GPIOE

#define RS485_TX_PORT       GPIOD
#define RS485_TX_PIN        GPIO_PIN_6
#define RS485_RX_PORT       GPIOD
#define RS485_RX_PIN        GPIO_PIN_5
#define RS485_GPIO_AF       GPIO_AF_7

#define RS485_DIR_PORT      GPIOE
#define RS485_DIR_PIN       GPIO_PIN_8

#define RS485_USART         USART1
#define RS485_IRQn          USART1_IRQn
#define RS485_BAUDRATE      19200

#define RS485_TX_MODE()     gpio_bit_set(RS485_DIR_PORT, RS485_DIR_PIN)
#define RS485_RX_MODE()     gpio_bit_reset(RS485_DIR_PORT, RS485_DIR_PIN)

#define USART1_RX_BUF_SIZE  8192

typedef struct {
    uint8_t  buf[USART1_RX_BUF_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint8_t  overflow;
} Usart1_RingBuf_t;

extern Usart1_RingBuf_t g_usart1_ring;

void RS485_Init(void);
void RS485_SendByte(uint8_t byte);
void RS485_SendData(const uint8_t* data, uint16_t len);
void RS485_Printf(const char* fmt, ...);

void Usart1Ring_PutChar(uint8_t ch);
uint16_t Usart1Ring_GetCount(void);
void Usart1Ring_Peek(uint8_t* dst, uint16_t len);
void Usart1Ring_Skip(uint16_t len);

uint8_t  Usart1_ReadByte(uint8_t* byte);
uint8_t  Usart1_RxOverflow(void);
void     Usart1_RxReset(void);

uint32_t Baudrate_CodeToValue(uint8_t code);
void RS485_SetBaudrate(uint32_t baudrate);
void RS485_ReInit(uint32_t baudrate);
#endif

