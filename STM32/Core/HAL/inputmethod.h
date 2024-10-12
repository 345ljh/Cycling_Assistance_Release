#ifndef _INPUTMETHOD_H
#define _INPUTMETHOD_H

#include "stdint.h"
#include "string.h"
#include "malloc.h"

#define INPUT_MAX_LEN 30
#define INITIAL_AMOUNT 24
#define FINAL_AMOUNT 33
#define ASCII_AMOUNT 95

#define INPUTMETHOD_DATA_AMOUNT 2


extern uint16_t character_len[];
extern uint16_t character_offset[];

typedef struct character_t  // 使用声母,韵母,下标对应一个唯一汉字3
{
    uint8_t initial;
    uint8_t final;
    uint16_t index;
}Character;

typedef struct inputmethod_data_t{
    uint8_t len;  // 当前完整的字符长度, 对应此时操作的buffer下标
    Character chs[INPUT_MAX_LEN];   // 储存文字
}InputMethodData;
typedef struct inputmethod_t{
    InputMethodData buffer[INPUTMETHOD_DATA_AMOUNT];
    uint8_t now_buffer_index;
    uint8_t operation;
    /* operation状态定义如下
    0: 即将输入一个声母
    1: 即将输入一个韵母
    2: 即将选择一个字
    */
}InputMethod;

InputMethod* InputMethod_Init(void);
void InputMethod_Add(InputMethod* obj, uint16_t code);
void InputMethod_Delete(InputMethod* obj);
void InputMethod_Switch(InputMethod* obj);

#endif