/************************************************************
 * ©2026CIMC Copyright©
 * 文件名: USART1.c (改进版)
 * 说明: UART/RS485驱动层 - 修复收发时序问题
 * 作者: Copilot
 * 版本: V1.1 2026/06/06 - 修复第二次通信失败
************************************************************/

#include "USART1.h"

/*================ 环形缓冲区 ================*/
Usart1_RingBuf_t g_usart1_ring = {0};

/*================ 环形缓冲区操作 ================*/
void Usart1Ring_PutChar(uint8_t ch)
{
    uint16_t next = (g_usart1_ring.head + 1) % USART1_RX_BUF_SIZE;

    if (next == g_usart1_ring.tail) {
        g_usart1_ring.overflow = 1;
        return;
    }

    g_usart1_ring.buf[g_usart1_ring.head] = ch;
    g_usart1_ring.head = next;
    g_usart1_ring.count++;
}

uint16_t Usart1Ring_GetCount(void)
{
    return g_usart1_ring.count;
}

void Usart1Ring_Peek(uint8_t* dst, uint16_t len)
{
    uint16_t i;
    uint16_t temp_tail = g_usart1_ring.tail;
    for (i = 0; i < len && i < g_usart1_ring.count; i++) {
        dst[i] = g_usart1_ring.buf[temp_tail];
        temp_tail = (temp_tail + 1) % USART1_RX_BUF_SIZE;
    }
}

void Usart1Ring_Skip(uint16_t len)
{
    if (len > g_usart1_ring.count) len = g_usart1_ring.count;
    g_usart1_ring.tail = (g_usart1_ring.tail + len) % USART1_RX_BUF_SIZE;
    g_usart1_ring.count -= len;
}

uint8_t Usart1_ReadByte(uint8_t* byte)
{
    if (g_usart1_ring.head == g_usart1_ring.tail) {
        return 0;
    }

    *byte = g_usart1_ring.buf[g_usart1_ring.tail];
    g_usart1_ring.tail = (g_usart1_ring.tail + 1) % USART1_RX_BUF_SIZE;
    g_usart1_ring.count--;
    return 1;
}

uint8_t Usart1_RxOverflow(void)
{
    return g_usart1_ring.overflow;
}

void Usart1_RxReset(void)
{
    g_usart1_ring.head = 0;
    g_usart1_ring.tail = 0;
    g_usart1_ring.count = 0;
    g_usart1_ring.overflow = 0;
}

/*================ 初始化 ================*/
void RS485_Init(void)
{
    rcu_periph_clock_enable(RS485_GPIO_CLK);
    rcu_periph_clock_enable(RS485_DIR_CLK);
    rcu_periph_clock_enable(RS485_USART_CLK);

    gpio_af_set(RS485_TX_PORT, RS485_GPIO_AF, RS485_TX_PIN);
    gpio_af_set(RS485_RX_PORT, RS485_GPIO_AF, RS485_RX_PIN);

    gpio_mode_set(RS485_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, RS485_TX_PIN);
    gpio_output_options_set(RS485_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RS485_TX_PIN);

    gpio_mode_set(RS485_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, RS485_RX_PIN);
    gpio_output_options_set(RS485_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RS485_RX_PIN);

    /* 方向控制引脚 */
    gpio_mode_set(RS485_DIR_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, RS485_DIR_PIN);
    gpio_output_options_set(RS485_DIR_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RS485_DIR_PIN);
    RS485_RX_MODE();

    usart_deinit(RS485_USART);
    usart_word_length_set(RS485_USART, USART_WL_8BIT);
    usart_stop_bit_set(RS485_USART, USART_STB_1BIT);
    usart_parity_config(RS485_USART, USART_PM_NONE);
    usart_baudrate_set(RS485_USART, RS485_BAUDRATE);
    usart_receive_config(RS485_USART, USART_RECEIVE_ENABLE);
    usart_transmit_config(RS485_USART, USART_TRANSMIT_ENABLE);
    usart_hardware_flow_rts_config(RS485_USART, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(RS485_USART, USART_CTS_DISABLE);
    usart_enable(RS485_USART);

    nvic_irq_enable(RS485_IRQn, 3, 1);
    usart_interrupt_enable(RS485_USART, USART_INT_RBNE);

    memset(&g_usart1_ring, 0, sizeof(g_usart1_ring));
}

/*================ 发送接口 ================*/

/* 发送单个字节 */
void RS485_SendByte(uint8_t byte)
{
    RS485_TX_MODE();
    delay_1ms(1);  /* 给RS485芯片时间切换到发送模式 */
    
    usart_data_transmit(RS485_USART, byte);
    while (RESET == usart_flag_get(RS485_USART, USART_FLAG_TC));
    
    delay_1ms(1);  /* 等待最后一个bit发完 */
    RS485_RX_MODE();
    delay_1ms(1);  /* 给RS485芯片时间切换到接收模式 */
}

/* 发送多个字节 */
void RS485_SendData(const uint8_t* data, uint16_t len)
{
    uint16_t i;
    if ((data == NULL) || (len == 0U)) return;

    /* 切换到发送模式 */
    RS485_TX_MODE();
    delay_1ms(1);
    
    /* 逐字节发送 */
    for (i = 0U; i < len; i++) {
        usart_data_transmit(RS485_USART, data[i]);
        /* 等待该字节发送完成 */
        while (RESET == usart_flag_get(RS485_USART, USART_FLAG_TBE));
    }
    
    /* ========== 关键修复：等待所有字节真的发完 ========== */
    /* 检查发送完成标志（TC = Transmission Complete） */
    while (RESET == usart_flag_get(RS485_USART, USART_FLAG_TC));
    
    /* 增加一点延迟，确保最后一个bit已经离开发送器 */
    delay_1ms(2);
    
    /* 切换回接收模式 */
    RS485_RX_MODE();
    delay_1ms(1);  /* 给RS485芯片充分时间切换 */
}

/* Printf函数 */
void RS485_Printf(const char* fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    RS485_SendData((uint8_t*)buf, strlen(buf));
}

/*================ 中断处理 ================*/
void USART1_IRQHandler(void)
{
    if (RESET != usart_interrupt_flag_get(RS485_USART, USART_INT_FLAG_RBNE)) {
        uint8_t ch = (uint8_t)usart_data_receive(RS485_USART);
        Usart1Ring_PutChar(ch);
        usart_interrupt_flag_clear(RS485_USART, USART_INT_FLAG_RBNE);
    }
}

/****************************End*****************************/
