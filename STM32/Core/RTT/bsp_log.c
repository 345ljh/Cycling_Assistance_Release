#include "bsp_log.h"

void BSP_Log_Init() { SEGGER_RTT_Init(); }

// us级延时
void delay_us(int nus)
{
    uint16_t differ = 0xffff - nus - 5;

    __HAL_TIM_SetCounter(&htim5, differ);
    //开启定时器
    HAL_TIM_Base_Start(&htim5);
 
    while( differ<0xffff-5)
    {
    differ = __HAL_TIM_GetCounter(&htim5);
    };
    //关闭定时器
    HAL_TIM_Base_Stop(&htim5);
}

int printf_log(const char* fmt, ...) {
    // char buffer[128];
    va_list args;
    va_start(args, fmt);
    // int n = vsnprintf(buffer,sizeof(buffer),fmt,args);
    // SEGGER_RTT_Write(BUFFER_INDEX,buffer,n);
    int n = SEGGER_RTT_vprintf(0, fmt, &args);
    va_end(args);
    return n;
}

void Float2Str(char* str, float va) {
    int flag = va < 0;
    int head = (int)va;
    int point = (int)((va - head) * 1000);
    head = abs(head);
    point = abs(point);
    if (flag)
        sprintf(str, "-%d.%d", head, point);
    else
        sprintf(str, "%d.%d", head, point);
}

uint8_t InRange(double num, double min, double max){
    if(num <= min || num >= max) return 0;
    else return 1;
}