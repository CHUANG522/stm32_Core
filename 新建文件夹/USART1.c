//#include "USART1.h"

///*================ 兼容上层命令处理的全局变量 ================*/
//bool Rxflag = 0;
//char RXData[100];

//static uint8_t pRead = 0;

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
//    RS485_RX_MODE();

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

//    if ((data == NULL) || (len == 0U))
//    {
//        return;
//    }

//    RS485_TX_MODE();
//    for (i = 0U; i < len; i++)
//    {
//        usart_data_transmit(RS485_USART, data[i]);
//        while (RESET == usart_flag_get(RS485_USART, USART_FLAG_TBE));
//    }

//    while (RESET == usart_flag_get(RS485_USART, USART_FLAG_TC));
//    RS485_RX_MODE();
//}



///*================ 中断服务函数（最简版：直接收字符到 RXData） ================*/
//void USART1_IRQHandler(void)
//{
//    if (RESET != usart_interrupt_flag_get(RS485_USART, USART_INT_FLAG_RBNE))
//    {
//        uint8_t ch = (uint8_t)usart_data_receive(RS485_USART);

//        if (Rxflag == 0)
//        {
//            if (ch == '\n' || pRead >= 99)  // 只认 \n 结束
//            {
//                RXData[pRead] = '\0';
//                pRead = 0;
//                Rxflag = 1;
//            }
//            else if (ch != '\r')  // 忽略 \r，不存入
//            {
//                RXData[pRead++] = ch;
//            }
//        }

//        usart_interrupt_flag_clear(RS485_USART, USART_INT_FLAG_RBNE);
//    }
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

////int fputc(int ch, FILE* f)
////{
////    RS485_SendByte((uint8_t)ch);
////    return ch;
////}

///****************************End*****************************/


#include "USART1.h"

/*================ 环形缓冲区定义 ================*/
Usart1_RingBuf_t g_usart1_ring = {0};

/*================ 环形缓冲区操作 ================*/
void Usart1Ring_PutChar(uint8_t ch)
{
    uint16_t next = (g_usart1_ring.head + 1) % USART1_RX_BUF_SIZE;

    if (next == g_usart1_ring.tail) {
        g_usart1_ring.overflow = 1;     /* 满了，标记溢出，丢弃新数据 */
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

/*================ 新增：兼容 upgrade.c 的读取接口 ================*/
uint8_t Usart1_ReadByte(uint8_t* byte)
{
    if (g_usart1_ring.head == g_usart1_ring.tail) {
        return 0;   /* 无数据 */
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
    RS485_RX_MODE();    /* 默认接收模式 */

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
    while (RESET == usart_flag_get(RS485_USART, USART_FLAG_TC));
    RS485_RX_MODE();
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

/*================ 中断服务函数 ================*/
void USART1_IRQHandler(void)
{
    if (RESET != usart_interrupt_flag_get(RS485_USART, USART_INT_FLAG_RBNE)) {
        uint8_t ch = (uint8_t)usart_data_receive(RS485_USART);
        Usart1Ring_PutChar(ch);
        usart_interrupt_flag_clear(RS485_USART, USART_INT_FLAG_RBNE);
    }
}

