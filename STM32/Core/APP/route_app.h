#ifndef _ROUTE_APP_H
#define _ROUTE_APP_H


#include "tft.h"
#include "gps.h"
#include "esp32.h"
#include "inputmethod.h"
#include "w25q.h"
#include "tim.h"
#include "malloc.h"
#include "string.h"
#include "stdlib.h"

typedef struct route_app_t
{
    ADC_HandleTypeDef* bat_hadc;
    TFT* tft;
    GPS* gps;
    ESP32* esp32;
    InputMethod* inputmethod;
    W25Q* w25q;

    uint16_t bat_value;

    TIM_HandleTypeDef* update_htim; // 50ms
    uint8_t update_count;
    float mileage;  // 累积里程

    Point show_center;
    float zoom_rate;
    float zoom_rate_temp;
}RouteApp;

RouteApp* RouteApp_Init(TIM_HandleTypeDef* htim, ADC_HandleTypeDef* hadc, TFT* tft, GPS* gps, ESP32* esp32, InputMethod* inputmethod, W25Q* w25q);
void RouteApp_SetStatus(RouteApp* obj);
void RouteApp_UpdateStatus(RouteApp* obj);
void RouteApp_SetRoute(RouteApp* obj);
void RouteApp_UpdateRoute(RouteApp* obj);

void RouteApp_SendMessage(RouteApp* obj);

void RouteApp_ShowError(RouteApp* obj);
uint8_t RouteApp_IsOutRange(RouteApp* obj, Point point);
#endif