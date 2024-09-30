#include "route_app.h"

RouteApp* RouteApp_Init(TIM_HandleTypeDef* htim, ADC_HandleTypeDef* hadc, TFT* tft, GPS* gps, ESP32* esp32, InputMethod* inputmethod, W25Q* w25q){
    RouteApp* obj = (RouteApp*)malloc(sizeof(RouteApp));
    memset(obj, 0, sizeof(RouteApp));

    obj->update_htim = htim;
    HAL_TIM_Base_Start_IT(obj->update_htim);
    obj->bat_hadc = hadc;
    // HAL_ADC_Start_DMA(obj->bat_hadc, (uint32_t*)&obj->bat_value, 1);
    obj->tft = tft;
    obj->gps = gps;
    obj->esp32 = esp32;
    obj->inputmethod = inputmethod;
    obj->w25q = w25q;

    obj->zoom_rate = 1;

    return obj;
}

/*
显示utc时间(xx:xx:xx) 电池电量(x.xx)
当前海拔(xxxx.x) gps精度(xx.x)
路线长度(xxxxx) 已行驶距离(xxxxx)
经度(E xxx.xxxxxx) 纬度(N xx.xxxxxx)
72行, 1s刷新

200*200
*/
void RouteApp_SetStatus(RouteApp* obj){
    TFT_Fill(obj->tft, 0, 0, 240, 54, 0);

    uint8_t sym[][32] = {
        {0, 0, 0, 0, 3, 192, 12, 48, 17, 8, 17, 8, 33, 36, 33, 68, 33, 132, 32, 4, 16, 8, 16, 8, 12, 48, 3, 192, 0, 0, 0, 0},
        {0, 0, 0, 0, 7, 224, 4, 32, 15, 240, 16, 8, 16, 8, 17, 8, 18, 8, 23, 232, 16, 72, 16, 136, 16, 8, 15, 240, 0, 0, 0, 0},
        {0, 0, 1, 192, 2, 32, 2, 32, 4, 16, 4, 48, 12, 208, 11, 16, 8, 8, 16, 8, 16, 8, 32, 4, 63, 252, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 32, 32, 16, 208, 9, 8, 7, 144, 4, 96, 4, 64, 12, 68, 19, 148, 33, 20, 22, 36, 8, 200, 0, 16, 1, 224, 0, 0},
        {0, 0, 0, 0, 2, 0, 3, 128, 2, 112, 2, 12, 3, 28, 2, 224, 2, 0, 2, 0, 2, 0, 15, 240, 0, 0, 31, 248, 0, 0, 0, 0},
        {0, 0, 0, 0, 60, 0, 34, 0, 34, 0, 34, 0, 62, 0, 34, 0, 33, 128, 33, 120, 34, 84, 32, 4, 63, 252, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 224, 1, 16, 2, 8, 2, 8, 2, 8, 1, 16, 1, 224, 2, 0, 4, 0, 8, 0, 16, 0, 32, 0, 0, 0, 0, 0}
    };

    char time[12] = "--:--:--";
    if(obj->gps->watchdog_count) {
        uint8_t hh = obj->gps->data.utc_time / 10000;
        uint8_t mm = obj->gps->data.utc_time / 100 % 100;
        uint8_t ss = obj->gps->data.utc_time % 100;
        sprintf(time, "%02d:%02d:%02d", hh, mm, ss);
    }
    TFT_DrawStr_ascii(obj->tft, 16, 0, time, RGB2565(29, 237, 254), 9);

    HAL_ADC_Start(obj->bat_hadc);
    obj->bat_value = HAL_ADC_GetValue(obj->bat_hadc);
    char bat[5] = "0.00";
    gcvt(obj->bat_value * 66 / 40960.0, 3, bat);
    if(strlen(bat) == 1) strcat(bat, ".00");  // gcvt会试图截掉末尾的0, 此语句用于补0
    if(strlen(bat) == 3) strcat(bat, "0");
    TFT_DrawStr_ascii(obj->tft, 16, 18, bat, RGB2565(255, 242, 0), 5);

    char zoom[5] = "1.00";
    gcvt(obj->zoom_rate_temp, 2, zoom);
    if(strlen(zoom) == 1) strcat(zoom, ".00");
    if(strlen(zoom) == 2) strcat(zoom, ".0");
    if(strlen(zoom) == 3) strcat(zoom, "0");
    TFT_DrawStr_ascii(obj->tft, 16, 108, zoom, (obj->zoom_rate_temp == obj->zoom_rate ? RGB2565(195, 195, 210) : RGB2565(195, 0, 0)), 5);

    if(obj->gps->watchdog_count) {
        TFT_DrawStr_ascii(obj->tft, 16, 36, obj->gps->data.altitude_str, RGB2565(252, 170, 18), 9);
        TFT_DrawStr_ascii(obj->tft, 16, 54, obj->gps->data.hdof_str, RGB2565(102, 236, 105), 9);

        char lat[10];
        gcvt(fabs(obj->gps->data.latitude), 8, lat);
        TFT_DrawStr_ascii(obj->tft, 16, 126, lat, RGB2565(128, 255, 255), 10);
        if(obj->gps->data.latitude > 0) TFT_DrawStr_ascii(obj->tft, 4, 126, "N", RGB2565(128, 255, 255), 1);
        else TFT_DrawStr_ascii(obj->tft, 4, 126, "S", RGB2565(128, 255, 255), 1);

        char lon[11];
        gcvt(fabs(obj->gps->data.longitude), 9, lon);
        TFT_DrawStr_ascii(obj->tft, 16, 144, lon, RGB2565(128, 255, 255), 11);

        if(obj->gps->data.latitude > 0) TFT_DrawStr_ascii(obj->tft, 4, 144, "E", RGB2565(128, 255, 255), 1);
        else TFT_DrawStr_ascii(obj->tft, 4, 144, "W", RGB2565(128, 255, 255), 1);
    } else{
        TFT_DrawStr_ascii(obj->tft, 16, 36, "----.-", RGB2565(252, 170, 18), 9);
        TFT_DrawStr_ascii(obj->tft, 16, 54, "--.-", RGB2565(102, 236, 105), 9);

        TFT_DrawStr_ascii(obj->tft, 16, 126, "--.------", RGB2565(128, 255, 255), 10);
        TFT_DrawStr_ascii(obj->tft, 4, 126, "N", RGB2565(128, 255, 255), 1);

        TFT_DrawStr_ascii(obj->tft, 16, 144, "---.------", RGB2565(128, 255, 255), 11);
        TFT_DrawStr_ascii(obj->tft, 4, 144, "E", RGB2565(128, 255, 255), 1);
    }

    if(obj->esp32->rx_data_temp.routepoint.len) {
        TFT_DrawStr_ascii(obj->tft, 16, 72, TFT_Itoa(obj->esp32->rx_data_temp.routepoint.distance), RGB2565(128, 255, 255), 9);
        TFT_DrawStr_ascii(obj->tft, 16, 90, TFT_Itoa((uint16_t)obj->mileage), RGB2565(255, 128, 128), 9);
    } else {
        TFT_DrawStr_ascii(obj->tft, 16, 72, "-----", RGB2565(128, 255, 255), 9);
        TFT_DrawStr_ascii(obj->tft, 16, 90, "-----", RGB2565(255, 128, 128), 9);
    }

    TFT_DrawStr_fonttype(obj->tft, 0, 0, sym[0], RGB2565(29, 237, 254));
    TFT_DrawStr_fonttype(obj->tft, 0, 18, sym[1], RGB2565(255, 242, 0));
    TFT_DrawStr_fonttype(obj->tft, 0, 36, sym[2], RGB2565(252, 170, 18));
    TFT_DrawStr_fonttype(obj->tft, 0, 54, sym[3], RGB2565(102, 236, 105));
    TFT_DrawStr_fonttype(obj->tft, 0, 72, sym[4], RGB2565(128, 255, 255));
    TFT_DrawStr_fonttype(obj->tft, 0, 90, sym[5], RGB2565(255, 128, 128));
    TFT_DrawStr_fonttype(obj->tft, 0, 108, sym[6], RGB2565(195, 195, 210));
}

void RouteApp_UpdateStatus(RouteApp* obj){
    char time[12] = "--:--:--";
    if(obj->gps->watchdog_count) {
        uint8_t hh = obj->gps->data.utc_time / 10000;
        uint8_t mm = obj->gps->data.utc_time / 100 % 100;
        uint8_t ss = obj->gps->data.utc_time % 100;
        sprintf(time, "%02d:%02d:%02d", hh, mm, ss);
    }
    TFT_DrawStr_ascii(obj->tft, 16, 0, time, RGB2565(29, 237, 254), 9);

    HAL_ADC_Start(obj->bat_hadc);
    obj->bat_value = HAL_ADC_GetValue(obj->bat_hadc);
    char bat[5] = "0.00";
    gcvt(obj->bat_value * 66 / 40960.0, 3, bat);
    if(strlen(bat) == 1) strcat(bat, ".00");
    if(strlen(bat) == 3) strcat(bat, "0");
    TFT_DrawStr_ascii(obj->tft, 16, 18, bat, RGB2565(255, 242, 0), 5);

    char zoom[5] = "1.00";
    gcvt(obj->zoom_rate_temp, 3, zoom);
    if(strlen(zoom) == 1) strcat(zoom, ".00");
    if(strlen(zoom) == 2) strcat(zoom, ".0");
    if(strlen(zoom) == 3) strcat(zoom, "0");
    TFT_DrawStr_ascii(obj->tft, 16, 108, zoom, (obj->zoom_rate_temp == obj->zoom_rate ? RGB2565(195, 195, 210) : RGB2565(195, 0, 0)), 5);

    if(obj->gps->watchdog_count) {
        TFT_DrawStr_ascii(obj->tft, 16, 36, obj->gps->data.altitude_str, RGB2565(252, 170, 18), 9);
        TFT_DrawStr_ascii(obj->tft, 16, 54, obj->gps->data.hdof_str, RGB2565(102, 236, 105), 9);

        char lat[10];
        gcvt(fabs(obj->gps->data.latitude), 8, lat);
        TFT_DrawStr_ascii(obj->tft, 16, 126, lat, RGB2565(128, 255, 255), 10);
        if(obj->gps->data.latitude > 0) TFT_DrawStr_ascii(obj->tft, 4, 126, "N", RGB2565(128, 255, 255), 1);
        else TFT_DrawStr_ascii(obj->tft, 4, 126, "S", RGB2565(128, 255, 255), 1);

        char lon[11];
        gcvt(fabs(obj->gps->data.longitude), 9, lon);
        TFT_DrawStr_ascii(obj->tft, 16, 144, lon, RGB2565(128, 255, 255), 11);

        if(obj->gps->data.latitude > 0) TFT_DrawStr_ascii(obj->tft, 4, 144, "E", RGB2565(128, 255, 255), 1);
        else TFT_DrawStr_ascii(obj->tft, 4, 144, "W", RGB2565(128, 255, 255), 1);

        // 计算里程
        double rad_y = fabs(obj->gps->data.latitude - obj->gps->lat_lp->y[1]) * PI / 180;
        double rad_x = fabs(obj->gps->data.longitude - obj->gps->lng_lp->y[1]) * PI / 180;
        double r_polar = 6.3569e6, r_equator = 6.3771e6;  // 极半径与赤道半径
        double r_lat = r_equator * cos(obj->gps->data.latitude * PI / 180);
        if(obj->gps->online_count >= 5) obj->mileage += sqrt(pow(rad_y * r_polar, 2) + pow(rad_x * r_lat, 2));
    } else{
        TFT_DrawStr_ascii(obj->tft, 16, 36, "----.-", RGB2565(252, 170, 18), 9);
        TFT_DrawStr_ascii(obj->tft, 16, 54, "--.-", RGB2565(102, 236, 105), 9);

        TFT_DrawStr_ascii(obj->tft, 16, 126, "--.------", RGB2565(128, 255, 255), 10);
        TFT_DrawStr_ascii(obj->tft, 4, 126, "N", RGB2565(128, 255, 255), 1);

        TFT_DrawStr_ascii(obj->tft, 16, 144, "---.------", RGB2565(128, 255, 255), 11);
        TFT_DrawStr_ascii(obj->tft, 4, 144, "E", RGB2565(128, 255, 255), 1);
    }

    if(obj->esp32->rx_data_temp.routepoint.len) {
        TFT_DrawStr_ascii(obj->tft, 16, 72, TFT_Itoa(obj->esp32->rx_data_temp.routepoint.distance), RGB2565(128, 255, 255), 9);
        TFT_DrawStr_ascii(obj->tft, 16, 90, TFT_Itoa((uint16_t)obj->mileage), RGB2565(255, 128, 128), 9);
    } else {
        TFT_DrawStr_ascii(obj->tft, 16, 72, "-----", RGB2565(128, 255, 255), 9);
        TFT_DrawStr_ascii(obj->tft, 16, 90, "-----", RGB2565(255, 128, 128), 9);
    }
}


void RouteApp_SetRoute(RouteApp* obj){
    uint8_t len = obj->esp32->rx_data_temp.routepoint.len;
    Point* route = (Point*)calloc(sizeof(Point), len);
    double hside = obj->esp32->rx_data_temp.routepoint.halfside / obj->zoom_rate;
    memcpy(route, obj->esp32->rx_data_temp.routepoint.routes, sizeof(Point) * len);

    // 将复制的route范围从(cent_x - hside, cent_y - hside)~(cent_x + hside, cent_y + hside) 映射到(0, 200)~(200, 0)
    for(int i = 0; i < len; i++){
        route[i].longitude = (route[i].longitude - (obj->show_center.longitude - hside)) * FRAME_BUF_SIDE / hside / 2;
        route[i].latitude = FRAME_BUF_SIDE - (route[i].latitude - (obj->show_center.latitude - hside)) * FRAME_BUF_SIDE / hside / 2;
    }
    // for(int i = 0; i < len; i++){
    //     printf_log("%d,%d,%d\n", i, (int)route[i].longitude, (int)route[i].latitude);
    // }
    // 在tft缓冲区绘制图片, 即直线路径
    memset(obj->tft->frame_buf, 0, FRAME_BUF_SIDE * FRAME_BUF_SIDE / 4);
    for(int i = 0; i < len - 1; i++){
        int32_t x1 = (int)route[i].longitude, x2 = (int)route[i + 1].longitude, y1 = (int)route[i].latitude, y2 = (int)route[i + 1].latitude;

        if(x1 > x2){
            int32_t temp = x2;
            x2 = x1;
            x1 = temp;
            temp = y2;
            y2 = y1;
            y1 = temp;
        }
        //竖直线
        if(x1 == x2) {
            for(int32_t y = y1; y <= y2 - 1; y++) {
                    if(InRange(x1, 0, FRAME_BUF_SIDE) && InRange(y, 0, FRAME_BUF_SIDE)){
                        obj->tft->frame_buf[y][x1 / 4] &= ~(3 << (x1 % 4 * 2));
                        obj->tft->frame_buf[y][x1 / 4] |= (1 << (x1 % 4 * 2));
                    }
                }
        } else {
            float k = (y2 - y1) * 1.0 / (x2 - x1);
            float b = y1 - k * x1;
            if(fabs(k) > 1){
                if(y1 > y2){
                    int32_t temp = x2;
                    x2 = x1;
                    x1 = temp;
                    temp = y2;
                    y2 = y1;
                    y1 = temp;
                }
                for(int32_t y = y1; y <= y2 - 1; y++){
                    int32_t x = (int32_t)round((y - b) / k);
                    if(InRange(x, 0, FRAME_BUF_SIDE) && InRange(y, 0, FRAME_BUF_SIDE)){
                        obj->tft->frame_buf[y][x / 4] &= ~(3 << (x % 4 * 2));
                        obj->tft->frame_buf[y][x / 4] |= (1 << (x % 4 * 2));
                    }
                }
            } else {
                for(int32_t x = x1; x <= x2 - 1; x++){
                    int32_t y = (int32_t)round(k * x + b);
                    if(InRange(x, 0, FRAME_BUF_SIDE) && InRange(y, 0, FRAME_BUF_SIDE)){
                        obj->tft->frame_buf[y][x / 4] &= ~(3 << (x % 4 * 2));
                        obj->tft->frame_buf[y][x / 4] |= (1 << (x % 4 * 2));
                    }
                }
            }

        }
    }
    TFT_DrawFrame(obj->tft, 100, 20);

    free(route);
}

void RouteApp_UpdateRoute(RouteApp* obj){
    uint8_t len = obj->esp32->rx_data_temp.routepoint.len;
    Point* route = (Point*)calloc(sizeof(Point), len);
    double hside = obj->esp32->rx_data_temp.routepoint.halfside / obj->zoom_rate;
    memcpy(route, obj->esp32->rx_data_temp.routepoint.routes, sizeof(Point) * len);

    // 将当前位置从(cent_x - hside, cent_y - hside)~(cent_x + hside, cent_y + hside) 映射到(0, 200)~(200, 0)
    double x = (obj->gps->data.longitude - (obj->show_center.longitude - hside)) * FRAME_BUF_SIDE / hside / 2;
    double y = FRAME_BUF_SIDE - (obj->gps->data.latitude - (obj->show_center.latitude - hside)) * FRAME_BUF_SIDE / hside / 2;

    // char str1[15], str2[15];
    // Float2Str(str1, x);
    // Float2Str(str2, y);
    // printf_log("%s %s\n", str1, str2);
    // printf_log("%d,%d\n", (int)x, (int)y);

    // 加载路线图
    TFT_DrawFramePart(obj->tft, 100, 20);

    // 画圆
    if(InRange(x, 0, FRAME_BUF_SIDE) && InRange(y, 0, FRAME_BUF_SIDE)){
        TFT_DrawArc(obj->tft, (uint16_t)round(x + 100), (uint16_t)round(y + 20), 3, 0, 360, 0xFA6A);
        TFT_DrawArc(obj->tft, (uint16_t)round(x + 100), (uint16_t)round(y + 20), 3, 0, 360, 0xFA6A);
    }

    // 记录当前位置
    obj->tft->frame_x1 = round(x) - 3;
    obj->tft->frame_x2 = round(x) + 3;
    obj->tft->frame_y1 = round(y) - 3;
    obj->tft->frame_y2 = round(y) + 3;

    free(route);
}

void RouteApp_SendMessage(RouteApp* obj){
    Point ori = {obj->gps->data.longitude, obj->gps->data.latitude};
    ESP32_SendOriginPosition(obj->esp32, ori);
    uint16_t dst_name[30] = {0};
    for(int i = 0; i < obj->inputmethod->buffer[0].len; i++){
        W25Q_ReadFlash(obj->w25q, (uint8_t*)dst_name + 2 * i, 0xF0000 + (character_offset[obj->inputmethod->buffer[0].chs[i].initial * FINAL_AMOUNT + obj->inputmethod->buffer[0].chs[i].final] + obj->inputmethod->buffer[0].chs[i].index) * 2, 2);
    }
    for(int i = 0; i < 30; i++) printf_log("%04x, ", dst_name[i]);

    uint16_t city[10] = {0};
    for(int i = 0; i < obj->inputmethod->buffer[1].len; i++){
        W25Q_ReadFlash(obj->w25q, (uint8_t*)city + 2 * i, 0xF0000 + (character_offset[obj->inputmethod->buffer[1].chs[i].initial * FINAL_AMOUNT + obj->inputmethod->buffer[1].chs[i].final] + obj->inputmethod->buffer[1].chs[i].index) * 2, 2);
    }
    for(int i = 0; i < 10; i++) printf_log("%04x, ", city[i]);

    ESP32_SendDestinationName(obj->esp32, dst_name, city);
}

void RouteApp_ShowError(RouteApp* obj){
    switch (obj->esp32->rx_data_temp.flag){
    case 1:
        TFT_DrawStr_ascii(obj->tft, 0, 0, "Fail to find destination!", RGB2565(255, 0, 0), 26);
        break;
    case 2:
        TFT_DrawStr_ascii(obj->tft, 0, 0, "Fail to plan the route!", RGB2565(255, 0, 0), 24);
        break;
    }
}

uint8_t RouteApp_IsOutRange(RouteApp* obj, Point point){
    double hside = obj->esp32->rx_data_temp.routepoint.halfside / obj->zoom_rate;
    if(InRange(obj->gps->data.longitude, obj->show_center.longitude - hside, obj->show_center.longitude + hside)
    && InRange(obj->gps->data.latitude, obj->show_center.latitude - hside, obj->show_center.latitude + hside))
        return 0;
    else return 1;
}