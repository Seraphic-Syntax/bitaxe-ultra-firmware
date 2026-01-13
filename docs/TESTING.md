# Testing

## Pre-flash checks
- `idf.py build` completes without warnings treated as errors
- Firmware image sanity:
  `esptool.py --chip esp32s3 image_info build/<app>.bin`

## Flash checks
- Use correct port: `/dev/ttyACM0` is common for ESP32-S3 USB-CDC
- Consider `idf.py erase_flash` once when switching firmwares

## Runtime checks
- Watch for:
  - reboot loops / panics
  - watchdog resets
  - heap exhaustion
  - thermal sensor NaN
- Validate fan curve and overheat shutdown behavior.
