#include "search_app.h"

SearchApp* SearchApp_Init(W25Q* w25q, TFT* tft, ESP32* esp32, GPS* gps){
    SearchApp* obj = (SearchApp*)malloc(sizeof(SearchApp));
    memset(obj, 0, sizeof(SearchApp));
    obj->w25q = w25q;
    obj->tft = tft;
    obj->esp32 = esp32;
    obj->gps = gps;
    return obj;
}

// 展示选项
void SearchApp_UpdateName(SearchApp* obj){
    TFT_Fill(obj->tft, 0, 0, 320, 240, 0);
    for(int s = 0; s < obj->esp32->rx_data_temp.searchname.selection_cnt; s++){
        for(int i = 0; i < obj->esp32->rx_data_temp.searchname.selection[s].len; i++){
            if(obj->esp32->rx_data_temp.searchname.selection[s].chs[i].initial != 0xFF){
                uint8_t buf[32];
                W25Q_ReadFlash(obj->w25q, buf, 32 * (character_offset[obj->esp32->rx_data_temp.searchname.selection[s].chs[i].initial * FINAL_AMOUNT + obj->esp32->rx_data_temp.searchname.selection[s].chs[i].final] + obj->esp32->rx_data_temp.searchname.selection[s].chs[i].index), 32);
                TFT_DrawStr_fonttype(obj->tft, i % 20 * 16, i / 20 * 18 + s * 36, buf, RGB2565(255, 242, 0));
            } else {
                char ch = obj->esp32->rx_data_temp.searchname.selection[s].chs[i].index + 32;
                TFT_DrawStr_ascii(obj->tft, i % 20 * 16 + 4, i / 20 * 18 + s * 36, &ch, RGB2565(255, 242, 0), 1);
            }
        }
    }
}

// 显示光标
void SearchApp_UpdateSelection(SearchApp* obj){
    for(int i = 0; i < 6; i++){
        TFT_DrawLine(obj->tft, 0, 17 + i * 36, 64, 17 + i * 36, 0);
        TFT_DrawLine(obj->tft, 0, 16 + i * 36, 64, 16 + i * 36, 0);
    }
    TFT_DrawLine(obj->tft, 0, 16 + obj->nowselect * 36, 64, 16 + obj->nowselect * 36, RGB2565(255, 128, 128));
    TFT_DrawLine(obj->tft, 0, 17 + obj->nowselect * 36, 64, 17 + obj->nowselect * 36, RGB2565(255, 128, 128));
}

// 向上
void SearchApp_Up(SearchApp* obj){
    obj->nowselect--;
    if(obj->nowselect < 0) obj->nowselect += obj->esp32->rx_data_temp.searchname.selection_cnt;
    SearchApp_UpdateSelection(obj);
}

// 向下
void SearchApp_Down(SearchApp* obj){
    obj->nowselect++;
    obj->nowselect %= obj->esp32->rx_data_temp.searchname.selection_cnt;
    SearchApp_UpdateSelection(obj);
}

// 发送选择的目的地
void SearchApp_Send(SearchApp* obj){
    Point ori = {obj->gps->data.longitude, obj->gps->data.latitude};
    ESP32_SendOriginPosition(obj->esp32, ori);
    ESP32_SendSearchSelection(obj->esp32, obj->nowselect);
}