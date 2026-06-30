# Honguera Leaf Node Modifications Guide

## Lite (`honguera-lite/`)
**Automated:** DAC, RS485, SDI-12, step-down regulator, extra MOSFET, extra relay, extra TVS removed.

**Manual KiCad needed:**
1. Add **MH-Z19B CO₂** → 4-pin header on UART2 (GPIO18 RX, GPIO19 TX)
2. Add **SSR** for humidifier → one free GPIO
3. Replace step-down → **AMS1117-3.3** (SOT-223)
4. Compact PCB ~30-40%
5. Remove redundant text labels & test points

## Pro (`honguera-pro/`)
**Automated:** Title/project metadata updated. All components kept.

**Manual KiCad needed:**
1. Add **MH-Z19B CO₂** → 4-pin header UART2
2. Add **SSR footprint** → AC humidifier control
3. Add **MPXV7002DP** → differential pressure (analog ADC)
4. Add **DS3231 RTC** → I2C bus
5. Add **5-pin expansion header** (+5V, GND, GPIO, SDA, SCL)

## Firmware targets (PlatformIO)
```ini
[env:honguera-lite]
board = esp32-s3-devkitc-1
build_flags = -DHONGUERA_LITE

[env:honguera-pro]
board = esp32-s3-devkitc-1
build_flags = -DHONGUERA_PRO
```
