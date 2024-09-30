#ifndef _BSP_LOG_H
#define _BSP_LOG_H

#include "SEGGER_RTT.h"
#include "SEGGER_RTT_Conf.h"
#include "stdio.h"
#include "tim.h"

void BSP_Log_Init();
int printf_log(const char* fmt, ...);
void Float2Str(char* str, float va);
void delay_us(int nus);
unsigned char InRange(double num, double min, double max);
#endif