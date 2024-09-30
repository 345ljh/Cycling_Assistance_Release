#ifndef _TLV493D_H
#define _TLV493D_H

#include "gpio.h"
#include "malloc.h"
#include "softi2c.h"
#include "butterlp.h"
#include "math.h"
#include "tft.h"
#include "bsp_log.h"

#define SOFT_I2C

/* 寄存器说明
read reg(RR)
0x00: Bx[11:4] x轴数据
0x01: By[11:4] y轴数据
0x02: Bz[11:4] z轴数据

0x03:
Temp[11:8] 温度
FRM[3:2] 转换完成后递增
CH[1:0] 读数时应为00, 否则表示正在进行y/z/t转换

0x04: Bx[3:0] By[3:0]

0x05:
Reserved[1bit]
T[1bit] 0表示数据有效
FF[1bit] 1表示内部校验位(?)正常
PD[1bit] 1表示测量完成
Bz[3:0]

0x06: Temp[7:0]
0x07: RES1[7:0]  需要写入WR 0x01
0x08: RES2[7:0]  需要写入WR 0x02
0x09: RES3[7:0]  需要写入WR 0x03

write reg(WR)
0x00: Reserved

0x01:
P[1bit] 校验位, WR 32bit之和必须为奇数
IICAddr[2bit] 设置从机地址, 默认00b
RES1[2bit] 写入WH 0x07[4:3]
INT[1bit] 1开启中断脉冲输出
FAST[1bit] 1开启fast mode
LOW[1bit] 1开启low-power mode

0x02: RES2[8bit] 写入WH 0x08[7:0]

0x03:
T[1bit]  0开启温度测量
LP[1bit] 0开启ulp mode
PT[1bit] 1开启校验位
RES3[5bit] 写入WH 0x09[4:0]
*/

typedef struct tlv493d_t
{
    #ifdef SOFT_I2C
        SoftI2C* i2c;
    #else
        I2C_HandleTypeDef* hi2c;
    #endif

    uint8_t rxbuf[10];
    uint8_t txbuf[4];
    double x, y, z, temp;
    ButterLP *x_lp, *y_lp, *z_lp;
}TLV493D;

typedef enum{
    POWER_DOWN = 0,
    FAST, LOW_POWER, ULTRA_LOW_POWER, MASTER_CTRL
}TLV493D_Mode;

#ifdef SOFT_I2C
TLV493D* TLV493D_Init(GPIO_TypeDef* sda_gpio, uint16_t sda_pin, GPIO_TypeDef* scl_gpio, uint16_t scl_pin);
#else
TLV493D* TLV493D_Init(I2C_HandleTypeDef* hi2c);
#endif

void I2C_WriteByte(SoftI2C* obj, uint8_t buf);
void TLV493D_SetMode(TLV493D* obj, TLV493D_Mode mode);
void TLV493D_Readout(TLV493D* obj, uint8_t len);
void TLV493D_Writeout(TLV493D* obj);
void TLV493D_SetTxParity(TLV493D* obj);
void TLV493D_Solve(TLV493D* obj);
#endif