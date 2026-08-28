"""
================================================================================
  STM32 RAM & CPU Monitor + Giant ASCII Smiley Face Banner
  DESIGNED BY ILKE
================================================================================
"""

import sys
import time
import os

if hasattr(sys.stdout, 'reconfigure'):
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except Exception:
        pass

BANNER = r"""
  ======================================================================
    ____ _____ __  __ _____ ____    ____      _    __  __ 
   / ___|_   _|  \/  |___ /|___ \  |  _ \    / \  |  \/  |
   \___ \ | | | |\/| | |_ \  |_ \  | |_) |  / _ \ | |\/| |
    ___) || | | |  | |___) |___) | |  _ <  / ___ \| |  | |
   |____/ |_| |_|  |_|____/|____/  |_| \_\/_/   \_\_|  |_|
  ======================================================================

                         .-----------------------.
                        /                         \
                       |     (O)           (O)     |
                       |                           |
                       |             /\            |
                       |            /  \           |
                       |                           |
                       \      \_____________/      /
                        \                         /
                         '.                     .'
                           '-------------------'

              =============================================
                     >>> SISTEM BASARIYLA BAGLANDI <<<
                         DESIGNED BY ILKE
              =============================================
"""

print(BANNER)

try:
    import psutil
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[!] Kütüphaneler eksik, yükleniyor: psutil, pyserial...")
    os.system("pip install psutil pyserial")
    import psutil
    import serial
    import serial.tools.list_ports

def find_stm32_port():
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        desc = (p.description or "").lower()
        mfg = (p.manufacturer or "").lower()
        if "stm" in desc or "stmicroelectronics" in mfg or "vcp" in desc or "0483" in str(p.hwid):
            return p.device
    return ports[-1].device if ports else None

def main():
    ser = None
    while ser is None:
        port = find_stm32_port()
        if port:
            try:
                ser = serial.Serial(port, baudrate=115200, timeout=1)
                print(f"[+] STM32 Portuna Baglandi: {port}")
                break
            except Exception as e:
                pass
        time.sleep(1)

    print("\n[+] Canli RAM & CPU Verisi STM32 Ekranina Aktariliyor...\n")
    psutil.cpu_percent(interval=None)

    try:
        while True:
            mem = psutil.virtual_memory()
            total_gb = mem.total / (1024 ** 3)
            used_gb = (mem.total - mem.available) / (1024 ** 3)
            ram_pct = (used_gb / total_gb) * 100.0
            cpu_pct = psutil.cpu_percent(interval=None)

            payload = f"RAM:{ram_pct:5.1f}% {used_gb:4.1f}G|CPU:{cpu_pct:4.1f}%\n"
            print(f"-> RAM: {ram_pct:5.1f}% ({used_gb:4.1f}/{total_gb:4.1f} GB) | CPU: {cpu_pct:4.1f}%")

            try:
                ser.write(payload.encode('utf-8'))
            except Exception:
                ser.close()
                ser = None
                while ser is None:
                    port = find_stm32_port()
                    if port:
                        try:
                            ser = serial.Serial(port, baudrate=115200, timeout=1)
                        except:
                            pass
                    time.sleep(1)

            time.sleep(1.0)

    except KeyboardInterrupt:
        print("\nKapatildi.")
        if ser and ser.is_open:
            ser.close()

if __name__ == "__main__":
    main()
