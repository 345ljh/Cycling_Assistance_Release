#ifndef _SEARCH_H
#define _SEARCH_H

#include "malloc.h"
#include "bsp_log.h"
#include "w25q.h"
#include "tft.h"
#include "esp32.h"
#include "gps.h"

typedef struct search_app_t
{
    W25Q* w25q;  // 储存字库
    TFT* tft;
    ESP32* esp32;
    GPS* gps;
    int8_t nowselect;
}SearchApp;

SearchApp* SearchApp_Init(W25Q* w25q, TFT* tft, ESP32* esp32, GPS* gps);
void SearchApp_UpdateName(SearchApp* obj);
void SearchApp_UpdateSelection(SearchApp* obj);

void SearchApp_Up(SearchApp* obj);
void SearchApp_Down(SearchApp* obj);
void SearchApp_Send(SearchApp* obj);
#endif