#include "tlv493d.h"

// addr = 94

#ifdef SOFT_I2C
TLV493D* TLV493D_Init(GPIO_TypeDef* sda_gpio, uint16_t sda_pin, GPIO_TypeDef* scl_gpio, uint16_t scl_pin){
#else
TLV493D* TLV493D_3Init(I2C_HandleTypeDef* hi2c){
#endif
    TLV493D* obj = (TLV493D*)malloc(sizeof(TLV493D));
    memset(obj, 0, sizeof(TLV493D));
    #ifdef SOFT_I2C
        obj->i2c = I2C_Init(sda_gpio, sda_pin, scl_gpio, scl_pin);
        obj->x_lp = ButterLP_Init(&obj->x, 5, 0.5);
        obj->y_lp = ButterLP_Init(&obj->y, 5, 0.5);
        obj->z_lp = ButterLP_Init(&obj->z, 5, 0.5);

        // reset
        I2C_Start(obj->i2c);
        I2C_WriteByte(obj->i2c, 0xFF);
        I2C_Stop(obj->i2c);
        delay_us(200);

        TLV493D_Readout(obj, 10);

        obj->txbuf[1] = 0b00000110;
        obj->txbuf[3] = 0b01000000;
        // 将RR对应部分写入WR
        // obj->txbuf[1] |= (obj->rxbuf[7] & 0b00011000);
        // obj->txbuf[2] = obj->rxbuf[8];
        // obj->txbuf[3] |= (obj->rxbuf[9] & 0b00011111);
        obj->txbuf[1] |= (128 & 0b00011000);
        obj->txbuf[2] = 4;
        obj->txbuf[3] |= (32 & 0b00011111);
        TLV493D_SetMode(obj, MASTER_CTRL);
        TLV493D_Writeout(obj);

        delay_us(1000);

    #else
        obj->hi2c = hi2c;
    #endif
    return obj;
}

// 此函数不发送信息
void TLV493D_SetMode(TLV493D* obj, TLV493D_Mode mode){
    switch (mode)
    {
    case POWER_DOWN:
        obj->txbuf[1] &= 0b11111100;
        break;
    case FAST:
        obj->txbuf[1] |= 0b00000110;  // INT, FAST = 1
        obj->txbuf[1] &= 0b11111110; // LP = 0
        obj->txbuf[3] |= 0b01000000;  // ULP = 1
        break;
    case LOW_POWER:
        obj->txbuf[1] |= 0b00000101;  // INT, LP = 1
        obj->txbuf[1] &= 0b11111101; // FAST = 0
        obj->txbuf[3] |= 0b01000000;  // ULP = 1
        break;
    case ULTRA_LOW_POWER:
        obj->txbuf[1] |= 0b00000100;  // INT = 1
        obj->txbuf[1] &= 0b11111100; // FAST, LP = 0
        obj->txbuf[3] &= 0b10111111;  // ULP = 0
        break;
    case MASTER_CTRL:
    default:
        obj->txbuf[1] |= 0b00000011;  // FAST, LP = 1
        obj->txbuf[1] &= 0b11111011; // INT = 0
        obj->txbuf[3] |= 0b01000000;  // ULP = 1
        break;
    }
}

void TLV493D_Readout(TLV493D* obj, uint8_t len){
    // 主机发送设备地址, 从机返回数据
    I2C_Start(obj->i2c);
    I2C_WriteByte(obj->i2c, 189);
    I2C_WaitAck(obj->i2c, 10000);
    delay_us(50000);
    for(int i = 0; i < len; i++){
        obj->rxbuf[i] = I2C_ReadByte(obj->i2c);
        I2C_Ack(obj->i2c, 0);
    }
    I2C_Stop(obj->i2c);
}

void TLV493D_SetTxParity(TLV493D* obj){
    uint8_t par = 0;
    for(int i = 0; i <= 3; i++){
        for(int j = 0; j <= 7; j++){
            par += ((obj->txbuf[i] & (1 << j)) != 0);
        }
    }
    if(par % 2 == 0) obj->txbuf[1] += 128;  // P(TR 0x01 bit7)翻转
}

void TLV493D_Writeout(TLV493D* obj){
    TLV493D_SetTxParity(obj);
    I2C_Start(obj->i2c);
    I2C_WriteByte(obj->i2c, 188);
    printf_log("WR addr: %d\n", I2C_WaitAck(obj->i2c, 10000));
    for(int i = 0; i < 4; i++){
        I2C_WriteByte(obj->i2c, obj->txbuf[i]);
        printf_log("WR %02x: %d\n", i, I2C_WaitAck(obj->i2c, 10000));
    }
    I2C_Stop(obj->i2c);
}

void TLV493D_Solve(TLV493D* obj){
    // printf_log("solve: %d\n", obj->rxbuf[5] >> 4);
    // printf_log("ch: %d\n", obj->rxbuf[3] % 16);
    // if((obj->rxbuf[5] >> 4) != 0b0011) return;
    if((obj->rxbuf[5] >> 5) != 0b001) return;

    obj->x = ((obj->rxbuf[0] & 0x7F) * 16.0f + (obj->rxbuf[4] >> 4)) * 0.098;
    if(obj->rxbuf[0] & 0x80) obj->x -= 2048 * 0.098;
    obj->y = ((obj->rxbuf[1] & 0x7F) * 16.0f + (obj->rxbuf[4] % 16)) * 0.098;
    if(obj->rxbuf[1] & 0x80) obj->y -= 2048 * 0.098;
    obj->z = ((obj->rxbuf[2] & 0x7F) * 16.0f + (obj->rxbuf[5] >> 4)) * 0.098;
    if(obj->rxbuf[2] & 0x80) obj->z -= 2048 * 0.098;
    obj->temp = ((obj->rxbuf[3] & 0x70) * 16.0f + obj->rxbuf[6] - 340) * 1.1 + 24.2;
    if(obj->rxbuf[2] & 0x80) obj->temp -= 2048 * 1.1;

    // obj->x = ButterLP_Step(obj->x_lp);
    // obj->y = ButterLP_Step(obj->y_lp);
    // obj->z = ButterLP_Step(obj->z_lp);
    // printf_log("%d, %d, %d\n", (obj->rxbuf[0] & 0x7F) * 16 + (obj->rxbuf[4] >> 4), (obj->rxbuf[1] & 0x7F) * 16 + (obj->rxbuf[4] % 16), (obj->rxbuf[2] & 0x7F) * 16 + (obj->rxbuf[5] >> 4));
}