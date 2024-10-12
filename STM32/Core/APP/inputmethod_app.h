#ifndef _INPUTMETHOD_APP_H
#define _INPUTMETHOD_APP_H

#include "inputmethod.h"
#include "stdint.h"
#include "w25q.h"
#include "tft.h"

// #define FONTTYPE 31

typedef struct inputmethod_app_t
{
    W25Q* w25q;  // 储存字库
    InputMethod* inputmethod;
    TFT* tft;
    int16_t nowselect;  // 记录当前选定的下标
    int16_t lastselect;  // 上一时刻选定的下标, 用于优化屏幕刷新
}InputMethodApp;

InputMethodApp* InputMethodApp_Init(InputMethod* inputmethod, W25Q* w25q, TFT* tft);

#ifdef FONTTYPE
void InputMethodApp_FonttypeDownload(InputMethodApp* obj);
#endif

void InputMethodApp_UpdateInput(InputMethodApp* obj);
void InputMethodApp_UpdateSelection(InputMethodApp* obj);
void InputMethodApp_UpdateTyped(InputMethodApp* obj);

void InputMethodApp_Save(InputMethodApp* obj);
void InputMethodApp_Load(InputMethodApp* obj);

void InputMethodApp_Prev(InputMethodApp* obj);
void InputMethodApp_Next(InputMethodApp* obj);
void InputMethodApp_Up(InputMethodApp* obj);
void InputMethodApp_Down(InputMethodApp* obj);
void InputMethodApp_Ensure(InputMethodApp* obj);
void InputMethodApp_Delete(InputMethodApp* obj);
void InputMethodApp_Clear(InputMethodApp* obj);
void InputMethodApp_Ascii(InputMethodApp* obj);
void InputMethodApp_Switch(InputMethodApp* obj);
#endif