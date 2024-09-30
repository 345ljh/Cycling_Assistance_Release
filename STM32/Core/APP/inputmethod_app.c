#include "inputmethod_app.h"

// 烧录字库文件, 每次烧录1024汉字, 对应32768byte
// 1个汉字对应32byte

#ifdef FONTTYPE
uint32_t begin_addr = (FONTTYPE * 0x8000);

void InputMethodApp_FonttypeDownload(InputMethodApp* obj){
    // 按每次4096 bytes写入
    if(FONTTYPE < 26 || FONTTYPE == 30){  // 32768 bytes
        for(int i = 0; i < 8; i++){
            W25Q_WriteFlash(obj->w25q, fonttype+4096*i, begin_addr+4096*i, 4096);
        }
    } else if(FONTTYPE == 26){  // 20230 bytes
        for(int i = 0; i < 5; i++){
            W25Q_WriteFlash(obj->w25q, fonttype+4096*i, begin_addr+4096*i, (i == 4 ? 3846 : 4096));
        }
    } else if(FONTTYPE == 31){  // 21750 bytes
        for(int i = 0; i < 6; i++){
            W25Q_WriteFlash(obj->w25q, fonttype+4096*i, begin_addr+4096*i, (i == 5 ? 1270 : 4096));
        }
    }
}

#endif


InputMethodApp* InputMethodApp_Init(InputMethod* inputmethod, W25Q* w25q, TFT* tft){
    InputMethodApp* obj = (InputMethodApp*)malloc(sizeof(InputMethodApp));
    obj->w25q = w25q;
    obj->inputmethod = inputmethod;
    obj->tft = tft;
    obj->nowselect = 0;

    obj->inputmethod->buffer[0].len = 4;
    Character ch00 = {17, 7, 52}, ch01 = {23, 7, 0}, ch02 = {1, 6, 1}, ch03 = {23, 2, 16};
    obj->inputmethod->buffer[0].chs[0] = ch00;
    obj->inputmethod->buffer[0].chs[1] = ch01;
    obj->inputmethod->buffer[0].chs[2] = ch02;
    obj->inputmethod->buffer[0].chs[3] = ch03;

    // 设置默认"广东"
    obj->inputmethod->buffer[1].len = 2;
    Character ch0 = {6, 27, 0}, ch1 = {4, 21, 0};
    obj->inputmethod->buffer[1].chs[0] = ch0;
    obj->inputmethod->buffer[1].chs[1] = ch1;
    return obj;
}

/* 以下为文字显示的实现 */
char* initials[] = {
    "/", "b", "c", "ch", "d", "f", "g", "h", "j", "k", "l", "m", "n", "p", "q", "r", "s", "sh", "t", "w", "x", "y", "z", "zh"
};
char* finals[] = {
    "a", "ai", "an", "ang", "ao", "e", "ei", "en", "eng", "er", "i", "ia", "ian", "iang", "iao", "ie", "in", "ing", "iong", "iu", "o", "ong", "ou", "u", "ua", "uai", "uan", "uang", "ue", "ui", "un", "uo", "v"
};

// 按照输入法存储的数据, 在TFT屏上进行显示
/*
0~35行显示两排输入文字
当输入声母/韵母时, 每行显示3个选项, 选项下方留出2行用于光标, 最大(36/3)*(16+2)=216行, 40行开始显示 
当输入汉字时, 每行最多显示20字, 若从40行开始显示可显示11行, 共220字
一个读音最大字数>220, 因此下标跨越220时需考虑翻页
*/

// 更新输入部分
void InputMethodApp_UpdateInput(InputMethodApp* obj){
    TFT_Fill(obj->tft, 0, 40, 320, 240, 0);
    if(obj->inputmethod->operation == 0){
        for(int i = 0; i < INITIAL_AMOUNT; i++){
            TFT_DrawStr_ascii(obj->tft, i % 3 * 80, 40 + i / 3 * 18, initials[i], 0xFFFF, 6);
        }
    } else if(obj->inputmethod->operation == 1){
        for(int i = 0; i < FINAL_AMOUNT; i++){
            TFT_DrawStr_ascii(obj->tft, i % 3 * 80, 40 + i / 3 * 18, finals[i], 
            character_len[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + (uint8_t)i] ? 0xFFFF : 0, 6);
        }
    } else if(obj->inputmethod->operation == 2){
        if(obj->nowselect >= 220){
            uint16_t len = character_len[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].final] - 220;
            uint8_t buf[32 * len];
            W25Q_ReadFlash(obj->w25q, buf, 32 * (character_offset[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].final] + 220), 32 * len);
            for(int i = 0; i < len; i++){
                TFT_DrawStr_fonttype(obj->tft, i % 20 * 16, 40 + i / 20 * 18, buf + 32 * i, 0xFFFF);
            }
        } else {
            uint16_t len = (character_len[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].final] >= 220 ?
            220 : character_len[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].final]);
            uint8_t buf[32 * len];
            W25Q_ReadFlash(obj->w25q, buf, 32 * character_offset[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].final], 32 * len);
            for(int i = 0; i < len; i++){
                TFT_DrawStr_fonttype(obj->tft, i % 20 * 16, 40 + i / 20 * 18, buf + 32 * i, 0xFFFF);
            }
        }
    } else if(obj->inputmethod->operation == 3){
        for(int i = 0; i < 95; i++){
            char ch = i + 32;
            TFT_DrawStr_ascii(obj->tft, i % 20 * 16 + 4, 40 + i / 20 * 18, &ch, 0xFFFF, 1);
        }
    }
}

// 更新光标
// 行数为40+17+18n
void InputMethodApp_UpdateSelection(InputMethodApp* obj){
    uint8_t clear_line = (obj->lastselect % 220) / (obj->inputmethod->operation >= 2 ? 20 : 3);
    TFT_DrawLine(obj->tft, 0, 57 + clear_line * 18, 320, 57 + clear_line * 18, 0);
    TFT_DrawLine(obj->tft, 0, 56 + clear_line * 18, 320, 56 + clear_line * 18, 0);

    if(obj->inputmethod->operation == 0 || obj->inputmethod->operation == 1){
        TFT_DrawLine(obj->tft, obj->nowselect % 3 * 80, 57 + obj->nowselect / 3 * 18, obj->nowselect % 3 * 80 + 32, 57 + obj->nowselect / 3 * 18, RGB2565(255, 128, 128));
        TFT_DrawLine(obj->tft, obj->nowselect % 3 * 80, 56 + obj->nowselect / 3 * 18, obj->nowselect % 3 * 80 + 32, 56 + obj->nowselect / 3 * 18, RGB2565(255, 128, 128));
    } else if(obj->inputmethod->operation == 2){
        TFT_DrawLine(obj->tft, (obj->nowselect % 220) % 20 * 16, 57 + (obj->nowselect % 220) / 20 * 18, (obj->nowselect % 220) % 20 * 16 + 16, 57 + (obj->nowselect % 220) / 20 * 18, RGB2565(255, 128, 128));
        TFT_DrawLine(obj->tft, (obj->nowselect % 220) % 20 * 16, 56 + (obj->nowselect % 220) / 20 * 18, (obj->nowselect % 220) % 20 * 16 + 16, 56 + (obj->nowselect % 220) / 20 * 18, RGB2565(255, 128, 128));
    } else if(obj->inputmethod->operation == 3){
        TFT_DrawLine(obj->tft, obj->nowselect % 20 * 16, 57 + obj->nowselect / 20 * 18, obj->nowselect % 20 * 16 + 16, 57 + obj->nowselect / 20 * 18, RGB2565(255, 128, 128));
        TFT_DrawLine(obj->tft, obj->nowselect % 20 * 16, 56 + obj->nowselect / 20 * 18, obj->nowselect % 20 * 16 + 16, 56 + obj->nowselect / 20 * 18, RGB2565(255, 128, 128));
    }
}

// 更新已输入文字
void InputMethodApp_UpdateTyped(InputMethodApp* obj){
    TFT_Fill(obj->tft, 0, 0, 320, 36, 0);
    for(int i = 0; i < obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len; i++){
        if(obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[i].initial != 0xFF){
            uint8_t buf[32];
            W25Q_ReadFlash(obj->w25q, buf, 32 * (character_offset[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[i].initial * FINAL_AMOUNT + obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[i].final] + obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[i].index), 32);
            TFT_DrawStr_fonttype(obj->tft, i % 20 * 16, i / 20 * 18, buf, RGB2565(255, 242, 0));
        } else {
            char ch = obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[i].index + 32;
            TFT_DrawStr_ascii(obj->tft, i % 20 * 16 + 4, i / 20 * 18, &ch, RGB2565(255, 242, 0), 1);
        }
    }
}

/* 以下为操作部分, 通过中断调用 */
// 前一选项
void InputMethodApp_Prev(InputMethodApp* obj){
    obj->lastselect = obj->nowselect;
    while(1){
        obj->nowselect--;
        if(obj->inputmethod->operation == 0)
        {
            if(obj->nowselect < 0) obj->nowselect += INITIAL_AMOUNT;
            break;
        } else if(obj->inputmethod->operation == 1){
            if(obj->nowselect < 0) obj->nowselect += FINAL_AMOUNT;
            if(obj->nowselect < FINAL_AMOUNT && character_len[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + obj->nowselect]) break;
        } else if(obj->inputmethod->operation == 2){
            if(obj->nowselect < 0) obj->nowselect += character_len[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].final];
            break;
        } else if(obj->inputmethod->operation == 3){
            if(obj->nowselect < 0) obj->nowselect += ASCII_AMOUNT;
            break;
        }
    }
    if((obj->inputmethod->operation == 2) & ((obj->nowselect < 220) ^ (obj->lastselect < 220))) InputMethodApp_UpdateInput(obj);
    InputMethodApp_UpdateSelection(obj);
}

// 后一选项
void InputMethodApp_Next(InputMethodApp* obj){
    obj->lastselect = obj->nowselect;
    while(1){
        obj->nowselect++;
        if(obj->inputmethod->operation == 0)
        {
            obj->nowselect %= INITIAL_AMOUNT;
            break;
        } else if(obj->inputmethod->operation == 1){
            obj->nowselect %= FINAL_AMOUNT;
            if(obj->nowselect < FINAL_AMOUNT && character_len[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + obj->nowselect]) break;
        } else if(obj->inputmethod->operation == 2){
            obj->nowselect %= character_len[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].final];
            break;
        } else if(obj->inputmethod->operation == 3){
            obj->nowselect %= ASCII_AMOUNT;
            break;
        }
    }
    if((obj->inputmethod->operation == 2) & ((obj->nowselect < 220) ^ (obj->lastselect < 220))) InputMethodApp_UpdateInput(obj);
    InputMethodApp_UpdateSelection(obj);
}

// 向上
void InputMethodApp_Up(InputMethodApp* obj){
    obj->lastselect = obj->nowselect;
    if(obj->inputmethod->operation == 0)
    {
        obj->nowselect -= 3;
        if(obj->nowselect < 0) obj->nowselect += INITIAL_AMOUNT;
    } else if(obj->inputmethod->operation == 1){
        int8_t next_sel = obj->nowselect;
        while(1){
            next_sel -= 3;
            if(next_sel < 0) next_sel += FINAL_AMOUNT;
            if(next_sel == obj->nowselect || (next_sel < FINAL_AMOUNT && character_len[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + next_sel])){
                obj->nowselect = next_sel;
                break;
            }
        }

    } else if(obj->inputmethod->operation == 2){
        uint16_t chs = character_len[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].final];
        if(obj->nowselect % 20 >= chs % 20){
            obj->nowselect -= 20;
            if(obj->nowselect < 0) obj->nowselect += chs / 20 * 20;
        } else {
            obj->nowselect -= 20;
            if(obj->nowselect < 0) obj->nowselect += chs / 20 * 20 + 20;
        }
    } else if(obj->inputmethod->operation == 3){
        if(obj->nowselect % 20 >= ASCII_AMOUNT % 20){
            obj->nowselect -= 20;
            if(obj->nowselect < 0) obj->nowselect += ASCII_AMOUNT / 20 * 20;
        } else {
            obj->nowselect -= 20;
            if(obj->nowselect < 0) obj->nowselect += ASCII_AMOUNT / 20 * 20 + 20;
        }
    }
    if((obj->inputmethod->operation == 2) & ((obj->nowselect < 220) ^ (obj->lastselect < 220))) InputMethodApp_UpdateInput(obj);
    InputMethodApp_UpdateSelection(obj);
}

// 向下
void InputMethodApp_Down(InputMethodApp* obj){
    obj->lastselect = obj->nowselect;
    if(obj->inputmethod->operation == 0)
    {
        obj->nowselect += 3;
        obj->nowselect %= INITIAL_AMOUNT;
    } else if(obj->inputmethod->operation == 1){
        int8_t next_sel = obj->nowselect;
        while(1){
            next_sel += 3;
            next_sel %= FINAL_AMOUNT;
            if(next_sel == obj->nowselect || (next_sel < FINAL_AMOUNT && character_len[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + next_sel])){
                obj->nowselect = next_sel;
                break;
            }
        }

    } else if(obj->inputmethod->operation == 2){
        uint16_t chs = character_len[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].final];
            obj->nowselect += 20;
            if(obj->nowselect >= chs) obj->nowselect %= 20;
    } else if(obj->inputmethod->operation == 3){
            obj->nowselect += 20;
            if(obj->nowselect >= ASCII_AMOUNT) obj->nowselect %= 20;
    }
    if((obj->inputmethod->operation == 2) & ((obj->nowselect < 220) ^ (obj->lastselect < 220))) InputMethodApp_UpdateInput(obj);
    InputMethodApp_UpdateSelection(obj);
}

// 确定
void InputMethodApp_Ensure(InputMethodApp* obj){
    obj->lastselect = obj->nowselect;
    InputMethod_Add(obj->inputmethod, obj->nowselect);
    obj->nowselect = 0;
    if(obj->inputmethod->operation == 1){
        while(1){
            if(character_len[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + obj->nowselect]) break;
            obj->nowselect++;
        }
    }

    InputMethodApp_UpdateInput(obj);
    InputMethodApp_UpdateSelection(obj);
    InputMethodApp_UpdateTyped(obj);
}

// 输入ascii字符, 仅在输入法0状态下有效
void InputMethodApp_Ascii(InputMethodApp* obj){
    obj->lastselect = obj->nowselect;
    if(obj->inputmethod->operation == 0){
        InputMethod_Add(obj->inputmethod, 0xFF);

        InputMethodApp_UpdateInput(obj);
        InputMethodApp_UpdateSelection(obj);
        InputMethodApp_UpdateTyped(obj);
    }
}

// 删除
void InputMethodApp_Delete(InputMethodApp* obj){
    obj->lastselect = obj->nowselect;
    InputMethod_Delete(obj->inputmethod);
    obj->nowselect = 0;
    if(obj->inputmethod->operation == 1){
        while(1){
            if(character_len[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].chs[obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len].initial * FINAL_AMOUNT + obj->nowselect]) break;
            obj->nowselect++;
        }
    }

    InputMethodApp_UpdateInput(obj);
    InputMethodApp_UpdateSelection(obj);
    InputMethodApp_UpdateTyped(obj);
}

// 清空
void InputMethodApp_Clear(InputMethodApp* obj){
    obj->lastselect = obj->nowselect;
    do{
        InputMethod_Delete(obj->inputmethod);
    }while(obj->inputmethod->buffer[obj->inputmethod->now_buffer_index].len || obj->inputmethod->operation);
    obj->nowselect = 0;

    InputMethodApp_UpdateInput(obj);
    InputMethodApp_UpdateSelection(obj);
    InputMethodApp_UpdateTyped(obj);
}

// 切换内容
void InputMethodApp_Switch(InputMethodApp* obj){
    InputMethod_Switch(obj->inputmethod);

    InputMethodApp_UpdateInput(obj);
    InputMethodApp_UpdateSelection(obj);
    InputMethodApp_UpdateTyped(obj);
}