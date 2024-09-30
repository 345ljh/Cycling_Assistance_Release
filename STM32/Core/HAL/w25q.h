#ifndef _W25Q_H
#define _W25Q_H

#include "stdint.h"
#include "string.h"
#include "gpio.h"
#include "spi.h"
#include "malloc.h"
#include "bsp_log.h"

typedef struct{
    // spi端口
    SPI_HandleTypeDef* hspi;
    // cs gpio引脚
    GPIO_TypeDef* cs_gpio;
    uint16_t cs_pin;
}W25Q;

W25Q* W25Q_Init(SPI_HandleTypeDef* hspi, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void W25Q_ReadID(W25Q* obj, uint8_t* manuf_id, uint8_t* dev_id);
void W25Q_ReadFlash(W25Q* obj, uint8_t* buf, uint32_t begin_addr, uint16_t len);
void W25Q_WriteEnable(W25Q* obj);
void W25Q_WriteDisable(W25Q* obj);
uint8_t W25Q_GetStatus(W25Q* obj);
void W25Q_Clear(W25Q* obj);
void W25Q_WriteFlash(W25Q* obj, uint8_t* buf, uint32_t begin_addr, uint16_t len);

#endif