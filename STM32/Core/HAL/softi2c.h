#ifndef _I2C_H
#define _I2C_H

#include "gpio.h"
#include "malloc.h"
#include "math.h"

typedef struct softi2c_t
{
    GPIO_TypeDef* sda_gpio;
    uint16_t sda_pin;
    GPIO_TypeDef* scl_gpio;
    uint16_t scl_pin;
}SoftI2C;

SoftI2C* I2C_Init(GPIO_TypeDef* sda_gpio, uint16_t sda_pin, GPIO_TypeDef* scl_gpio, uint16_t scl_pin);
void I2C_Start(SoftI2C* obj);
void I2C_Stop(SoftI2C* obj);
void I2C_Ack(SoftI2C* obj, uint8_t ack);
uint8_t I2C_WaitAck(SoftI2C* obj, uint16_t timeout);
void I2C_WriteByte(SoftI2C* obj, uint8_t buf);
uint8_t I2C_ReadByte(SoftI2C* obj);

#endif