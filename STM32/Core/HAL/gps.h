#ifndef _GPS_H
#define _GPS_H

#include "usart.h"
#include "tim.h"
#include "butterlp.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "malloc.h"
#include "bsp_log.h"

#define GPS_BUFFER_LEN 1024 
#define GPS_OFFLINE_ADJUST_STEP 0.001   // GPS离线时, 允许通过按键微调定位以进行调试
typedef struct gps_data
{
    uint32_t utc_time;  // utc时间
    double longitude;  // 经度, E为正W为负
    double latitude;  // 纬度, N为正S为负
    char altitude_str[7];  // 海拔(m), 此数据不参与计算, 仅供显示, 直接使用字符串存储
    char hdof_str[5];  // 水平精度因子, 越小越好
}GPS_Data;


typedef struct gps_t
{
    UART_HandleTypeDef* huart;
    uint8_t rxbuf[GPS_BUFFER_LEN];
    GPS_Data data;
    ButterLP *lng_lp, *lat_lp;

    TIM_HandleTypeDef* watchdog_htim;    
    uint8_t watchdog_count;
    uint8_t online_count;
}GPS;

GPS *GPS_Init(UART_HandleTypeDef *huart, TIM_HandleTypeDef* htim);
void GPS_RxCallback(GPS *obj);
void GPS_Solve(GPS* obj);

#endif