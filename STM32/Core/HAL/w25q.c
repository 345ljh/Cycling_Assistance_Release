#include "w25q.h"

// spi需设置为8分频

W25Q* W25Q_Init(SPI_HandleTypeDef* hspi, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin){
    W25Q* obj = (W25Q*)malloc(sizeof(W25Q));
    obj->hspi = hspi;
    obj->cs_gpio = GPIOx;
    obj->cs_pin = GPIO_Pin;
    return obj;
}

// 读取芯片id, 可用于测试spi
void W25Q_ReadID(W25Q* obj, uint8_t* manuf_id, uint8_t* dev_id){
    uint8_t a = 0x90, b = 0x00, c = 0x11;
    HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 0);
    HAL_SPI_TransmitReceive(&hspi2, &a, manuf_id, 1, 100);
    HAL_SPI_TransmitReceive(&hspi2, &b, manuf_id, 1, 100);
    HAL_SPI_TransmitReceive(&hspi2, &b, manuf_id, 1, 100);
    HAL_SPI_TransmitReceive(&hspi2, &b, manuf_id, 1, 100);
    HAL_SPI_TransmitReceive(&hspi2, &c, manuf_id, 1, 100);
    HAL_SPI_TransmitReceive(&hspi2, &c, dev_id, 1, 100);
    HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 1);
}

// 读取flash, 起始地址为24位
void W25Q_ReadFlash(W25Q* obj, uint8_t* buf, uint32_t begin_addr, uint16_t len){
    uint8_t a = 0x03, b;
    uint8_t addr2 = (uint8_t)(begin_addr >> 16), addr1 = (uint8_t)(begin_addr >> 8), addr0 = (uint8_t)begin_addr;
    while(W25Q_GetStatus(obj) & 0x01);
    HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 0);
    HAL_SPI_TransmitReceive(&hspi2, &a, &b, 1, 100);
    // 发送地址
    HAL_SPI_TransmitReceive(&hspi2, &addr2, &b, 1, 100);
    HAL_SPI_TransmitReceive(&hspi2, &addr1, &b, 1, 100);
    HAL_SPI_TransmitReceive(&hspi2, &addr0, &b, 1, 100);
    // 读取数据
    for(int i = 0; i < len; i++){
        HAL_SPI_TransmitReceive(&hspi2, &a, buf + i, 1, 100);
    }
    HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 1);
    printf_log("%06x read\n", begin_addr);
}

// 读状态
uint8_t W25Q_GetStatus(W25Q* obj){
    uint8_t a = 0x05, b;
    HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 0);
    HAL_SPI_TransmitReceive(&hspi2, &a, &b, 1, 100);
    HAL_SPI_TransmitReceive(&hspi2, &a, &b, 1, 100);
    HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 1);
    return b;
}
// 写使能
void W25Q_WriteEnable(W25Q* obj){
    uint8_t a = 0x06, b;
    HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 0);
    HAL_SPI_TransmitReceive(&hspi2, &a, &b, 1, 100);
    HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 1);
}

// 写失能
void W25Q_WriteDisable(W25Q* obj){
    uint8_t a = 0x04, b;
    HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 0);
    HAL_SPI_TransmitReceive(&hspi2, &a, &b, 1, 100);
    HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 1);
}

// 擦除芯片
void W25Q_Clear(W25Q* obj){
    // 写使能
    W25Q_WriteEnable(obj);

    uint8_t a = 0xC7, b;
    while(W25Q_GetStatus(obj) & 0x01);
    HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 0);
    HAL_SPI_TransmitReceive(&hspi2, &a, &b, 1, 100);
    HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 1);
}

// 写数据最小单位为页(256 bytes), 擦除最小单位为扇区(4K)
// 只能在0xFF的区域写

uint8_t temp[4096];

// 写扇区
void W25Q_WriteFlash(W25Q* obj, uint8_t* buf, uint32_t begin_addr, uint16_t len){
    // 扇区起始字节
    uint32_t sector_addr = begin_addr / 4096 * 4096;
    uint8_t addr2 = (uint8_t)(sector_addr >> 16), addr1 = (uint8_t)(sector_addr >> 8), addr0 = (uint8_t)sector_addr;
    // 计算剩余字节数, 防止覆盖起始内容
    uint16_t rest_data = 4096 - (begin_addr - sector_addr);
    if(rest_data < len) len = rest_data;
    // 备份当前数据
    memset(temp, 0xFF, 4096);
    W25Q_ReadFlash(obj, temp, sector_addr, 4096);
    // 临时数组内操作
    memcpy(temp + (begin_addr - sector_addr), buf, len);
    // 擦除扇区
    while(W25Q_GetStatus(obj) & 0x01);
    W25Q_WriteEnable(obj);
    uint8_t a = 0x20, d;
    HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 0);
    HAL_SPI_TransmitReceive(&hspi2, &a, &d, 1, 100);
    HAL_SPI_TransmitReceive(&hspi2, &addr2, &d, 1, 100);
    HAL_SPI_TransmitReceive(&hspi2, &addr1, &d, 1, 100);
    HAL_SPI_TransmitReceive(&hspi2, &addr0, &d, 1, 100);
    HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 1);
    // 写入数据, 依次写16页
    for(int i = 0; i < 16; i++){
        // // 若全为0xff则跳过
        // uint8_t is_all_ff = 1;
        // for(int j = 0; j < 256; j++){
        //     if(temp[i * 256 + j] != 0xFF){
        //         is_all_ff = 0;
        //         break;
        //     }
        // }
        // if(is_all_ff) continue;
        // 发送指令
        while(W25Q_GetStatus(obj) & 0x01);
        W25Q_WriteEnable(obj);
        uint8_t c = 0x02;
        addr2 = (uint8_t)(sector_addr >> 16), addr1 = (uint8_t)((sector_addr >> 8) + i), addr0 = (uint8_t)sector_addr;
        HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 0);
        HAL_SPI_TransmitReceive(&hspi2, &c, &d, 1, 100);
        HAL_SPI_TransmitReceive(&hspi2, &addr2, &d, 1, 100);
        HAL_SPI_TransmitReceive(&hspi2, &addr1, &d, 1, 100);
        HAL_SPI_TransmitReceive(&hspi2, &addr0, &d, 1, 100);
        // 写数据
        for(int j = 0; j < 256; j++){
            HAL_SPI_TransmitReceive(&hspi2, temp + i * 256 + j, &d, 1, 100);
        }
        HAL_GPIO_WritePin(obj->cs_gpio, obj->cs_pin, 1);
        printf_log("%06x written\n", sector_addr + i * 256);
    }
}