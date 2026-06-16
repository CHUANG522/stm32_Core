/**
 * CT117E-M4 / GPIO - I2C
 */

#include "i2c_hal.h"

#define DELAY_TIME 20

//
void SDA_Input_Mode()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = GPIO_PIN_7;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
}

//
void SDA_Output_Mode()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = GPIO_PIN_7;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
}

//
void SDA_Output(uint16_t val)
{
    if (val)
    {
        GPIOB->BSRR |= GPIO_PIN_7;
    }
    else
    {
        GPIOB->BRR |= GPIO_PIN_7;
    }
}

//
void SCL_Output(uint16_t val)
{
    if (val)
    {
        GPIOB->BSRR |= GPIO_PIN_6;
    }
    else
    {
        GPIOB->BRR |= GPIO_PIN_6;
    }
}

//
uint8_t SDA_Input(void)
{
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_SET)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//
static void delay1(volatile unsigned int n)
{
    volatile uint32_t i;
    for (i = 0; i < n; ++i)
        ;
}

//
void I2CStart(void)
{
    SDA_Output(1);
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    SDA_Output(0);
    delay1(DELAY_TIME);
    SCL_Output(0);
    delay1(DELAY_TIME);
}

//
void I2CStop(void)
{
    SCL_Output(0);
    delay1(DELAY_TIME);
    SDA_Output(0);
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    SDA_Output(1);
    delay1(DELAY_TIME);
}

//
unsigned char I2CWaitAck(void)
{
    unsigned short cErrTime = 5;
    SDA_Input_Mode();
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    while (SDA_Input())
    {
        cErrTime--;
        delay1(DELAY_TIME);
        if (0 == cErrTime)
        {
            SDA_Output_Mode();
            I2CStop();
            return ERROR;
        }
    }
    SCL_Output(0);
    SDA_Output_Mode();
    delay1(DELAY_TIME);
    return SUCCESS;
}

//
void I2CSendAck(void)
{
    SDA_Output(0);
    delay1(DELAY_TIME);
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    SCL_Output(0);
    delay1(DELAY_TIME);
}

//
void I2CSendNotAck(void)
{
    SDA_Output(1);
    delay1(DELAY_TIME);
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    SCL_Output(0);
    delay1(DELAY_TIME);
}

//
void I2CSendByte(unsigned char cSendByte)
{
    unsigned char i = 8;
    while (i--)
    {
        SCL_Output(0);
        delay1(DELAY_TIME);
        SDA_Output(cSendByte & 0x80);
        delay1(DELAY_TIME);
        cSendByte += cSendByte;
        delay1(DELAY_TIME);
        SCL_Output(1);
        delay1(DELAY_TIME);
    }
    SCL_Output(0);
    delay1(DELAY_TIME);
}

//
unsigned char I2CReceiveByte(void)
{
    unsigned char i = 8;
    unsigned char cR_Byte = 0;
    SDA_Input_Mode();
    while (i--)
    {
        cR_Byte += cR_Byte;
        SCL_Output(0);
        delay1(DELAY_TIME);
        delay1(DELAY_TIME);
        SCL_Output(1);
        delay1(DELAY_TIME);
        cR_Byte |= SDA_Input();
    }
    SCL_Output(0);
    delay1(DELAY_TIME);
    SDA_Output_Mode();
    return cR_Byte;
}

//
void I2CInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = GPIO_PIN_7 | GPIO_PIN_6;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
}
void eeprom_write(uint8_t address, uint8_t data)
{
    I2CStart();
    I2CSendByte(0xa0); // 地址尾部为0是写模式
    I2CWaitAck();

    I2CSendByte(address);
    I2CWaitAck();

    I2CSendByte(data);
    I2CWaitAck();
    I2CStop();
    HAL_Delay(10); // 加个延时，防止时序有问题
}
uint8_t eeprom_data;                 // 在此处定义了eeprom数据，在i2c.h文件中声明了此数据
uint8_t eeprom_read(uint8_t address) // 注意此地址是整型，eeprom中内部存储地址本质就是一串数字编号，为整数索引
{
    I2CStart();        // 启动
    I2CSendByte(0xa0); // 地址尾部为0是写模式
    I2CWaitAck();      // 每次操作后都要等待

    I2CSendByte(address);
    I2CWaitAck();
    I2CStop(); // 每次读取数据时要先开启写模式，找到其地址，且要关闭写模式

    I2CStart();        // 写模式完成重新开启读取模式
    I2CSendByte(0xa1); // 地址尾部为0是写模式，1为读取模式
    I2CWaitAck();

    eeprom_data = I2CReceiveByte(); // 已经在address地址处，调用接受函数获取当前数据
    I2CWaitAck();

    I2CStop(); // 读取模式结束
    return eeprom_data;
}
// 练习
// void eeprom_write(uint8_t address, uint8_t data)
// {

//     I2CStart();
//     I2CSendByte(0xa0);
//     I2CWaitAck();

//     I2CSendByte(address);
//     I2CWaitAck();

//     I2CSendByte(data);
//     I2CWaitAck();
//     I2CStop();
//     HAL_Delay(10);忘记加延时
// }
// uint8_t eeprom_read(uint8_t address)
// {//函数类型写错

//     I2CStart();
//     I2CSendByte(0xa0);
//     I2CWaitAck();

//     I2CSendByte(address);
//     I2CWaitAck();
//     I2CStop();

//     I2CStart();
//     I2CSendByte(0xa1);
//     eeprom_data = I2CReceiveByte();
//     I2CWaitAck();读取后没有等待
//     I2CStop();
//     return eeprom_data;
// }
