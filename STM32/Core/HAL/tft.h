#ifndef TFT_H
#define TFT_H

#define USE_HORIZONTAL 2  //设置横屏或者竖屏显示 0或1为竖屏 2或3为横屏

#if USE_HORIZONTAL == 0 || USE_HORIZONTAL == 1
    #define WIDTH 240
    #define HEIGHT 320
#else
    #define WIDTH 320
    #define HEIGHT 240
#endif

#define PI 3.141592
#define FRAME_BUF_SIDE 200

#include "spi.h"
#include "tim.h"
#include "gpio.h"
#include "math.h"
#include "string.h"
#include "malloc.h"

extern uint8_t TFT_chara[][16];
typedef struct{
    // spi端口
    SPI_HandleTypeDef* hspi;
    // gpio引脚
    // 片选
    GPIO_TypeDef* cs_gpio;
    uint16_t cs_pin;
    // 选择发送指令或数据(a0)
    GPIO_TypeDef* dc_gpio;
    uint16_t dc_pin;
    // 背光
    GPIO_TypeDef* bl_gpio;
    uint16_t bl_pin;
    // 复位
    GPIO_TypeDef* re_gpio;
    uint16_t re_pin;

    // 使用缓冲区储存一幅图像, 每一像素使用2bit(每个u8采用大端存储), 支持4种颜色
    // 修改缓冲区后, 可记录修改的范围(需在外部修改), 下一次加载图像时可仅修改局部, 提高效率
    uint8_t frame_buf[FRAME_BUF_SIDE][FRAME_BUF_SIDE / 4];
    // 以下变量*相对于图片*, 而非相对屏幕
    uint8_t frame_x1, frame_x2, frame_y1, frame_y2;
}TFT;

uint16_t RGB2565(uint8_t red, uint8_t green, uint8_t blue);

TFT* TFT_Init(SPI_HandleTypeDef* hspi, GPIO_TypeDef *cs_gpio, uint16_t cs_pin,
										GPIO_TypeDef *dc_gpio, uint16_t dc_pin,
										GPIO_TypeDef *bl_gpio, uint16_t bl_pin,
										GPIO_TypeDef *re_gpio, uint16_t re_pin);
void TFT_HardInit(TFT* obj);
void TFT_Fill(TFT* obj, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void TFT_DrawLine(TFT* obj, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void TFT_DrawArc(TFT* obj, uint16_t centerx, uint16_t centery, uint16_t radius, int16_t deg1, int16_t deg2, uint16_t color);
void TFT_DrawStr_ascii(TFT* obj, uint16_t x, uint16_t y, char* str, uint16_t color, uint8_t max_len);
void TFT_DrawStr_fonttype(TFT* obj, uint16_t x, uint16_t y, uint8_t* buf, uint16_t color);
void TFT_DrawFrame(TFT* obj, uint16_t x, uint16_t y);
void TFT_DrawFramePart(TFT* obj, uint16_t x, uint16_t y);
char* TFT_Itoa(short num);

#endif