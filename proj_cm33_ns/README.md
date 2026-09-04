# PSoC Edge E84 AI Kit Radar Bring-up and FFT Spectrum (CM33 NS)

This README summarizes the current implementation progress for:

- onboard radar signal acquisition (BGT60TR13C)
- integration with PSoC Edge E84 AI kit (CM33 non-secure app)
- FFT-based spectrum generation
- UART terminal output for validation and debugging

The implementation is inside this project folder:

- `proj_cm33_ns/main.c`
- `proj_cm33_ns/phase1/phase1_config.h`
- `proj_cm33_ns/phase1/phase1_mock_data.c`
- `proj_cm33_ns/phase1/phase1_radar_pipeline.c`

---

## 1. Data Flow Implemented

### 1.1 End-to-end path

1. Radar front-end (onboard BGT60TR13C) produces IF samples.
2. PSoC Edge configures SPI and radar IRQ using BSP aliases.
3. Firmware starts one frame, waits for IRQ, reads one FIFO frame.
4. Frame data is mapped into phase1 FFT pipeline input size.
5. FFT pipeline performs:
   - per-chirp DC removal
   - zero-padding
   - Hamming window
   - FFT
   - magnitude spectrum
   - chirp integration
   - peak search
6. UART prints frame-level diagnostics and spectrum summary.

### 1.2 One-frame-per-loop behavior

In `sensor_fft` mode, each loop does:

- clear stale interrupt state
- `start_frame(true)`
- wait for a fresh IRQ (with timeout)
- `get_fifo_data(...)`
- process FFT/spectrum
- `start_frame(false)`

This guarantees every printed line corresponds to a fresh radar frame.

---

## 2. Runtime Modes

Mode is selected by `PHASE1_APP_MODE` in `proj_cm33_ns/main.c`.

- `1` (`PHASE1_APP_MODE_MOCK`)
  - synthetic radar-like samples
  - validates FFT/spectrum pipeline without hardware dependency

- `2` (`PHASE1_APP_MODE_SENSOR_SMOKE`)
  - onboard radar digital path smoke test
  - validates SPI + IRQ + FIFO transport
  - uses sensor data test mode

- `3` (`PHASE1_APP_MODE_SENSOR_FFT`)
  - onboard real IF signal path
  - computes FFT/spectrum and prints peak/noise/SNR-like metrics

Current recommended mode for real bring-up:

- `PHASE1_APP_MODE = 3`

---

## 3. UART Output Fields Meaning

Example line:

```text
Frame=1200 | WaitMs=31 | RawPeakBin=2 | SearchPeakBin=6 | Peak=1.611 | Range=9.78 cm | Noise=0.474 | SNRx=3.40 | B2=7.11 B6=1.61 B10=1.00 B14=0.71
```

Field definitions:

- `Frame`: frame index
- `WaitMs`: wait time for new IRQ (frame pacing indicator)
- `RawPeakBin`: strongest bin from full valid range (can be near-field dominated)
- `SearchPeakBin`: strongest bin from filtered search range (ignores very near bins)
- `Peak`: magnitude at `SearchPeakBin`
- `Range`: bin-to-range converted value (for trend; not final calibrated absolute distance)
- `Noise`: average magnitude over search range
- `SNRx`: `Peak / Noise`
- `B2/B6/B10/B14`: sampled spectrum bins for quick shape inspection

---

## 4. How to Use

## 4.1 Hardware

1. Connect KIT_PSE84_AI to PC through KitProg3 USB.
2. Open UART terminal on the corresponding COM port:
   - Baud: `115200`
   - Data: `8`
   - Parity: `None`
   - Stop bits: `1`

No external radar module is required for this stage because the board has onboard BGT60TR13C.

## 4.2 Build and Program (Eclipse)

1. Open application workspace in Eclipse for ModusToolbox.
2. Select active configuration for `proj_cm33_ns` (Debug).
3. Build the application (or build all multicore projects as required by your launch config).
4. Program using KitProg3 launch configuration.
5. Open UART terminal and observe logs.

## 4.3 Select test mode

Edit `proj_cm33_ns/main.c`:

```c
#if !defined(PHASE1_APP_MODE)
#define PHASE1_APP_MODE (3U)
#endif
```

Use:

- `1U` for synthetic FFT validation
- `2U` for radar transport smoke test
- `3U` for real radar IF FFT/spectrum

---

## 5. What Has Been Verified So Far

1. `sensor_smoke` mode confirms transport path is alive:
   - SPI init works
   - IRQ triggers repeatedly
   - FIFO frames are read continuously

2. `sensor_fft` mode confirms processing path is alive:
   - fresh-frame pacing via `WaitMs`
   - frame-by-frame FFT/spectrum generation
   - stable peak/noise/SNR-like outputs

3. Decimation from sensor frame to phase1 FFT input is active with simple anti-aliasing:
   - average over each decimation block before feeding FFT

---

## 6. Important Source Sections

- `proj_cm33_ns/main.c`
  - mode switch and runtime entry points
  - onboard radar SPI/IRQ initialization
  - smoke test loop
  - real-frame FFT loop and UART diagnostics

- `proj_cm33_ns/phase1/phase1_radar_pipeline.c`
  - Hamming window generation
  - radix-2 FFT implementation
  - per-chirp DC removal
  - spectrum integration and peak extraction

- `proj_cm33_ns/phase1/phase1_mock_data.c`
  - synthetic test vectors used by mode 1

---

## 7. Troubleshooting

### 7.1 Program keeps running; how to stop

Options:

1. Press board reset button.
2. Attach debugger and use Suspend/Halt.
3. Program a different image.
4. Power-cycle the board (last resort).

### 7.2 `make` not found in terminal

If command-line `make` is unavailable, build/program from Eclipse IDE directly.

### 7.3 Peak stuck in very near bins

1. This can be near-field coupling/background.
2. Use `SearchPeakBin` instead of `RawPeakBin` for early validation.
3. Place a strong reflector (metal plate) at known distances and compare trend changes.

---

## 8. Suggested Validation Procedure

1. Baseline:
   - run mode 3 with no deliberate target for 200+ frames
   - capture `SearchPeakBin`, `Noise`, `SNRx`

2. Controlled target:
   - place metal reflector at fixed distances (for example 20 cm, 40 cm, 60 cm)
   - capture 200+ frames per distance
   - compare bin and SNR-like trend

3. In/out test:
   - insert and remove target repeatedly
   - confirm `SearchPeakBin` and `SNRx` respond consistently

---

## 9. Current Limitations

1. Range conversion is currently used mainly for trend observation, not final calibrated absolute distance.
2. FFT pipeline currently uses an internal phase1 configuration (reduced sample count via decimation).
3. A full production version should align FFT configuration directly with sensor configuration and apply calibration.

---

## 10. Next Planned Steps

1. Add periodic aggregate statistics (mean/std/hit rate) every N frames.
2. Export compact CSV spectrum snapshots for offline plotting.
3. Add optional clutter/background subtraction for clearer target contrast.
4. Align FFT parameters to the full sensor profile for improved range fidelity.
