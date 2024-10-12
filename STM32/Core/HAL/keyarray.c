#include "keyarray.h"

KeyArray* KeyArray_Init(GPIO_TypeDef** row_gpio, uint16_t* row_pin, GPIO_TypeDef** col_gpio, uint16_t* col_pin){
    KeyArray* obj = (KeyArray*)malloc(sizeof(KeyArray));
    memset(obj, 0, sizeof(KeyArray));
    memcpy(obj->row_gpio, row_gpio, KEY_ROW * sizeof(GPIO_TypeDef*));
    memcpy(obj->row_pin, row_pin, KEY_ROW * sizeof(uint16_t));
    memcpy(obj->col_gpio, col_gpio, KEY_COL * sizeof(GPIO_TypeDef*));
    memcpy(obj->col_pin, col_pin, KEY_COL * sizeof(uint16_t));
    for(int i = 0; i < KEY_ROW; i++){
        HAL_GPIO_WritePin(obj->row_gpio[i], obj->row_pin[i], 0);
    }
    return obj;
}

// 确定按下的按键
void KeyArray_Judge(KeyArray* obj, uint16_t GPIO_Pin){
    uint8_t tmp = obj->now_key;
    tmp = 0;
    for(int i = 0; i < KEY_COL; i++){
        if(obj->col_pin[i] == GPIO_Pin || !HAL_GPIO_ReadPin(obj->col_gpio[i], obj->col_pin[i])){
            tmp += i;
            break;
        }
    }

    for(int i = 0; i < KEY_ROW; i++){
        HAL_GPIO_WritePin(obj->row_gpio[i], obj->row_pin[i], 1);
        if(HAL_GPIO_ReadPin(obj->col_gpio[tmp], obj->col_pin[tmp])){
            tmp += KEY_COL * i;
            HAL_GPIO_WritePin(obj->row_gpio[i], obj->row_pin[i], 0);  
            break;
        }
        HAL_GPIO_WritePin(obj->row_gpio[i], obj->row_pin[i], 0);
    }

    if(tmp > obj->now_key) obj->now_key = tmp;

}