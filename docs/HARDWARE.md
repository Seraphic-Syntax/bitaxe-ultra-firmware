# Hardware Notes

## Target
- ESP32-S3 (ESP-IDF target: `esp32s3`)

## Default pin assumptions (adjust per Bitaxe variant)
- I2C: SDA=21, SCL=22
- Temp sensor: EMC2101 assumed at I2C address 0x4C
- Fan PWM GPIO: 8

## Bring-up checklist
- Serial port appears in `/dev/ttyACM*` or `/dev/ttyUSB*`
- I2C bus is stable (pull-ups present)
- Temperature read is not NaN
- Fan responds to PWM changes
- Device does not reboot under load
