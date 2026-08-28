/**
  ******************************************************************************
  * @file    usbd_composite.h
  * @brief   Header file for USB Composite Device (CDC + HID Keyboard)
  ******************************************************************************
  */

#ifndef __USBD_COMPOSITE_H
#define __USBD_COMPOSITE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_ioreq.h"
#include "usbd_cdc.h"

/* Endpoints definitions */
#define CDC_IN_EP                                   0x81U  /* EP1 for CDC data IN */
#define CDC_OUT_EP                                  0x01U  /* EP1 for CDC data OUT */
#define CDC_CMD_EP                                  0x82U  /* EP2 for CDC commands */
#define HID_IN_EP                                   0x83U  /* EP3 for HID Keyboard IN */

#define CDC_DATA_FS_MAX_PACKET_SIZE                 64U
#define CDC_CMD_PACKET_SIZE                         8U
#define HID_KEYBOARD_IN_PACKET_SIZE                 8U

#define USB_COMPOSITE_CONFIG_DESC_SIZ               100U
#define HID_KEYBOARD_REPORT_DESC_SIZE               63U

#define HID_DESCRIPTOR_TYPE                         0x21U
#define HID_REPORT_DESC                             0x22U

#define HID_REQ_SET_PROTOCOL                        0x0BU
#define HID_REQ_GET_PROTOCOL                        0x03U
#define HID_REQ_SET_IDLE                            0x0AU
#define HID_REQ_GET_IDLE                            0x02U

typedef struct
{
  uint32_t data[CDC_DATA_FS_MAX_PACKET_SIZE / 4U];
  uint8_t  CmdOpCode;
  uint8_t  CmdLength;
  uint8_t  *RxBuffer;
  uint8_t  *TxBuffer;
  uint32_t RxLength;
  uint32_t TxLength;
  __IO uint32_t TxState;
  __IO uint32_t RxState;

  /* HID specific */
  uint32_t Protocol;
  uint32_t IdleState;
  uint32_t AltSetting;
  __IO uint32_t HidState;
} USBD_Composite_HandleTypeDef;

extern USBD_ClassTypeDef USBD_COMPOSITE;

uint8_t USBD_COMPOSITE_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_CDC_ItfTypeDef *fops);
uint8_t USBD_COMPOSITE_SetTxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuff, uint32_t length);
uint8_t USBD_COMPOSITE_SetRxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuff);
uint8_t USBD_COMPOSITE_ReceivePacket(USBD_HandleTypeDef *pdev);
uint8_t USBD_COMPOSITE_TransmitPacket(USBD_HandleTypeDef *pdev);
uint8_t USBD_COMPOSITE_HID_SendReport(USBD_HandleTypeDef *pdev, uint8_t *report, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __USBD_COMPOSITE_H */
