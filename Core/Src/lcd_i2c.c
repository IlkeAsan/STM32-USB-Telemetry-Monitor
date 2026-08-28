/**
  ******************************************************************************
  * @file    lcd_i2c.c
  * @brief   I2C 16x2 / 20x4 LCD (PCF8574) Driver (PB8=SCL, PB9=SDA)
  ******************************************************************************
  */

#include "lcd_i2c.h"
#include <string.h>
#include <stdio.h>

static void LCD_Delay_ms(uint32_t ms)
{
  HAL_Delay(ms);
}

void LCD_I2C_Init(void)
{
  /* 1. Clock Hatlarini Ac (GPIOB ve I2C1) */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_I2C1_CLK_ENABLE();

  /* 2. PB8 (SCL) ve PB9 (SDA) Alternate Function (AF4) Yap */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* 3. I2C1 Ayarlari (APB1 = 42 MHz icin 100 kHz Standart Mod) */
  I2C1->CR1 |= I2C_CR1_SWRST;
  I2C1->CR1 &= ~I2C_CR1_SWRST;

  I2C1->CR2 = 42;         /* 42 MHz APB1 frekansi */
  I2C1->CCR = 210;        /* 100 kHz: 42MHz / (2 * 100kHz) = 210 */
  I2C1->TRISE = 43;       /* (1000ns / 23.8ns) + 1 = 43 */

  I2C1->CR1 |= I2C_CR1_PE; /* I2C Enable */
}

static void LCD_Write_I2C(uint8_t data)
{
  uint32_t timeout;

  /* START Condition */
  I2C1->CR1 |= I2C_CR1_START;
  timeout = 10000;
  while (!(I2C1->SR1 & I2C_SR1_SB) && --timeout);
  if (timeout == 0) return;

  /* Slave Adresi Gonder */
  I2C1->DR = LCD_ADDR;
  timeout = 10000;
  while (!(I2C1->SR1 & I2C_SR1_ADDR) && --timeout);
  if (timeout == 0) { I2C1->CR1 |= I2C_CR1_STOP; return; }
  (void)I2C1->SR2; /* Clear ADDR flag */

  /* Veriyi Gonder */
  timeout = 10000;
  while (!(I2C1->SR1 & I2C_SR1_TXE) && --timeout);
  if (timeout == 0) { I2C1->CR1 |= I2C_CR1_STOP; return; }

  I2C1->DR = data;
  timeout = 10000;
  while (!(I2C1->SR1 & I2C_SR1_BTF) && --timeout);

  /* STOP Condition */
  I2C1->CR1 |= I2C_CR1_STOP;
}

static void LCD_Send_Nibble(uint8_t nibble, uint8_t rs)
{
  uint8_t data = (nibble & 0xF0) | LCD_BACKLIGHT_PIN;
  if (rs) data |= LCD_RS_PIN;

  LCD_Write_I2C(data | LCD_EN_PIN);
  HAL_Delay(1);
  LCD_Write_I2C(data);
  HAL_Delay(1);
}

static void LCD_Send(uint8_t data, uint8_t is_data)
{
  LCD_Send_Nibble(data & 0xF0, is_data);
  LCD_Send_Nibble((data << 4) & 0xF0, is_data);
}

void LCD_SendCommand(uint8_t cmd)
{
  LCD_Send(cmd, 0);
}

void LCD_SendData(uint8_t data)
{
  LCD_Send(data, 1);
}

void LCD_Init(void)
{
  LCD_I2C_Init();
  LCD_Delay_ms(50);

  LCD_Send_Nibble(0x30, 0);
  LCD_Delay_ms(5);
  LCD_Send_Nibble(0x30, 0);
  LCD_Delay_ms(1);
  LCD_Send_Nibble(0x30, 0);
  LCD_Delay_ms(1);
  LCD_Send_Nibble(0x20, 0);
  LCD_Delay_ms(1);

  LCD_SendCommand(0x28); /* 4-bit, 2 Satir, 5x8 font */
  LCD_SendCommand(0x0C); /* Ekran Acik, Imlec Kapali */
  LCD_SendCommand(0x01); /* Ekrani Temizle */
  LCD_Delay_ms(2);
  LCD_SendCommand(0x06); /* Giris Modu (Auto-increment) */
}

void LCD_Clear(void)
{
  LCD_SendCommand(0x01);
  LCD_Delay_ms(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
  uint8_t address = (row == 0) ? (0x80 + col) : (0xC0 + col);
  LCD_SendCommand(address);
}

void LCD_Print(const char *str)
{
  while (*str)
  {
    LCD_SendData((uint8_t)*str);
    str++;
  }
}

void LCD_DisplayRAM(const char *payload)
{
  char line1[17] = {0};
  char line2[17] = {0};

  /* Payload format: "RAM:45.2% 6.8G/CPU:23.1%" (/ separator) */
  char *sep = strchr(payload, '/');
  if (sep != NULL)
  {
    int len1 = sep - payload;
    if (len1 > 16) len1 = 16;
    strncpy(line1, payload, len1);

    /* Trim any newline at the end of line 2 */
    strncpy(line2, sep + 1, 16);
    char *nl = strchr(line2, '\n');
    if (nl) *nl = '\0';
    nl = strchr(line2, '\r');
    if (nl) *nl = '\0';
  }
  else
  {
    strncpy(line1, payload, 16);
    char *nl = strchr(line1, '\n');
    if (nl) *nl = '\0';
  }

  /* 16 Karakteri dolduracak sekilde bosluk ekle (Eski karakterlerin ustune yazilmasi icin) */
  char padded1[17];
  char padded2[17];
  snprintf(padded1, sizeof(padded1), "%-16s", line1);
  snprintf(padded2, sizeof(padded2), "%-16s", line2);

  LCD_SetCursor(0, 0);
  LCD_Print(padded1);

  LCD_SetCursor(1, 0);
  LCD_Print(padded2);
}
