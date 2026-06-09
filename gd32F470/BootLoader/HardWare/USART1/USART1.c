//#include "USART1.h"

///*================ 环形缓冲区定义 ================*/
//Usart1_RingBuf_t g_usart1_ring = {0};

///*================ 环形缓冲区操作 ================*/
//void Usart1Ring_PutChar(uint8_t ch)
//{
//    uint16_t next = (g_usart1_ring.head + 1) % USART1_RX_BUF_SIZE;

//    if (next == g_usart1_ring.tail) {
//        g_usart1_ring.overflow = 1;     /* 满了，标记溢出，丢弃新数据 */
//        return;
//    }

//    g_usart1_ring.buf[g_usart1_ring.head] = ch;
//    g_usart1_ring.head = next;
//    g_usart1_ring.count++;
//}

//uint16_t Usart1Ring_GetCount(void)
//{
//    return g_usart1_ring.count;
//}

//void Usart1Ring_Peek(uint8_t* dst, uint16_t len)
//{
//    uint16_t i;
//    uint16_t temp_tail = g_usart1_ring.tail;
//    for (i = 0; i < len && i < g_usart1_ring.count; i++) {
//        dst[i] = g_usart1_ring.buf[temp_tail];
//        temp_tail = (temp_tail + 1) % USART1_RX_BUF_SIZE;
//    }
//}

//void Usart1Ring_Skip(uint16_t len)
//{
//    if (len > g_usart1_ring.count) len = g_usart1_ring.count;
//    g_usart1_ring.tail = (g_usart1_ring.tail + len) % USART1_RX_BUF_SIZE;
//    g_usart1_ring.count -= len;
//}

///*================ 新增：兼容 upgrade.c 的读取接口 ================*/
//uint8_t Usart1_ReadByte(uint8_t* byte)
//{
//    if (g_usart1_ring.head == g_usart1_ring.tail) {
//        return 0;   /* 无数据 */
//    }

//    *byte = g_usart1_ring.buf[g_usart1_ring.tail];
//    g_usart1_ring.tail = (g_usart1_ring.tail + 1) % USART1_RX_BUF_SIZE;
//    g_usart1_ring.count--;
//    return 1;
//}

//uint8_t Usart1_RxOverflow(void)
//{
//    return g_usart1_ring.overflow;
//}

//void Usart1_RxReset(void)
//{
//    g_usart1_ring.head = 0;
//    g_usart1_ring.tail = 0;
//    g_usart1_ring.count = 0;
//    g_usart1_ring.overflow = 0;
//}

///*================ 初始化 ================*/
//void RS485_Init(void)
//{
//    rcu_periph_clock_enable(RS485_GPIO_CLK);
//    rcu_periph_clock_enable(RS485_DIR_CLK);
//    rcu_periph_clock_enable(RS485_USART_CLK);

//    gpio_af_set(RS485_TX_PORT, RS485_GPIO_AF, RS485_TX_PIN);
//    gpio_af_set(RS485_RX_PORT, RS485_GPIO_AF, RS485_RX_PIN);

//    gpio_mode_set(RS485_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, RS485_TX_PIN);
//    gpio_output_options_set(RS485_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RS485_TX_PIN);

//    gpio_mode_set(RS485_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, RS485_RX_PIN);
//    gpio_output_options_set(RS485_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RS485_RX_PIN);

//    /* 方向控制引脚 */
//    gpio_mode_set(RS485_DIR_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, RS485_DIR_PIN);
//    gpio_output_options_set(RS485_DIR_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RS485_DIR_PIN);
//    RS485_RX_MODE();    /* 默认接收模式 */

//    usart_deinit(RS485_USART);
//    usart_word_length_set(RS485_USART, USART_WL_8BIT);
//    usart_stop_bit_set(RS485_USART, USART_STB_1BIT);
//    usart_parity_config(RS485_USART, USART_PM_NONE);
//    usart_baudrate_set(RS485_USART, RS485_BAUDRATE);
//    usart_receive_config(RS485_USART, USART_RECEIVE_ENABLE);
//    usart_transmit_config(RS485_USART, USART_TRANSMIT_ENABLE);
//    usart_hardware_flow_rts_config(RS485_USART, USART_RTS_DISABLE);
//    usart_hardware_flow_cts_config(RS485_USART, USART_CTS_DISABLE);
//    usart_enable(RS485_USART);

//    nvic_irq_enable(RS485_IRQn, 3, 1);
//    usart_interrupt_enable(RS485_USART, USART_INT_RBNE);

//    memset(&g_usart1_ring, 0, sizeof(g_usart1_ring));
//}

///*================ 发送接口 ================*/
//void RS485_SendByte(uint8_t byte)
//{
//    RS485_TX_MODE();
//    usart_data_transmit(RS485_USART, byte);
//    while (RESET == usart_flag_get(RS485_USART, USART_FLAG_TC));
//    RS485_RX_MODE();
//}

//void RS485_SendData(const uint8_t* data, uint16_t len)
//{
//    uint16_t i;
//    if ((data == NULL) || (len == 0U)) return;

//    RS485_TX_MODE();
//    for (i = 0U; i < len; i++) {
//        usart_data_transmit(RS485_USART, data[i]);
//        while (RESET == usart_flag_get(RS485_USART, USART_FLAG_TBE));
//    }
//    while (RESET == usart_flag_get(RS485_USART, USART_FLAG_TC));
//    RS485_RX_MODE();
//}

//void RS485_Printf(const char* fmt, ...)
//{
//    char buf[128];
//    va_list args;
//    va_start(args, fmt);
//    vsnprintf(buf, sizeof(buf), fmt, args);
//    va_end(args);
//    RS485_SendData((uint8_t*)buf, strlen(buf));
//}

///*================ 中断服务函数 ================*/
//void USART1_IRQHandler(void)
//{
//    if (RESET != usart_interrupt_flag_get(RS485_USART, USART_INT_FLAG_RBNE)) {
//        uint8_t ch = (uint8_t)usart_data_receive(RS485_USART);
//        Usart1Ring_PutChar(ch);
//        usart_interrupt_flag_clear(RS485_USART, USART_INT_FLAG_RBNE);
//    }
//}



//int fputc(int ch, FILE* f)
//{
//    RS485_SendByte((uint8_t)ch);
//    return ch;
//}

///****************************End*****************************/

#include "USART1.h"
#include "LED.h"

Usart1_RingBuf_t g_usart1_ring = {0};

void Usart1Ring_PutChar(uint8_t ch)
{
    uint16_t next = (g_usart1_ring.head + 1) % USART1_RX_BUF_SIZE;
    if (next == g_usart1_ring.tail) {
        g_usart1_ring.overflow = 1;
        return;
    }
    g_usart1_ring.buf[g_usart1_ring.head] = ch;
    g_usart1_ring.head = next;
}

uint16_t Usart1Ring_GetCount(void)
{
    return (g_usart1_ring.head + USART1_RX_BUF_SIZE - g_usart1_ring.tail) % USART1_RX_BUF_SIZE;
}

void Usart1Ring_Peek(uint8_t* dst, uint16_t len)
{
    uint16_t i;
    uint16_t temp_tail = g_usart1_ring.tail;
    uint16_t count = Usart1Ring_GetCount();
    for (i = 0; i < len && i < count; i++) {
        dst[i] = g_usart1_ring.buf[temp_tail];
        temp_tail = (temp_tail + 1) % USART1_RX_BUF_SIZE;
    }
}

void Usart1Ring_Skip(uint16_t len)
{
    uint16_t count = Usart1Ring_GetCount();
    if (len > count) len = count;
    g_usart1_ring.tail = (g_usart1_ring.tail + len) % USART1_RX_BUF_SIZE;
}

uint8_t Usart1_ReadByte(uint8_t* byte)
{
    if (g_usart1_ring.head == g_usart1_ring.tail) {
        return 0;
    }
    *byte = g_usart1_ring.buf[g_usart1_ring.tail];
    g_usart1_ring.tail = (g_usart1_ring.tail + 1) % USART1_RX_BUF_SIZE;
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
    g_usart1_ring.overflow = 0;
}

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

void RS485_SendByte(uint8_t byte)
{
    RS485_TX_MODE();
    usart_data_transmit(RS485_USART, byte);
    while (RESET == usart_flag_get(RS485_USART, USART_FLAG_TC));
    RS485_RX_MODE();
}

void RS485_SendData(const uint8_t* data, uint16_t len)
{
    uint16_t i;
    if ((data == NULL) || (len == 0U)) return;

    RS485_TX_MODE();

    for (i = 0U; i < len; i++) {
        usart_data_transmit(RS485_USART, data[i]);
        while (RESET == usart_flag_get(RS485_USART, USART_FLAG_TBE));
    }

    /* 等最后一个字节真正发完 */
    while (RESET == usart_flag_get(RS485_USART, USART_FLAG_TC));

    /* === 关键修复：等 RS485 总线方向稳定 ===
     * Modbus RTU 标准要求帧间隔 >= 3.5 字符时间
     * 115200: 1字符=87us, 3.5字符=304us
     * 取 1ms 确保所有波特率下都稳定 */
	delay_1ms(5);  
	RS485_RX_MODE();
    
}




uint32_t Baudrate_CodeToValue(uint8_t code)
{
    switch (code)
    {
        case 0x11: return 4800;
        case 0x12: return 9600;
        case 0x13: return 19200;
        case 0x14: return 115200;
        default:   return 19200;   /* 异常回退默认 */
    }
}

/*================ 修改 USART1 波特率 =================*/
void RS485_ReInit(uint32_t baudrate)
{
    /* 1. 等当前发送完成 */
    while (RESET == usart_flag_get(RS485_USART, USART_FLAG_TC));
    
    /* 2. 切回接收，释放总线 */
    RS485_RX_MODE();
    delay_1ms(2);
    
    /* 3. 关接收中断 */
    usart_interrupt_disable(RS485_USART, USART_INT_RBNE);
    
    /* 4. 关键：必须先关 USART（UE=0），才能安全改波特率 */
    usart_disable(RS485_USART);
    
    /* 5. 改波特率（此时 UE=0，安全） */
    usart_baudrate_set(RS485_USART, baudrate);

    /* 6. 重新使能 USART */
    usart_enable(RS485_USART);
    
    /* 7. 重新打开接收和发送（usart_disable 后 RE/TE 位会丢失！） */
    usart_receive_config(RS485_USART, USART_RECEIVE_ENABLE);
    usart_transmit_config(RS485_USART, USART_TRANSMIT_ENABLE);
    
    /* 8. 清空缓冲区 + 清错误标志 */
    Usart1_RxReset();
    (void)usart_data_receive(RS485_USART);
    (void)usart_data_receive(RS485_USART);
    
    /* 9. 重新开中断 */
    usart_interrupt_enable(RS485_USART, USART_INT_RBNE);
}













void RS485_Printf(const char* fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    RS485_SendData((uint8_t*)buf, strlen(buf));
}

void USART1_IRQHandler(void)
{
    /* 必须先清错误标志，否则 ORE/FE 会永久阻塞 RBNE 中断 */
    if (usart_flag_get(RS485_USART, USART_FLAG_ORERR) != RESET) {
        (void)usart_data_receive(RS485_USART);  /* 读 DATA 清 ORE */
    }
    if (usart_flag_get(RS485_USART, USART_FLAG_FERR) != RESET) {
        (void)usart_data_receive(RS485_USART);  /* 读 DATA 清 FE */
    }
    if (usart_flag_get(RS485_USART, USART_FLAG_NERR) != RESET) {
        (void)usart_data_receive(RS485_USART);  /* 读 DATA 清 NE */
    }
    
    /* 正常接收 */
    if (usart_interrupt_flag_get(RS485_USART, USART_INT_FLAG_RBNE) != RESET) {
        uint8_t ch = (uint8_t)usart_data_receive(RS485_USART);
        Usart1Ring_PutChar(ch);
        /* 不要手动 clear RBNE，读 DR 已自动清除 */
    }
}


/* 删除 fputc 重定向！防止 printf/sprintf 等库函数偷偷往 RS485 发数据 */
#if 0
int fputc(int ch, FILE* f)
{
    RS485_SendByte((uint8_t)ch);
    return ch;
}
#endif

