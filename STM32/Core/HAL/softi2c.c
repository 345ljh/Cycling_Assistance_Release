#include "softi2c.h"
#include "bsp_log.h"

#define SCL_Write(a) HAL_GPIO_WritePin(obj->scl_gpio, obj->scl_pin, a)
#define SDA_Write(a) HAL_GPIO_WritePin(obj->sda_gpio, obj->sda_pin, a)
#define SDA_READ HAL_GPIO_ReadPin(obj->sda_gpio, obj->sda_pin)
#define SDA_OUT I2C_SetOUT(obj->sda_gpio, obj->sda_pin)
#define SDA_IN I2C_SetIN(obj->sda_gpio, obj->sda_pin)
#define SCL_OUT I2C_SetOUT(obj->scl_gpio, obj->scl_pin)
#define SCL_IN I2C_SetIN(obj->scl_gpio, obj->scl_pin)

void I2C_SetOUT(GPIO_TypeDef* gpio, uint16_t pin){
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(gpio, &GPIO_InitStruct);
}

void I2C_SetIN(GPIO_TypeDef* gpio, uint16_t pin){
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(gpio, &GPIO_InitStruct);
}

SoftI2C* I2C_Init(GPIO_TypeDef* sda_gpio, uint16_t sda_pin, GPIO_TypeDef* scl_gpio, uint16_t scl_pin){
    SoftI2C* obj = (SoftI2C*)malloc(sizeof(SoftI2C));
    obj->sda_gpio = sda_gpio;
    obj->sda_pin = sda_pin;
    obj->scl_gpio = scl_gpio;
    obj->scl_pin = scl_pin;
    SCL_Write(1);
    SDA_Write(1);
    return obj;
}

void I2C_Start(SoftI2C* obj){
    SDA_OUT;
    SDA_Write(1);
    SCL_Write(1);
    delay_us(2);
    SDA_Write(0);
    delay_us(2);
    SCL_Write(0);
}

void I2C_Stop(SoftI2C* obj){
    SDA_OUT;
    SCL_Write(0);
    SDA_Write(0);
    delay_us(2);
    SCL_Write(1);
    SDA_Write(1);
    delay_us(2);
}

// 0为ack
void I2C_Ack(SoftI2C* obj, uint8_t ack){
    SDA_OUT;
    SDA_Write(ack);
    SCL_Write(1);
    delay_us(2);
    SCL_Write(0);
    delay_us(2);
}

uint8_t I2C_WaitAck(SoftI2C* obj, uint16_t timeout){
    SDA_Write(1);
    SDA_IN;
    delay_us(2);
    SCL_Write(1);
    delay_us(2);
    while(SDA_READ){
        if(timeout == 0){
            I2C_Stop(obj);
            return -1;
        } else timeout--;
    }
    SCL_Write(0);
    return 0;
}

void I2C_WriteByte(SoftI2C* obj, uint8_t buf){
    SDA_OUT;
    SCL_Write(0);
    for(int i = 0; i < 8; i++){
        SDA_Write((buf & 0x80) != 0);
        buf <<= 1;
        delay_us(2);
        SCL_Write(1);
        delay_us(2);
        SCL_Write(0);
    }
}

uint8_t I2C_ReadByte(SoftI2C* obj){
    uint8_t tmp = 0;
    SDA_IN;
    for(int i = 0; i < 8; i++){
        SCL_Write(0);
        delay_us(2);
        SCL_Write(1);
        tmp <<= 1;
        if(SDA_READ) tmp++;
        delay_us(2);
    }
    SCL_Write(0);
    return tmp;
}