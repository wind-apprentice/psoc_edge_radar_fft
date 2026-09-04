# psoc_edge_radar_fft

## 1. Project Summary

This project runs on PSOC Edge E84 AI Kit and captures radar IF samples from the onboard XENSIV BGT60TR13C sensor.

Processing chain used in this branch:

- radar signal frame acquisition
- DC removal (per chirp)
- windowing (Hamming)
- range FFT
- magnitude spectrum integration across chirps
- spectrum diagnostics output on UART

This branch follows Infineon sample flow so vendor support can reproduce and debug easily.

## 2. Reference Repositories

This implementation references public Infineon code and libraries:

- https://github.com/Infineon/mtb-example-ce241721-xensiv-60ghz-static-distance
- https://github.com/Infineon/mtb-example-psoc-edge-hello-world
- https://github.com/Infineon/sensor-xensiv-bgt60trxx
- mtb://sensor-dsp (ModusToolbox dependency)

## 3. Target Hardware

- Board: KIT_PSE84_AI (PSOC Edge E84 AI Kit)
- Radar sensor: onboard XENSIV BGT60TR13C
- Link between MCU and radar: SPI + IRQ
- UART for logs: KitProg3 virtual COM, 115200-8-N-1

## 4. Input and Output

Input:

- raw radar frame samples read from sensor FIFO
- frame layout is based on XENSIV_BGT60TRXX_CONF_* settings

Output:

- frame diagnostics on UART
- spectrum summary bins (for example B2/B6/B10/B14)
- peak bin and SNR-like metric for bring-up validation

Note:

- Distance return from get_static_distance() is not used as the main output in this branch.
- Main output target is spectrum and related diagnostics.

## 5. Project Structure

- proj_cm33_ns: non-secure app, radar acquire + processing + UART output
- proj_cm33_s: secure side project
- proj_cm55: CM55 side project

## 6. Open in ModusToolbox IDE

1. Start ModusToolbox IDE.
2. Use File -> Import -> Existing Application In-Place.
3. Select folder: psoc_edge_radar_fft.
4. Wait for workspace indexing to finish.

Alternative (CLI):

- open a ModusToolbox shell at this folder
- run: make getlibs

## 7. Build and Program

In ModusToolbox shell at project root (psoc_edge_radar_fft):

1. Fetch dependencies:
   - make getlibs
2. Build all subprojects:
   - make build
3. Program board:
   - make program

If needed, clean and rebuild:

- make clean
- make build

## 8. Runtime Mode

In proj_cm33_ns/main.c:

- PHASE1_APP_MODE = 2U: sensor transport smoke test
- PHASE1_APP_MODE = 3U: real radar signal to FFT/spectrum mode (default)

## 9. UART Verification

Expected startup logs include mode and spectrum source. In mode 3, each frame prints values similar to:

- Frame=...
- RawPeakBin=...
- SearchPeakBin=...
- Peak=...
- Noise=...
- SNRx=...
- B2/B6/B10/B14=...

## 10. License and IP Notice

This project contains code derived from Infineon public examples and public ModusToolbox libraries.

Before customer delivery, keep the original license headers and attributions in source files.
Do not remove upstream copyright/license statements.