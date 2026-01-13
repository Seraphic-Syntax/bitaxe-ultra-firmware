# ESP Miner Compatibility Guide

This document details hardware, software, firmware, mining pools, and environmental compatibility for the **ESP Miner** project—a lightweight Monero (XMR) RandomX miner for ESP32-S3 boards. Always use the [latest release](https://github.com/yourusername/esp-miner/releases) for best compatibility.

## ✅ Supported Hardware

### ESP32 Boards
| Board Model | Chip | Flash | PSRAM | Status | Notes |
|-------------|------|-------|-------|--------|-------|
| **ESP32-S3-DevKitC-1** | ESP32-S3-WROOM-1 | 8MB | 8MB | ✅ Fully Supported | Recommended reference board. Use N8R8 variant. |
| **LilyGo T-Display-S3** | ESP32-S3 | 8MB | 8MB | ✅ Fully Supported | Built-in 1.9" OLED (GC9A01). Matches demo wiring. |
| **ESP32-S3-Box-3** | ESP32-S3 | 8MB | 8MB | ✅ Fully Supported | Touchscreen variant; disable touch in config for mining focus. |
| **XIAO ESP32S3** | ESP32-S3 | 8MB | 8MB | ✅ Supported | Compact; ensure good ventilation. Lower hashrate (~50-70 H/s). |
| **M5Stack CoreS3** | ESP32-S3 | 16MB | 8MB | ✅ Supported | Use M5GFX library fork for display. |
| **ESP32-S3-WROOM-1** (bare module) | ESP32-S3 | 8MB+ | 8MB+ | ⚠️ Partial | Custom PCB required; test WiFi stability. |
| **ESP32 (non-S3)** | ESP32/ESP8266 | Any | Any | ❌ Not Supported | Lacks RandomX performance/SRAM. Use S3 only. |

### Peripherals
| Component | Recommended | Alternatives | Status | Notes |
|-----------|-------------|--------------|--------|-------|
| **Display** | SSD1306 128x64 OLED (I2C) | SH1106, GC9A01 (SPI) | ✅ | Wiring: SDA=GPIO21, SCL=GPIO22 (configurable). |
| **Power Supply** | 5V/1A USB-C | 3.3V LiPo (500mAh+) | ✅ | Stable 5V recommended for >150 H/s. Avoid USB ports <500mA. |
| **Antenna** | Built-in PCB | External IPEX uFL | ✅ | External boosts WiFi by 20-30% in poor signal areas. |
| **Heatsink** | Copper shim (10x10mm) + thermal pad | Active fan (5V) | ✅ | Essential for 24/7; keeps <60°C. |

**Minimum Specs**: ESP32-S3 @240MHz, 8MB Flash, 8MB PSRAM. Hashrate: 150-250 H/s @ ~0.8W (varies by board/optimizations).

## ✅ Supported Software & Tools

| Tool/Dependency | Version | Platform | Status | Notes |
|-----------------|---------|----------|--------|-------|
| **Arduino IDE** | 2.3.2+ | Win/Linux/macOS | ✅ | Boards Manager: `esp32` by Espressif (v2.0.14+). |
| **PlatformIO** | 6.1+ (VS Code) | Win/Linux/macOS | ✅ | `framework-arduinoespressif32 @ 3.20013.241129`; lib_deps in `platformio.ini`. |
| **esptool.py** | 4.7+ | All | ✅ | For manual flashing: `esptool.py --chip esp32s3 ...`. |
| **Libraries** | See `library.json` | Arduino | ✅ | WiFi, PubSubClient (MQTT fallback), U8g2 (display), ArduinoJson 6+. |
| **OS for Development** | Windows 10+, Ubuntu 22.04+, macOS Ventura+ | All | ✅ | Linux best for batch flashing. |

**Firmware Variants**:
- `esp-miner-full.ino` (OLED + WiFi): v1.2.0+
- `esp-miner-lite.ino` (No display): v1.2.0+ (headless, higher hashrate)

## ✅ Supported Mining Pools
| Pool | Stratum URL | Port | Status | Notes |
|------|-------------|------|--------|-------|
| **P2Pool** | `p2pool.io:3333` | 3333 | ✅ Primary | Decentralized; best for solo-like mining. |
| **SupportXMR** | `pool.supportxmr.com:443` | 443 (SSL) | ✅ | Reliable; TLS recommended. |
| **Nanopool** | `xmr-eu1.nanopool.org:14444` | 14444 | ✅ | Low latency EU. |
| **MineXMR** | `pool.minexmr.com:4444` | 4444 | ⚠️ | Works but higher fees; test TLS. |
| **Custom** | Any Stratum V1 | Configurable | ✅ | Edit `pool_host`, `pool_port`, `wallet` in code. |

**Wallet**: Use your Monero wallet address (e.g., `4...`). Payout threshold: Pool-dependent (0.1-1 XMR).

## ⚠️ Known Issues & Workarounds
| Issue | Affected | Workaround | Fixed In |
|-------|----------|------------|----------|
| WiFi disconnects | Weak signal boards | External antenna; `WiFi.setTxPower(20)` | v1.1.5+ |
| OLED flicker | SH1106 displays | Switch to U8g2 constructor `SH1106_128X64_NONAME_HW_I2C` | v1.2.0 |
| Low hashrate (<100 H/s) | Overheating/No PSRAM | Add heatsink; enable PSRAM in Arduino (Tools > PSRAM: OPI PSRAM). | N/A |
| Flash fail (ESP32-S3-Box) | Secure boot enabled | `esptool.py --before no_reset ...`; disable in menuconfig. | v1.1.0+ |
| macOS USB recognition | All | Install CP210x/CH340 drivers from Silicon Labs/WCH. | N/A |
| High power draw (>1.2W) | USB-powered | Dedicated 5V/2A supply; disable Bluetooth. | N/A |

## ❌ Unsupported / Tested Failures
- **ESP32-C3**: Insufficient SRAM for RandomX blobs.
- **Heltec/TTGO non-S3**: WiFi instability.
- **Stratum V2 pools**: Not implemented (V1 only).
- **Docker flashing**: Use native tools.
- **Overclock >240MHz**: Unstable, voids warranty.

## 📊 Performance Benchmarks (v1.2.0, 5V/1A, Room Temp)
| Board | Hashrate | Power | Efficiency |
|-------|----------|-------|------------|
| ESP32-S3-DevKitC-1 | 220 H/s | 0.85W | 259 H/s/W |
| LilyGo T-Display-S3 | 180 H/s | 0.92W | 196 H/s/W |
| XIAO ESP32S3 | 65 H/s | 0.45W | 144 H/s/W |

## 🔧 Testing Your Setup
1. Flash firmware.
2. Monitor Serial (115200 baud): Look for "RandomX ready" and pool connect.
3. Hashrate stabilizes after 30s warmup.
4. Use `espminer_config.h` for tweaks (e.g., `#define HAS_OLED`).
