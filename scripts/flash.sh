#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"
BAUD="${2:-460800}"

idf.py -p "$PORT" -b "$BAUD" flash monitor
