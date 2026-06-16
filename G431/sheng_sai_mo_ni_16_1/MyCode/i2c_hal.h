#ifndef __I2C_HAL_H
#define __I2C_HAL_H

#include "stm32g4xx_hal.h"

void I2CStart(void);
void I2CStop(void);
unsigned char I2CWaitAck(void);
void I2CSendAck(void);
void I2CSendNotAck(void);
void I2CSendByte(unsigned char cSendByte);
unsigned char I2CReceiveByte(void);
void I2CInit(void);

extern uint8_t eeprom_data; // Header文件包含了此.h文件，则在所有文件中都能使用此变量
uint8_t eeprom_read(uint8_t address);
void eeprom_write(uint8_t address, uint8_t data);
#endif
