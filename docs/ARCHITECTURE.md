# Architecture

This firmware is an ESP-IDF project structured similarly to bitaxeorg/ESP-Miner.

## Runtime loop
The main worker task runs periodically:
1. Mining work (small unit per loop)
2. Thermal update (temp read + fan curve + throttling/shutdown)
3. Vcore update (safe placeholder)
4. Global stats update (hash rate estimate, shares)

## Modules
- `global_state.*`:
  Holds shared runtime state and computes rolling hash-rate estimates.
- `mining.*`:
  Software sha256d demo loop (toy target) to validate stability and performance.
  Intended to be replaced by real BM1370 work submission + nonce scanning.
- `thermal.*`:
  Reads a temperature sensor over I2C (EMC2101 scaffold) and drives fan PWM.
  Applies conservative frequency throttling and overheat shutdown.
- `vcore.*`:
  Placeholder for real Vcore control (depends on Bitaxe hardware implementation).
- `i2c.*`:
  Minimal I2C wrapper.

## Sustainability goals
- Avoid thermal runaway (fan curve + throttle)
- Avoid unstable overclock/undervolt (conservative clamps)
- Keep system responsive (small work chunks, avoid blocking)
