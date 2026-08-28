/**
  ******************************************************************************
  * @file    keyboard_helper.h
  * @brief   Helper functions to simulate USB HID Keyboard keystrokes
  ******************************************************************************
  */

#ifndef __KEYBOARD_HELPER_H
#define __KEYBOARD_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "usb_device.h"

/* USB HID Keyboard Modifier Bits */
#define KEY_MOD_LCTRL     0x01
#define KEY_MOD_LSHIFT    0x02
#define KEY_MOD_LALT      0x04
#define KEY_MOD_LGUI      0x08  /* Windows Key */
#define KEY_MOD_RCTRL     0x10
#define KEY_MOD_RSHIFT    0x20
#define KEY_MOD_RALT      0x40  /* AltGr */
#define KEY_MOD_RGUI      0x80

/* USB HID Keycodes */
#define KEY_NONE          0x00
#define KEY_ENTER         0x28
#define KEY_ESCAPE        0x29
#define KEY_BACKSPACE     0x2A
#define KEY_TAB           0x2B
#define KEY_SPACE         0x2C

#define KEY_A             0x04
#define KEY_B             0x05
#define KEY_C             0x06
#define KEY_D             0x07
#define KEY_E             0x08
#define KEY_F             0x09
#define KEY_G             0x0A
#define KEY_H             0x0B
#define KEY_I             0x0C
#define KEY_J             0x0D
#define KEY_K             0x0E
#define KEY_L             0x0F
#define KEY_M             0x10
#define KEY_N             0x11
#define KEY_O             0x12
#define KEY_P             0x13
#define KEY_Q             0x14
#define KEY_R             0x15
#define KEY_S             0x16
#define KEY_T             0x17
#define KEY_U             0x18
#define KEY_V             0x19
#define KEY_W             0x1A
#define KEY_X             0x1B
#define KEY_Y             0x1C
#define KEY_Z             0x1D

#define KEY_1             0x1E
#define KEY_2             0x1F
#define KEY_3             0x20
#define KEY_4             0x21
#define KEY_5             0x22
#define KEY_6             0x23
#define KEY_7             0x24
#define KEY_8             0x25
#define KEY_9             0x26
#define KEY_0             0x27

#define KEY_MINUS         0x2D
#define KEY_EQUAL         0x2E

typedef struct
{
  uint8_t modifier;
  uint8_t reserved;
  uint8_t keycode[6];
} __attribute__((packed)) KeyboardReport_t;

void Keyboard_SendKey(uint8_t modifier, uint8_t keycode);
void Keyboard_ReleaseAll(void);
void Keyboard_PressWinR(void);
void Keyboard_SendString(const char *str);
void Keyboard_ExecutePowerShell(const char *cmd);
void Keyboard_LaunchNativeRAMMonitor(void);

#ifdef __cplusplus
}
#endif

#endif /* __KEYBOARD_HELPER_H */
