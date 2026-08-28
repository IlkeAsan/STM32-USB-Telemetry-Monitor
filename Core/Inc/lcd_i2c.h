/**
  ******************************************************************************
  * @file    lcd_i2c.h
  * @brief   I2C 16x2 / 20x4 LCD (PCF8574) Driver for STM32F439ZI (PB8=SCL, PB9=SDA)
  ******************************************************************************
  */

#ifndef __LCD_I2C_H
#define __LCD_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define LCD_ADDR            0x4E  /* 0x27 << 1 */

#define LCD_RS_PIN          (1 << 0)
#define LCD_EN_PIN          (1 << 2)
#define LCD_BACKLIGHT_PIN   (1 << 3)

void LCD_I2C_Init(void);
void LCD_Init(void);
void LCD_SendCommand(uint8_t cmd);
void LCD_SendData(uint8_t data);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Print(const char *str);
void LCD_Clear(void);
void LCD_DisplayRAM(const char *payload);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_I2C_H */
