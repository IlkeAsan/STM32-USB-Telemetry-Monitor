/**
  ******************************************************************************
  * @file    keyboard_helper.c
  * @brief   USB HID Keyboard - Python REPL Line-by-Line Approach
  *
  *  Temiz yaklaşım: Python interaktif modunu acar, kodu satir satir yazar.
  *  Kullanici ekranda okunabilir Python kodu gorur, rastgele harfler degil.
  *
  *  TURKISH Q LAYOUT CORRECTED SCANCODE MAP:
  *    US Key (scancode) -> TR-Q: Normal / Shift / AltGr
  *    - (0x2D)  -> *  ?  backslash
  *    = (0x2E)  -> -  _
  *    \ (0x31)  -> ,  ;  `
  *    ' (0x34)  -> i  İ
  *    ` (0x35)  -> "  é
  *    , (0x36)  -> ö  Ö
  *    / (0x38)  -> .  :
  *    i (0x0C)  -> ı  I
  ******************************************************************************
  */

#include "keyboard_helper.h"
#include "usbd_composite.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

static KeyboardReport_t keyboardReport;

void Keyboard_SendKey(uint8_t modifier, uint8_t keycode)
{
  keyboardReport.modifier = modifier;
  keyboardReport.reserved = 0x00;
  keyboardReport.keycode[0] = keycode;
  keyboardReport.keycode[1] = 0;
  keyboardReport.keycode[2] = 0;
  keyboardReport.keycode[3] = 0;
  keyboardReport.keycode[4] = 0;
  keyboardReport.keycode[5] = 0;

  while (USBD_COMPOSITE_HID_SendReport(&hUsbDeviceFS,
         (uint8_t *)&keyboardReport, sizeof(keyboardReport)) == USBD_BUSY)
  {
    HAL_Delay(2);
  }
  HAL_Delay(3);
  Keyboard_ReleaseAll();
}

void Keyboard_ReleaseAll(void)
{
  keyboardReport.modifier = 0;
  keyboardReport.reserved = 0;
  for (int i = 0; i < 6; i++) keyboardReport.keycode[i] = 0;

  while (USBD_COMPOSITE_HID_SendReport(&hUsbDeviceFS,
         (uint8_t *)&keyboardReport, sizeof(keyboardReport)) == USBD_BUSY)
  {
    HAL_Delay(2);
  }
  HAL_Delay(3);
}

void Keyboard_PressWinR(void)
{
  keyboardReport.modifier = KEY_MOD_LGUI;
  keyboardReport.reserved = 0;
  keyboardReport.keycode[0] = KEY_R;
  while (USBD_COMPOSITE_HID_SendReport(&hUsbDeviceFS,
         (uint8_t *)&keyboardReport, sizeof(keyboardReport)) == USBD_BUSY)
  {
    HAL_Delay(2);
  }
  HAL_Delay(50);
  Keyboard_ReleaseAll();
  HAL_Delay(600);
}

/* =========================================================================
   CORRECT Turkish Q Layout - Verified Character Map
   ========================================================================= */
static uint8_t CharToKeycode(char c, uint8_t *mod)
{
  *mod = 0;

  /* Lowercase letters (except i) */
  if (c >= 'a' && c <= 'z' && c != 'i') return KEY_A + (c - 'a');
  if (c == 'i') return 0x34;  /* TR-Q: US apostrophe pos = i */

  /* Uppercase letters (except I) */
  if (c >= 'A' && c <= 'Z' && c != 'I') { *mod = KEY_MOD_LSHIFT; return KEY_A + (c - 'A'); }
  if (c == 'I') { *mod = KEY_MOD_LSHIFT; return 0x0C; } /* TR-Q: Shift+US-i = I */

  /* Digits */
  if (c >= '1' && c <= '9') return KEY_1 + (c - '1');
  if (c == '0') return KEY_0;

  /* Whitespace */
  if (c == ' ') return KEY_SPACE;
  if (c == '\n' || c == '\r') return KEY_ENTER;

  /* === TR-Q Special Characters === */
  if (c == '.') return 0x38;                                 /* US-slash = . */
  if (c == ':') { *mod = KEY_MOD_LSHIFT; return 0x38; }     /* Shift+US-slash = : */
  if (c == '-') return 0x2E;                                 /* US-equal = - */
  if (c == '_') { *mod = KEY_MOD_LSHIFT; return 0x2E; }     /* Shift+US-equal = _ */
  if (c == '"') return 0x35;                                 /* US-grave = " */
  if (c == '\'') { *mod = KEY_MOD_LSHIFT; return KEY_2; }   /* Shift+2 = ' */
  if (c == '(') { *mod = KEY_MOD_LSHIFT; return KEY_8; }    /* Shift+8 = ( */
  if (c == ')') { *mod = KEY_MOD_LSHIFT; return KEY_9; }    /* Shift+9 = ) */
  if (c == '=') { *mod = KEY_MOD_LSHIFT; return KEY_0; }    /* Shift+0 = = */
  if (c == '/') { *mod = KEY_MOD_LSHIFT; return KEY_7; }    /* Shift+7 = / */
  if (c == '+') { *mod = KEY_MOD_LSHIFT; return KEY_4; }    /* Shift+4 = + */
  if (c == ';') { *mod = KEY_MOD_LSHIFT; return 0x31; }     /* Shift+US-bslash = ; */
  if (c == ',') return 0x31;                                 /* US-bslash = , */
  if (c == '*') return 0x2D;                                 /* US-minus = * */
  if (c == '?') { *mod = KEY_MOD_LSHIFT; return 0x2D; }     /* Shift+US-minus = ? */
  if (c == '!') { *mod = KEY_MOD_LSHIFT; return KEY_1; }    /* Shift+1 = ! */
  if (c == '%') { *mod = KEY_MOD_LSHIFT; return KEY_5; }    /* Shift+5 = % */
  if (c == '&') { *mod = KEY_MOD_LSHIFT; return KEY_6; }    /* Shift+6 = & */
  if (c == '[') { *mod = KEY_MOD_RALT; return KEY_8; }      /* AltGr+8 = [ */
  if (c == ']') { *mod = KEY_MOD_RALT; return KEY_9; }      /* AltGr+9 = ] */
  if (c == '{') { *mod = KEY_MOD_RALT; return KEY_7; }      /* AltGr+7 = { */
  if (c == '}') { *mod = KEY_MOD_RALT; return KEY_0; }      /* AltGr+0 = } */
  if (c == '$') { *mod = KEY_MOD_RALT; return KEY_4; }      /* AltGr+4 = $ */
  if (c == '@') { *mod = KEY_MOD_RALT; return KEY_Q; }      /* AltGr+Q = @ */
  if (c == '#') { *mod = KEY_MOD_RALT; return KEY_3; }      /* AltGr+3 = # */
  if (c == '\\') { *mod = KEY_MOD_RALT; return 0x2D; }      /* AltGr+US-minus = \ */
  if (c == '~') { *mod = KEY_MOD_RALT; return 0x30; }       /* AltGr+US-] = ~ */

  return KEY_NONE;
}

void Keyboard_SendString(const char *str)
{
  while (*str)
  {
    uint8_t mod = 0;
    uint8_t code = CharToKeycode(*str, &mod);
    if (code != KEY_NONE)
    {
      Keyboard_SendKey(mod, code);
      HAL_Delay(2);
    }
    str++;
  }
}

/* Helper: Bir satir yaz ve kisa bekle (Python REPL icin) */
static void REPL_Line(const char *line, uint16_t wait_ms)
{
  Keyboard_SendString(line);
  Keyboard_SendKey(0, KEY_ENTER);
  HAL_Delay(wait_ms);
}

void Keyboard_ExecutePowerShell(const char *cmd)
{
  Keyboard_PressWinR();
  HAL_Delay(600);
  Keyboard_SendString("powershell\n");
  HAL_Delay(1500);
  Keyboard_SendString(cmd);
  Keyboard_SendKey(0, KEY_ENTER);
}

/**
  * @brief  Self-deploying RAM Monitor via Python REPL (TEMIZ YONTEM)
  *
  *  Kullanici ekranda okunabilir Python kodu gorur:
  *
  *  PS> pip install psutil pyserial
  *  PS> python
  *  >>> import time
  *  >>> import psutil
  *  >>> import serial
  *  >>> ...
  *  >>> while 1:
  *  ...  m=psutil.virtual_memory()
  *  ...  (devam)
  *
  *  Hicbir Base64 veya rastgele karakter YOK!
  */
void Keyboard_LaunchNativeRAMMonitor(void)
{
  /* ===== ADIM 1: PowerShell Ac ===== */
  Keyboard_PressWinR();
  HAL_Delay(600);
  Keyboard_SendString("powershell");
  Keyboard_SendKey(0, KEY_ENTER);
  HAL_Delay(1500);

  /* ===== ADIM 2: Kutuphaneleri Yukle ===== */
  /* pip install işleminin bitmesi internet hızına bağlı olarak uzun sürebilir.
     Eğer STM32 beklemeden diğer tuşları yollarsa (Alt+Tab gibi) komutlar yanlış pencereye gider.
     Bu yüzden yükleme için 12 saniye (12000 ms) pay bırakıyoruz. */
  REPL_Line("pip install psutil pyserial", 12000);

  /* YouTube Videosunu Ac */
  REPL_Line("Start-Process chrome \"https://www.youtube.com/watch?v=NTa6Xbzfq1U\"", 4000);

  /* Chrome açıldıktan sonra Alt + Tab ile PowerShell'e Geri Don */
  Keyboard_SendKey(KEY_MOD_LALT, KEY_TAB);
  HAL_Delay(1000);

  /* ===== ADIM 3: Python Interaktif Modu Ac ===== */
  REPL_Line("python", 3000);

  /* ===== ADIM 4: Import Satirlari ===== */
  REPL_Line("import time", 500);
  REPL_Line("import psutil", 500);
  REPL_Line("import serial", 500);
  REPL_Line("import serial.tools.list_ports as lp", 500);


  /* print("STM32 RAM MONITOR") */
  REPL_Line("print(\"STM32 RAM MONITOR\")", 300);

  /* ===== ADIM 6: Evrensel Olarak STM32 Portunu Bul ve Baglan ===== */
  REPL_Line("p=[x.device for x in lp.comports() if '5740' in x.hwid or ('stlink' not in (x.description or '').lower() and '0483' in x.hwid)]", 300);
  REPL_Line("s=serial.Serial(p[0] if p else lp.comports()[-1].device, 115200)", 600);

  /* ===== ADIM 5: Yorum Satırı Olarak Logo ve İmza ===== */
  REPL_Line("# =====================================", 10);
  REPL_Line("#    .-------------------.             ", 10);
  REPL_Line("#   /    (O)       (O)    /            ", 10);
  REPL_Line("#  |                       |           ", 10);
  REPL_Line("#  |       (_______)       |           ", 10);
  REPL_Line("#   *                     *            ", 10);
  REPL_Line("#    '-------------------'             ", 10);
  REPL_Line("# =====================================", 10);
  REPL_Line("#      Designed by Ilke                ", 10);


  /* print("Baglandi: "+s.port) */
  REPL_Line("print(\"Baglandi: \"+s.port)", 300);

  /* ===== ADIM 7: Sonsuz Dongu (while blogu) ===== */
  /* while 1:  -> Enter (Python REPL ... promptuna gecer) */
  REPL_Line("while 1:", 500);

  /* Her satirin basina 1 bosluk (Python indent) */
  REPL_Line(" m=psutil.virtual_memory()", 200);
  REPL_Line(" u=round((m.total-m.available)/1073741824,1)", 200);
  REPL_Line(" t=round(m.total/1073741824,1)", 200);
  REPL_Line(" r=round(u/t*100,1)", 200);
  REPL_Line(" c=round(psutil.cpu_percent(),1)", 200);

  /* Seri porta veri gonder (x= atamasi sayesinde ekrana 25/26 sayilari basmaz, tertemiz calisir): */
  REPL_Line(" x=s.write((\"RAM:\"+str(r)+\"% \"+str(u)+\"G/CPU:\"+str(c)+\"%\"+chr(10)).encode())", 200);

  REPL_Line(" time.sleep(1)", 200);

  /* Bos satir -> while blogunu calistir! */
  Keyboard_SendKey(0, KEY_ENTER);
}
