#include "fun.h"

uint8_t lcd_page = 1;                            // lcd界面数
float adc_volA, adc_volB;                        // 两个模拟电压值定义为浮点数
uint8_t DS_DR_FS_FR_line;                        // 控制参数界面的行加减
int32_t PARA_D_SR_F_SR[4] = {10, 80, 100, 2000}; // 数组定义四个参数，简洁高效
uint32_t IC_fre;                                 // 输入频率检测
uint32_t PWM_CF, PWM_CD;                         // PA
uint8_t tim_S, tim_M, tim_H;                     // 钟表显示运行时间，替代Rtc
uint8_t ST_flag, recd_flag, recd_flag_last;      // ST锁定1为锁定 ,recd状态显示：1为异常
uint32_t tim_current;                            // 按键长短按逻辑变量 获取uwTick当前的计数值
// uint16_t CF_last, CD_last, XF_last, tim_S_last, tim_M_last, tim_H_last;此处已被数组形式代替
uint16_t last_CF_CD_DF_XF_H_M_S[7]; // 保存上次参数值
void led_show(uint8_t led, uint8_t mode)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET); // 开关锁存器
    if (mode)
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
}

uint8_t B1_statu = 1, B1_last = 1, B2_statu = 1, B2_last = 1, B3_statu = 1, B3_last = 1, B4_statu = 1, B4_last = 1;
uint16_t time_crrent; // 记录当前时间
void key_scan(void)
{ // 系统计数器uwTick 每1毫秒加1
    if (uwTick - time_crrent < 10)
        return;           // 非物理消抖，减少按键执行效率，当小于10毫秒时强制函数返回
    time_crrent = uwTick; // 得到当前计数值
    B1_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    B2_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
    B3_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);
    B4_statu = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    // B1按键
    if (!B1_statu & B1_last)
    {

        LCD_Clear(Black);
        if (++lcd_page == 4)
            lcd_page = 1;
        if (lcd_page == 2)
            DS_DR_FS_FR_line = 0; // 参数行
        if (lcd_page == 2)
        { // 这两部分在进入recd界面时先得到DF，XF值让显示更准确
            last_CF_CD_DF_XF_H_M_S[2] = IC_fre;
            last_CF_CD_DF_XF_H_M_S[3] = IC_fre - PWM_CF;
        }
    }

    // B2按键

    if (!B2_statu & B2_last)
    {
        if (lcd_page == 3)
        {
            if (DS_DR_FS_FR_line++ == 4)
                DS_DR_FS_FR_line = 0;
        }
    }
    // B2监控界面
    if (lcd_page == 1)
    { // 按键长短按，长按置零钟表
        if (!B2_statu & B2_last)
        {
            tim_current = uwTick;
        }
        if (B2_statu & !B2_last)
        {
            if (uwTick - tim_current >= 2000)
            {
                tim_H = tim_M = tim_S = 0;
            }
            else
                ST_flag ^= 1;
        }
    }
    // B3：加按键
    if (!B3_statu & B3_last)
    {
        if (lcd_page == 3)
        { // math函数pow （x，y） x的y次方
            PARA_D_SR_F_SR[DS_DR_FS_FR_line] += pow(10, DS_DR_FS_FR_line);
        }
    }
    if (!B4_statu & B4_last)
    {
        if (lcd_page == 3)
        {
            PARA_D_SR_F_SR[DS_DR_FS_FR_line] -= pow(10, DS_DR_FS_FR_line);
        }
    }
    B1_last = B1_statu;
    B2_last = B2_statu;
    B3_last = B3_statu;
    B4_last = B4_statu;
}
char string[20];
uint32_t lcd_time; // lcd每0.1秒刷新变量
void lcd_show(void)
{ // 实现每0.1秒刷新变量
    if (uwTick - lcd_time < 99)
        return; // 小于0.1秒就return
    else
        lcd_time = uwTick; // lcd的频率刷新定为0.1秒
    if (lcd_page == 1)
    { // PWM界面

        sprintf(string, "       PWM         ");
        LCD_DisplayStringLine(Line1, (uint8_t *)string);

        sprintf(string, "   CF=%dHz           ", PWM_CF);
        LCD_DisplayStringLine(Line3, (uint8_t *)string);

        sprintf(string, "   CD=%d%%         ", PWM_CD);
        LCD_DisplayStringLine(Line4, (uint8_t *)string);

        sprintf(string, "   DF=%dHz         ", IC_fre);
        LCD_DisplayStringLine(Line5, (uint8_t *)string);
        if (ST_flag)
            sprintf(string, "   ST=LOCK           ");
        else
            sprintf(string, "   ST=UNLOCK         ");
        LCD_DisplayStringLine(Line6, (uint8_t *)string);

        sprintf(string, "   %.2dH%.2dM%.2dS ", tim_H, tim_M, tim_S);
        LCD_DisplayStringLine(Line7, (uint8_t *)string);
    }
    if (lcd_page == 2)
    {
        sprintf(string, "       RECD        ");
        LCD_DisplayStringLine(Line1, (uint8_t *)string);
        // RECD正常界面
        if (!recd_flag)
        {
            sprintf(string, "   CF=%dHz           ", PWM_CF);
            LCD_DisplayStringLine(Line3, (uint8_t *)string);

            sprintf(string, "   CD=%d%%         ", PWM_CD);
            LCD_DisplayStringLine(Line4, (uint8_t *)string);

            sprintf(string, "   DF=%dHz         ", IC_fre);
            LCD_DisplayStringLine(Line5, (uint8_t *)string);

            sprintf(string, "   XF=%dHz         ", IC_fre - PWM_CF);
            LCD_DisplayStringLine(Line6, (uint8_t *)string);

            sprintf(string, "   %.2dH%.2dM%.2dS ", tim_H, tim_M, tim_S);
            LCD_DisplayStringLine(Line7, (uint8_t *)string);
        }
        // RECD异常界面利用last数组显示上一次的值
        if (recd_flag)
        {
            sprintf(string, "   CF=%dHz           ", last_CF_CD_DF_XF_H_M_S[0]);
            LCD_DisplayStringLine(Line3, (uint8_t *)string);

            sprintf(string, "   CD=%d%%         ", last_CF_CD_DF_XF_H_M_S[1]);
            LCD_DisplayStringLine(Line4, (uint8_t *)string);

            sprintf(string, "   DF=%dHz         ", last_CF_CD_DF_XF_H_M_S[2]);
            LCD_DisplayStringLine(Line5, (uint8_t *)string);

            sprintf(string, "   XF=%dHz         ", last_CF_CD_DF_XF_H_M_S[3]);
            LCD_DisplayStringLine(Line6, (uint8_t *)string);

            sprintf(string, "   %.2dH%.2dM%.2dS ", last_CF_CD_DF_XF_H_M_S[4], last_CF_CD_DF_XF_H_M_S[5], last_CF_CD_DF_XF_H_M_S[6]);
            LCD_DisplayStringLine(Line7, (uint8_t *)string);
        }
    }
    if (lcd_page == 3)
    { // PARA参数界面
        sprintf(string, "       PARA        ");
        LCD_DisplayStringLine(Line1, (uint8_t *)string);

        sprintf(string, "   DS=%d%%          ", PARA_D_SR_F_SR[0]);
        LCD_DisplayStringLine(Line3, (uint8_t *)string);

        sprintf(string, "   DR=%d%%          ", PARA_D_SR_F_SR[1]);
        LCD_DisplayStringLine(Line4, (uint8_t *)string);

        sprintf(string, "   FS=%dHz           ", PARA_D_SR_F_SR[2]);
        LCD_DisplayStringLine(Line5, (uint8_t *)string);

        sprintf(string, "   FR=%dHz           ", PARA_D_SR_F_SR[3]);
        LCD_DisplayStringLine(Line6, (uint8_t *)string);
    }
}
// adc电压测量最好写成double形式
void adc_vlo(void)
{
    // 注意测量模拟电压前先开启adc，这是adc模拟电压转换数字的函数，如果不开启，那就是模电电压不是数字电压
    HAL_ADC_Start(&hadc1);
    HAL_ADC_Start(&hadc2);

    adc_volB = HAL_ADC_GetValue(&hadc1) * 3.3 / 4095; // 对数字电压线性转化为0到3.3伏
    adc_volA = HAL_ADC_GetValue(&hadc2) * 3.3 / 4095;
}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        IC_fre = 1000000 / (TIM2->CCR1 + 1); // 检测TIM2的输出频率，涉及到通道不要只写TIM2->CCR
        TIM2->CNT = 0;                       // CCR从零开始计数要加上1是准确计数值
    }
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        tim_S++; // 利用定时器实现RTC功能
    }
}

void time_show(void)
{ // 钟表部分

    if (tim_S > 59)
    {
        tim_S = 0;
        tim_M++;
    }
    if (tim_M > 59)
    {
        tim_H++;
        tim_M = 0;
    }

    if (tim_H > 23)
    {
        tim_H = 0;
    }
}
void data_proc(void)
{ // adc测量获得电压
    adc_vlo();

    // 阶梯计算
    // 占空比部分
    if (!ST_flag) // 这里的锁定判定在数据处理中进行最好不要在lcd中进行，这样可以减少代码量也让逻辑更清晰
    {             // 阶数n=(DR-10)/DS更要注意逻辑中步长是n值还是n-1值
        uint16_t n = (PARA_D_SR_F_SR[1] - 10) / PARA_D_SR_F_SR[0];
        float VA = 3.3 / (n + 1);
        // if (adc_volA > = 0 && adc_volA < 1 * VA)
        // {
        //     TIM17->CCR1 = 10;
        // }
        // if (adc_volA > = 1 * VA && adc_volA < 2 * VA)
        // {
        //     TIM17->CCR1 = 10 + PARA_D_SR_F_SR[0] * 1;
        // }if (adc_volA > = 2 * VA && adc_volA < 3 * VA)
        // {
        //     TIM17->CCR1 = 10 + PARA_D_SR_F_SR[0] * 2;
        // }
        for (int i = 0; i <= n; i++)
        {
            if (adc_volA >= i * VA && adc_volA < (i + 1) * VA)
            {
                TIM17->CCR1 = 10 + PARA_D_SR_F_SR[0] * i;
                break;
            }
        }

        // 频率部分
        uint16_t N = (PARA_D_SR_F_SR[3] - 1000) / PARA_D_SR_F_SR[2];
        float VB = 3.3 / (N + 1);
        for (int i = 0; i <= N; i++)
        {
            if (adc_volB >= i * VB && adc_volB < (i + 1) * VB)
            {
                TIM17->PSC = 800000 / (1000 + PARA_D_SR_F_SR[2] * i) - 1;
                break;
            }
        }
    }
    // 输出处理部分
    PWM_CF = 800000 / (TIM17->PSC + 1);
    PWM_CD = 1.0 * TIM17->CCR1 / TIM17->ARR * 100;
    // 异常判定部分
    // 利用双标志位判断，注意：一个标志位时是否有用，函数是不是在标志位条件下一直执行（违背了只想执行一次的逻辑）
    if (IC_fre - PWM_CF > 1000)
        recd_flag = 1;
    else
        recd_flag = 0;
    if (recd_flag & !recd_flag_last)
    {

        last_CF_CD_DF_XF_H_M_S[0] = PWM_CF;
        last_CF_CD_DF_XF_H_M_S[1] = PWM_CD;
        last_CF_CD_DF_XF_H_M_S[2] = IC_fre;
        last_CF_CD_DF_XF_H_M_S[3] = IC_fre - PWM_CF;
        last_CF_CD_DF_XF_H_M_S[4] = tim_H;
        last_CF_CD_DF_XF_H_M_S[5] = tim_M;
        last_CF_CD_DF_XF_H_M_S[6] = tim_S;
    }
    recd_flag_last = recd_flag; // 在函数最后赋值标志位上一次的状态
}
void main_proc(void)
{
    time_show(); // 钟表显示
    key_scan();

    data_proc(); // 数据处理

    lcd_show();
    led_show(1, lcd_page == 1);
    led_show(2, ST_flag == 1);
    led_show(3, recd_flag == 1);
}
