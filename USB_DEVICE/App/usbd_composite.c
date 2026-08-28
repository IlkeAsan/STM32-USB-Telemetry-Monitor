/**
  ******************************************************************************
  * @file    usbd_composite.c
  * @brief   USB Composite Device (CDC + HID Keyboard) implementation
  ******************************************************************************
  */

#include "usbd_composite.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"

static uint8_t USBD_COMPOSITE_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_COMPOSITE_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_COMPOSITE_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_COMPOSITE_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_COMPOSITE_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_COMPOSITE_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t *USBD_COMPOSITE_GetFSCfgDesc(uint16_t *length);
static uint8_t *USBD_COMPOSITE_GetDeviceQualifierDescriptor(uint16_t *length);

/* USB Composite Class structure */
USBD_ClassTypeDef USBD_COMPOSITE =
{
  USBD_COMPOSITE_Init,
  USBD_COMPOSITE_DeInit,
  USBD_COMPOSITE_Setup,
  NULL, /* EP0_TxSent */
  USBD_COMPOSITE_EP0_RxReady,
  USBD_COMPOSITE_DataIn,
  USBD_COMPOSITE_DataOut,
  NULL, /* SOF */
  NULL, /* IsoINIncomplete */
  NULL, /* IsoOUTIncomplete */
  USBD_COMPOSITE_GetFSCfgDesc,
  USBD_COMPOSITE_GetFSCfgDesc,
  USBD_COMPOSITE_GetFSCfgDesc,
  USBD_COMPOSITE_GetDeviceQualifierDescriptor,
};

/* HID Keyboard Report Descriptor (Standard Boot Keyboard 63 bytes) */
__ALIGN_BEGIN static uint8_t HID_KEYBOARD_ReportDesc[HID_KEYBOARD_REPORT_DESC_SIZE] __ALIGN_END =
{
  0x05, 0x01,                    /* USAGE_PAGE (Generic Desktop) */
  0x09, 0x06,                    /* USAGE (Keyboard) */
  0xa1, 0x01,                    /* COLLECTION (Application) */
  0x05, 0x07,                    /*   USAGE_PAGE (Keyboard) */
  0x19, 0xe0,                    /*   USAGE_MINIMUM (Keyboard LeftControl) */
  0x29, 0xe7,                    /*   USAGE_MAXIMUM (Keyboard Right GUI) */
  0x15, 0x00,                    /*   LOGICAL_MINIMUM (0) */
  0x25, 0x01,                    /*   LOGICAL_MAXIMUM (1) */
  0x75, 0x01,                    /*   REPORT_SIZE (1) */
  0x95, 0x08,                    /*   REPORT_COUNT (8) */
  0x81, 0x02,                    /*   INPUT (Data,Var,Abs) */
  0x95, 0x01,                    /*   REPORT_COUNT (1) */
  0x75, 0x08,                    /*   REPORT_SIZE (8) */
  0x81, 0x03,                    /*   INPUT (Cnst,Var,Abs) */
  0x95, 0x05,                    /*   REPORT_COUNT (5) */
  0x75, 0x01,                    /*   REPORT_SIZE (1) */
  0x05, 0x08,                    /*   USAGE_PAGE (LEDs) */
  0x19, 0x01,                    /*   USAGE_MINIMUM (Num Lock) */
  0x29, 0x05,                    /*   USAGE_MAXIMUM (Kana) */
  0x91, 0x02,                    /*   OUTPUT (Data,Var,Abs) */
  0x95, 0x01,                    /*   REPORT_COUNT (1) */
  0x75, 0x03,                    /*   REPORT_SIZE (3) */
  0x91, 0x03,                    /*   OUTPUT (Cnst,Var,Abs) */
  0x95, 0x06,                    /*   REPORT_COUNT (6) */
  0x75, 0x08,                    /*   REPORT_SIZE (8) */
  0x15, 0x00,                    /*   LOGICAL_MINIMUM (0) */
  0x25, 0x65,                    /*   LOGICAL_MAXIMUM (101) */
  0x05, 0x07,                    /*   USAGE_PAGE (Keyboard) */
  0x19, 0x00,                    /*   USAGE_MINIMUM (Reserved) */
  0x29, 0x65,                    /*   USAGE_MAXIMUM (Keyboard Application) */
  0x81, 0x00,                    /*   INPUT (Data,Ary,Abs) */
  0xc0                           /* END_COLLECTION */
};

/* USB Composite Configuration Descriptor (CDC + HID Keyboard) */
__ALIGN_BEGIN static uint8_t USBD_COMPOSITE_CfgFSDesc[USB_COMPOSITE_CONFIG_DESC_SIZ] __ALIGN_END =
{
  /* Configuration Descriptor */
  0x09,                                       /* bLength: Configuration Descriptor size */
  USB_DESC_TYPE_CONFIGURATION,                /* bDescriptorType: Configuration */
  LOBYTE(USB_COMPOSITE_CONFIG_DESC_SIZ),      /* wTotalLength: 100 Bytes */
  HIBYTE(USB_COMPOSITE_CONFIG_DESC_SIZ),
  0x03,                                       /* bNumInterfaces: 3 interfaces (2 CDC, 1 HID) */
  0x01,                                       /* bConfigurationValue: Configuration value */
  0x00,                                       /* iConfiguration: Index of string descriptor */
  0xC0,                                       /* bmAttributes: Self powered */
  0x32,                                       /* MaxPower 100 mA */

  /******************** Interface Association Descriptor (IAD) for CDC ********************/
  0x08,                                       /* bLength */
  0x0B,                                       /* bDescriptorType: IAD */
  0x00,                                       /* bFirstInterface: Interface 0 */
  0x02,                                       /* bInterfaceCount: 2 interfaces */
  0x02,                                       /* bFunctionClass: CDC */
  0x02,                                       /* bFunctionSubClass: Abstract Control Model */
  0x01,                                       /* bFunctionProtocol: Common AT commands */
  0x00,                                       /* iFunction */

  /******************** Interface 0: CDC Command Interface ********************/
  0x09,                                       /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                    /* bDescriptorType: Interface */
  0x00,                                       /* bInterfaceNumber: Number of Interface (0) */
  0x00,                                       /* bAlternateSetting: Alternate setting */
  0x01,                                       /* bNumEndpoints: One endpoints used */
  0x02,                                       /* bInterfaceClass: Communication Interface Class */
  0x02,                                       /* bInterfaceSubClass: Abstract Control Model */
  0x01,                                       /* bInterfaceProtocol: Common AT commands */
  0x00,                                       /* iInterface: */

  /* Header Functional Descriptor */
  0x05,                                       /* bLength: Endpoint Descriptor size */
  0x24,                                       /* bDescriptorType: CS_INTERFACE */
  0x00,                                       /* bDescriptorSubtype: Header Func Desc */
  0x10,                                       /* bcdCDC: spec release number 1.10 */
  0x01,

  /* Call Management Functional Descriptor */
  0x05,                                       /* bFunctionLength */
  0x24,                                       /* bDescriptorType: CS_INTERFACE */
  0x01,                                       /* bDescriptorSubtype: Call Management Func Desc */
  0x00,                                       /* bmCapabilities: D0+D1 */
  0x01,                                       /* bDataInterface: 1 */

  /* ACM Functional Descriptor */
  0x04,                                       /* bFunctionLength */
  0x24,                                       /* bDescriptorType: CS_INTERFACE */
  0x02,                                       /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,                                       /* bmCapabilities */

  /* Union Functional Descriptor */
  0x05,                                       /* bFunctionLength */
  0x24,                                       /* bDescriptorType: CS_INTERFACE */
  0x06,                                       /* bDescriptorSubtype: Union func desc */
  0x00,                                       /* bMasterInterface: Communication class interface (0) */
  0x01,                                       /* bSlaveInterface0: Data Class Interface (1) */

  /* CDC Command Endpoint Descriptor (EP2 IN) */
  0x07,                                       /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
  CDC_CMD_EP,                                 /* bEndpointAddress: EP2 IN */
  0x03,                                       /* bmAttributes: Interrupt */
  LOBYTE(CDC_CMD_PACKET_SIZE),                /* wMaxPacketSize */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  CDC_FS_BINTERVAL,                           /* bInterval */

  /******************** Interface 1: CDC Data Interface ********************/
  0x09,                                       /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                    /* bDescriptorType: Interface */
  0x01,                                       /* bInterfaceNumber: Number of Interface (1) */
  0x00,                                       /* bAlternateSetting: Alternate setting */
  0x02,                                       /* bNumEndpoints: Two endpoints used */
  0x0A,                                       /* bInterfaceClass: CDC Data */
  0x00,                                       /* bInterfaceSubClass: */
  0x00,                                       /* bInterfaceProtocol: */
  0x00,                                       /* iInterface: */

  /* CDC Data OUT Endpoint Descriptor (EP1 OUT) */
  0x07,                                       /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
  CDC_OUT_EP,                                 /* bEndpointAddress: EP1 OUT */
  0x02,                                       /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),        /* wMaxPacketSize: 64 */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                                       /* bInterval: ignore for Bulk */

  /* CDC Data IN Endpoint Descriptor (EP1 IN) */
  0x07,                                       /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
  CDC_IN_EP,                                  /* bEndpointAddress: EP1 IN */
  0x02,                                       /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),        /* wMaxPacketSize: 64 */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                                       /* bInterval: ignore for Bulk */

  /******************** Interface 2: HID Keyboard Interface ********************/
  0x09,                                       /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                    /* bDescriptorType: Interface */
  0x02,                                       /* bInterfaceNumber: Number of Interface (2) */
  0x00,                                       /* bAlternateSetting: Alternate setting */
  0x01,                                       /* bNumEndpoints: 1 endpoint */
  0x03,                                       /* bInterfaceClass: HID */
  0x01,                                       /* bInterfaceSubClass: 1=Boot, 0=No Boot */
  0x01,                                       /* bInterfaceProtocol: 1=Keyboard, 2=Mouse */
  0x00,                                       /* iInterface: */

  /* HID Descriptor */
  0x09,                                       /* bLength: HID Descriptor size */
  HID_DESCRIPTOR_TYPE,                        /* bDescriptorType: HID */
  0x11,                                       /* bcdHID: HID Class Spec release number (1.11) */
  0x01,
  0x00,                                       /* bCountryCode: Hardware target country */
  0x01,                                       /* bNumDescriptors: Number of HID class descriptors to follow */
  0x22,                                       /* bDescriptorType: Report */
  LOBYTE(HID_KEYBOARD_REPORT_DESC_SIZE),      /* wItemLength: 63 */
  HIBYTE(HID_KEYBOARD_REPORT_DESC_SIZE),

  /* HID Keyboard Endpoint Descriptor (EP3 IN) */
  0x07,                                       /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
  HID_IN_EP,                                  /* bEndpointAddress: EP3 IN */
  0x03,                                       /* bmAttributes: Interrupt */
  LOBYTE(HID_KEYBOARD_IN_PACKET_SIZE),        /* wMaxPacketSize: 8 bytes */
  HIBYTE(HID_KEYBOARD_IN_PACKET_SIZE),
  0x0A                                        /* bInterval: Polling Interval (10 ms) */
};

/* Device Qualifier Descriptor */
__ALIGN_BEGIN static uint8_t USBD_COMPOSITE_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0xEF,
  0x02,
  0x01,
  0x40,
  0x01,
  0x00,
};

static uint8_t USBD_COMPOSITE_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_Composite_HandleTypeDef *hcomp;

  hcomp = (USBD_Composite_HandleTypeDef *)USBD_malloc(sizeof(USBD_Composite_HandleTypeDef));
  if (hcomp == NULL)
  {
    pdev->pClassData = NULL;
    return (uint8_t)USBD_EMEM;
  }

  pdev->pClassData = (void *)hcomp;
  USBD_memset(hcomp, 0, sizeof(USBD_Composite_HandleTypeDef));

  /* Open CDC Endpoints */
  (void)USBD_LL_OpenEP(pdev, CDC_IN_EP, USBD_EP_TYPE_BULK, CDC_DATA_FS_IN_PACKET_SIZE);
  pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 1U;

  (void)USBD_LL_OpenEP(pdev, CDC_OUT_EP, USBD_EP_TYPE_BULK, CDC_DATA_FS_OUT_PACKET_SIZE);
  pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 1U;

  (void)USBD_LL_OpenEP(pdev, CDC_CMD_EP, USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE);
  pdev->ep_in[CDC_CMD_EP & 0xFU].is_used = 1U;

  /* Open HID Endpoint */
  (void)USBD_LL_OpenEP(pdev, HID_IN_EP, USBD_EP_TYPE_INTR, HID_KEYBOARD_IN_PACKET_SIZE);
  pdev->ep_in[HID_IN_EP & 0xFU].is_used = 1U;

  hcomp->TxState = 0U;
  hcomp->RxState = 0U;
  hcomp->HidState = 0U;

  /* Initialize CDC interface */
  if (pdev->pUserData[0] != NULL)
  {
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData[0])->Init();
  }

  /* Prepare Out endpoint to receive CDC data */
  (void)USBD_LL_PrepareReceive(pdev, CDC_OUT_EP, hcomp->RxBuffer, CDC_DATA_FS_OUT_PACKET_SIZE);

  return (uint8_t)USBD_OK;
}

static uint8_t USBD_COMPOSITE_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  /* Close CDC Endpoints */
  (void)USBD_LL_CloseEP(pdev, CDC_IN_EP);
  pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 0U;

  (void)USBD_LL_CloseEP(pdev, CDC_OUT_EP);
  pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 0U;

  (void)USBD_LL_CloseEP(pdev, CDC_CMD_EP);
  pdev->ep_in[CDC_CMD_EP & 0xFU].is_used = 0U;

  /* Close HID Endpoint */
  (void)USBD_LL_CloseEP(pdev, HID_IN_EP);
  pdev->ep_in[HID_IN_EP & 0xFU].is_used = 0U;

  if (pdev->pClassData != NULL)
  {
    if (pdev->pUserData[0] != NULL)
    {
      ((USBD_CDC_ItfTypeDef *)pdev->pUserData[0])->DeInit();
    }
    (void)USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  }

  return (uint8_t)USBD_OK;
}

static uint8_t USBD_COMPOSITE_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;
  USBD_CDC_ItfTypeDef *cdc_fops = (USBD_CDC_ItfTypeDef *)pdev->pUserData[0];
  uint8_t ifalt = 0U;
  uint16_t status_info = 0U;
  USBD_StatusTypeDef ret = USBD_OK;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    case USB_REQ_TYPE_CLASS:
      if (req->wLength != 0U)
      {
        if ((req->bmRequest & 0x80U) != 0U)
        {
          /* Class GET Request */
          if (LOBYTE(req->wIndex) == 0x02) /* HID Interface */
          {
            switch (req->bRequest)
            {
              case HID_REQ_GET_PROTOCOL:
                (void)USBD_CtlSendData(pdev, (uint8_t *)&hcomp->Protocol, 1U);
                break;
              case HID_REQ_GET_IDLE:
                (void)USBD_CtlSendData(pdev, (uint8_t *)&hcomp->IdleState, 1U);
                break;
              default:
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
                break;
            }
          }
          else /* CDC Interface */
          {
            if (cdc_fops != NULL)
            {
              cdc_fops->Control(req->bRequest, (uint8_t *)hcomp->data, req->wLength);
            }
            (void)USBD_CtlSendData(pdev, (uint8_t *)hcomp->data, req->wLength);
          }
        }
        else
        {
          /* Class SET Request */
          if (LOBYTE(req->wIndex) == 0x02) /* HID Interface */
          {
            switch (req->bRequest)
            {
              case HID_REQ_SET_PROTOCOL:
                hcomp->Protocol = (uint8_t)(req->wValue);
                break;
              case HID_REQ_SET_IDLE:
                hcomp->IdleState = (uint8_t)(req->wValue >> 8);
                break;
              default:
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
                break;
            }
          }
          else /* CDC Interface */
          {
            hcomp->CmdOpCode = req->bRequest;
            hcomp->CmdLength = (uint8_t)req->wLength;
            (void)USBD_CtlPrepareRx(pdev, (uint8_t *)hcomp->data, req->wLength);
          }
        }
      }
      else
      {
        /* No data phase class request */
        if (LOBYTE(req->wIndex) == 0x02)
        {
          if (req->bRequest == HID_REQ_SET_IDLE)
          {
            hcomp->IdleState = (uint8_t)(req->wValue >> 8);
          }
        }
        else
        {
          if (cdc_fops != NULL)
          {
            cdc_fops->Control(req->bRequest, (uint8_t *)req, 0U);
          }
        }
      }
      break;

    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest)
      {
        case USB_REQ_GET_STATUS:
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            (void)USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
          }
          else
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_GET_INTERFACE:
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            (void)USBD_CtlSendData(pdev, &ifalt, 1U);
          }
          else
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_SET_INTERFACE:
          if (pdev->dev_state != USBD_STATE_CONFIGURED)
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_GET_DESCRIPTOR:
          if (req->wValue >> 8 == HID_REPORT_DESC)
          {
            uint16_t len = MIN(HID_KEYBOARD_REPORT_DESC_SIZE, req->wLength);
            (void)USBD_CtlSendData(pdev, HID_KEYBOARD_ReportDesc, len);
          }
          else if (req->wValue >> 8 == HID_DESCRIPTOR_TYPE)
          {
            uint16_t len = MIN(0x09, req->wLength);
            (void)USBD_CtlSendData(pdev, &USBD_COMPOSITE_CfgFSDesc[100 - 16], len);
          }
          else
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_CLEAR_FEATURE:
          break;

        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
          break;
      }
      break;

    default:
      USBD_CtlError(pdev, req);
      ret = USBD_FAIL;
      break;
  }

  return (uint8_t)ret;
}

static uint8_t USBD_COMPOSITE_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;

  if (pdev->pClassData == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  if (epnum == (CDC_IN_EP & 0x7FU))
  {
    hcomp->TxState = 0U;
    if (pdev->pUserData[0] != NULL)
    {
      ((USBD_CDC_ItfTypeDef *)pdev->pUserData[0])->TransmitCplt(hcomp->TxBuffer, &hcomp->TxLength, epnum);
    }
  }
  else if (epnum == (HID_IN_EP & 0x7FU))
  {
    hcomp->HidState = 0U; /* Ready for next HID report */
  }

  return (uint8_t)USBD_OK;
}

static uint8_t USBD_COMPOSITE_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;

  if (pdev->pClassData == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  if (epnum == CDC_OUT_EP)
  {
    hcomp->RxLength = USBD_LL_GetRxDataSize(pdev, epnum);
    if (pdev->pUserData[0] != NULL)
    {
      ((USBD_CDC_ItfTypeDef *)pdev->pUserData[0])->Receive(hcomp->RxBuffer, &hcomp->RxLength);
    }
  }

  return (uint8_t)USBD_OK;
}

static uint8_t USBD_COMPOSITE_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;

  if ((pdev->pUserData[0] != NULL) && (hcomp->CmdOpCode != 0xFFU))
  {
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData[0])->Control(hcomp->CmdOpCode, (uint8_t *)hcomp->data, (uint16_t)hcomp->CmdLength);
    hcomp->CmdOpCode = 0xFFU;
  }

  return (uint8_t)USBD_OK;
}

static uint8_t *USBD_COMPOSITE_GetFSCfgDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_COMPOSITE_CfgFSDesc);
  return USBD_COMPOSITE_CfgFSDesc;
}

static uint8_t *USBD_COMPOSITE_GetDeviceQualifierDescriptor(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_COMPOSITE_DeviceQualifierDesc);
  return USBD_COMPOSITE_DeviceQualifierDesc;
}

uint8_t USBD_COMPOSITE_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_CDC_ItfTypeDef *fops)
{
  if (fops == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  pdev->pUserData[0] = fops;
  return (uint8_t)USBD_OK;
}

uint8_t USBD_COMPOSITE_SetTxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuff, uint32_t length)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;

  if (hcomp == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  hcomp->TxBuffer = pbuff;
  hcomp->TxLength = length;

  return (uint8_t)USBD_OK;
}

uint8_t USBD_COMPOSITE_SetRxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuff)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;

  if (hcomp == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  hcomp->RxBuffer = pbuff;

  return (uint8_t)USBD_OK;
}

uint8_t USBD_COMPOSITE_TransmitPacket(USBD_HandleTypeDef *pdev)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;

  if (pdev->pClassData == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  if (hcomp->TxState == 0U)
  {
    hcomp->TxState = 1U;
    pdev->ep_in[CDC_IN_EP & 0xFU].total_length = hcomp->TxLength;
    (void)USBD_LL_Transmit(pdev, CDC_IN_EP, hcomp->TxBuffer, hcomp->TxLength);
    return (uint8_t)USBD_OK;
  }
  else
  {
    return (uint8_t)USBD_BUSY;
  }
}

uint8_t USBD_COMPOSITE_ReceivePacket(USBD_HandleTypeDef *pdev)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;

  if (pdev->pClassData == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  (void)USBD_LL_PrepareReceive(pdev, CDC_OUT_EP, hcomp->RxBuffer, CDC_DATA_FS_OUT_PACKET_SIZE);

  return (uint8_t)USBD_OK;
}

uint8_t USBD_COMPOSITE_HID_SendReport(USBD_HandleTypeDef *pdev, uint8_t *report, uint16_t len)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;

  if (pdev->pClassData == NULL || pdev->dev_state != USBD_STATE_CONFIGURED)
  {
    return (uint8_t)USBD_FAIL;
  }

  if (hcomp->HidState == 0U)
  {
    hcomp->HidState = 1U;
    (void)USBD_LL_Transmit(pdev, HID_IN_EP, report, len);
    return (uint8_t)USBD_OK;
  }
  else
  {
    return (uint8_t)USBD_BUSY;
  }
}
