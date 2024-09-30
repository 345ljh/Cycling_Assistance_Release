#include "gps.h"

// HITSZ 113.966936,22.58839

GPS *GPS_Init(UART_HandleTypeDef *huart, TIM_HandleTypeDef * htim){
    GPS *obj = (GPS *)malloc(sizeof(GPS));
    memset(obj, 0, sizeof(GPS));
    // 配置滤波器
    obj->lng_lp = ButterLP_Init(&obj->data.longitude, 1, 0.2);
    obj->lat_lp = ButterLP_Init(&obj->data.latitude, 1, 0.2);

    obj->huart = huart;
    HAL_DMA_DeInit(huart->hdmatx);
    HAL_DMA_DeInit(huart->hdmarx);
    HAL_DMA_Init(huart->hdmatx);
    HAL_DMA_Init(huart->hdmarx);
    HAL_UART_DMAStop(huart);
    // 使能串口空闲中断
    __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
    HAL_UART_Receive_DMA(huart, obj->rxbuf, GPS_BUFFER_LEN);

    obj->watchdog_htim = htim;
    HAL_TIM_Base_Start_IT(obj->watchdog_htim);
    return obj;
}

void GPS_RxCallback(GPS *obj){
    __HAL_UART_CLEAR_IDLEFLAG(obj->huart);  // 清除中断标志位
      HAL_UART_DMAStop(obj->huart);
      GPS_Solve(obj);
      memset(obj->rxbuf, 0, GPS_BUFFER_LEN);
      HAL_UART_Receive_DMA(obj->huart, obj->rxbuf, GPS_BUFFER_LEN);
}

// 解析数据
// 接收到的数据如以下形式:(离线)
// $GNGGA,101755.000,,,,,0,00,25.5,,,,,,*7D
// $GNGLL,,,,,101755.000,V,N*63
// $GPGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5*02
// $BDGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5*13
// $GPGSV,1,1,02,10,,,32,194,,,22*47
// $BDGSV,1,1,01,10,,,22*68
// $GNRMC,101755.000,V,,,,,,,210824,,,N*59
// $GNVTG,,,,,,,,,N*2E
// $GNZDA,101755.000,21,08,2024,00,00*40
// $GPTXT,01,01,01,ANTENNA OK*35

// 在线
// $GNGGA,120619.000,2235.32855,N,11358.01696,E,1,08,4.5,94.2,M,0.0,M,,*40
// $GNGLL,2235.32855,N,11358.01696,E,120619.000,A,A*43
// $GPGSA,A,3,16,27,31,194,,,,,,,,,6.9,4.5,5.2*07
// $BDGSA,A,3,09,16,39,45,,,,,,,,,6.9,4.5,5.2*2F
// $GPGSV,3,1,12,03,07,225,,04,59,276,,07,05,312,,08,61,209,*74
// $GPGSV,3,2,12,09,32,307,,16,45,019,29,21,08,180,,26,24,052,*75
// $GPGSV,3,3,12,27,80,072,27,28,11,116,,31,39,097,26,194,59,045,30*4B
// $BDGSV,3,1,09,06,,,29,07,,,29,09,59,350,29,16,70,039,33*60
// $BDGSV,3,2,09,21,,,23,29,,,25,39,70,068,37,40,,,26*59
// $BDGSV,3,3,09,45,51,004,38*5B
// $GNRMC,120619.000,A,2235.32855,N,11358.01696,E,0.00,66.27,300924,,,A*4D
// $GNVTG,66.27,T,,M,0.00,N,0.00,K,A*16
// $GNZDA,120619.000,30,09,2024,00,00*4B
// $GPTXT,01,01,01,ANTENNA OK*35


// 仅需对"GNGGA"开头的数据进行解析
// 一共14个','
void GPS_Solve(GPS* obj){
    // printf_log("%s\n", obj->rxbuf);
    // 分割出开头"$GNGGA"的数据段
    // 确定起始位置即'G'对应下标
    char copy[GPS_BUFFER_LEN];
    strcpy(copy, (char*)obj->rxbuf);
    strtok(copy, "GGA");  // 函数会将第一个'G"替换为'\0'
    uint16_t begin_index = strlen(copy);
    // 确定结束位置
    memset(copy, 0, GPS_BUFFER_LEN);
    strcpy(copy, (char*)obj->rxbuf + begin_index);
    strtok(copy, "$");
    uint8_t len = strlen(copy);

    // printf_log("%s\n", copy);

    // 寻找','出现的位置, 通过将','转换为'\0'分割字符串
    uint8_t comma_index[14] = {0};
    uint8_t cnt = 0;
    for(int i = 0; i < len; i++){
        if(copy[i] == ','){
            comma_index[cnt] = i;
            copy[i] = 0;
            cnt++;
        }
    }

    uint8_t status = atoi(copy + comma_index[5] + 1);
    if(!status) return;
    // printf_log("status: %d\n", status);

    // 读取信息, 经纬度度分格式转换为double
    obj->data.utc_time = ((uint32_t)atof(copy + comma_index[0] + 1) + 80000) % 240000;  // hhmmss格式, 北京时间比utc快8h
    double latitude_recv = atof(copy + comma_index[1] + 1) / 100;
    int latitude_int = (int)latitude_recv;
    obj->data.latitude = latitude_int + (latitude_recv - latitude_int) / 0.6;
    double longitude_recv = atof(copy + comma_index[3] + 1) / 100;
    int longitude_int = (int)longitude_recv;
    obj->data.longitude = longitude_int + (longitude_recv - longitude_int) / 0.6;

    if(*(copy + comma_index[2] + 1) == 'S') obj->data.latitude = -obj->data.latitude;
    if(*(copy + comma_index[4] + 1) == 'W') obj->data.longitude = -obj->data.longitude;

    obj->data.latitude = ButterLP_Step(obj->lat_lp);
    obj->data.longitude = ButterLP_Step(obj->lng_lp);

    // memset(obj->data.hdof_str, 0, 5);
    // strcpy(obj->data.hdof_str, copy + comma_index[7] + 1);
    // memset(obj->data.altitude_str, 0, 7);
    // strcpy(obj->data.altitude_str, copy + comma_index[8] + 1);
    // 补空格
    char hdof[5] = {0}, alt[7] = {0};
    for(int i = 0; i < 4 - strlen(copy + comma_index[7] + 1); i++){
        hdof[i] = ' ';
    }
    for(int i = 0; i < 6 - strlen(copy + comma_index[8] + 1); i++){
        alt[i] = ' ';
    }
    strcat(hdof, copy + comma_index[7] + 1);
    strcat(alt, copy + comma_index[8] + 1);
    strcpy(obj->data.hdof_str, hdof);
    strcpy(obj->data.altitude_str, alt);

    // 重置看门狗
    obj->watchdog_count = 250;
    if(obj->online_count <= 5) obj->online_count++;
}