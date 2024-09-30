#ifndef _KEYARRAY_H
#define _KEYARRAY_H

#include "gpio.h"
#include "malloc.h"
#include "string.h"

#define KEY_ROW 3
#define KEY_COL 4

typedef struct keyarray_t
{
    // 3行4列, 仅支持同时按1键
    // row低电平, 下降沿触发col中断
    GPIO_TypeDef* row_gpio[KEY_ROW];
    uint16_t row_pin[KEY_ROW];
    GPIO_TypeDef* col_gpio[KEY_COL];
    uint16_t col_pin[KEY_COL];
    uint8_t now_key;
    uint32_t last_irq_time;
}KeyArray;

KeyArray* KeyArray_Init(GPIO_TypeDef** row_gpio, uint16_t* row_pin, GPIO_TypeDef** col_gpio, uint16_t* col_pin);
void KeyArray_Judge(KeyArray* obj, uint16_t GPIO_Pin);
#endif