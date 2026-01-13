#!/usr/bin/env bash
set -euo pipefail

APP_NAME="bitaxe_ultra"
OUT_DIR="dist"

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

cp -v "build/${APP_NAME}.bin" "$OUT_DIR"/
cp -v "build/bootloader/bootloader.bin" "$OUT_DIR"/
cp -v "build/partition_table/partition-table.bin" "$OUT_DIR"/ 2>/dev/null || true
cp -v "build/flasher_args.json" "$OUT_DIR"/ 2>/dev/null || true

tar -czf "${APP_NAME}_firmware.tar.gz" -C "$OUT_DIR" .
echo "Wrote ${APP_NAME}_firmware.tar.gz"
