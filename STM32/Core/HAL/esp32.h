#ifndef _ESP32_H
#define _ESP32_H

#include "usart.h"
#include "malloc.h"
#include "string.h"
#include "bsp_log.h"

#define ESP_BUFFER_LEN 128
#define DESTNAME_MAX_LEN 30

#define SEND_ORIGIN_POSITION_DATALEN 16
#define SEND_DESTINATION_NAME_DATALEN 80

#pragma pack(1)
typedef struct point_t
{
    double longitude;  // 经度, E为正W为负
    double latitude;  // 纬度, N为正S为负
}Point;

// 接收信息
typedef struct rx_test_t
{
    double a;
    double b;
}RxTest;

typedef struct rx_routepoint_t
{
    uint32_t distance;
    uint16_t duration;
    uint8_t len;
    Point center;
    double halfside;
    Point* routes;
}RxRoutePoint;

#pragma pack()


typedef struct esp32_t
{
    UART_HandleTypeDef* huart;
    uint8_t rxbuf[ESP_BUFFER_LEN];
    struct{
        RxTest rxtest;
        RxRoutePoint routepoint;
        uint8_t flag;
    }rx_data_temp;
    
}ESP32;

ESP32 *ESP32_Init(UART_HandleTypeDef *huart);
void ESP32_SendOriginPosition(ESP32* obj, Point origin);
void ESP32_SendDestinationName(ESP32* obj, uint16_t* name, uint16_t* city);

void ESP32_RxCallback(ESP32 *obj);
void ESP32_Solve(ESP32* obj);
#endif