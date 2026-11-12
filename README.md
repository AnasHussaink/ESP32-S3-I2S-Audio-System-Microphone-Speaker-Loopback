# ESP32-S3 I2S Audio System

A practical embedded-audio project for the ESP32-S3 demonstrating digital microphone input, I2S speaker output, real-time microphone-to-speaker loopback, and Wi-Fi internet radio.

## Overview

This repository is organized as a progressive audio lab:
1. Test the MAX98357A speaker path.
2. Test the INMP441 microphone.
3. Combine both into real-time I2S loopback.
4. Extend the platform with Wi-Fi internet radio.

The project focuses on reliable I2S configuration, buffering, signal scaling, noise handling, and feedback protection.

## Hardware

| Component | Model | Purpose |
|---|---|---|
| MCU | ESP32-S3 Dev Module | Audio processing and Wi-Fi |
| Microphone | INMP441 | I2S digital input |
| Amplifier | MAX98357A | I2S Class-D output |
| Speaker | 4Ω / 8Ω | Audio playback |

### Pin Configuration

INMP441: VDD → 3.3V, GND → GND, L/R → GND, SCK → GPIO18, WS → GPIO17, SD → GPIO15.

MAX98357A: VIN → 5V, GND → GND, BCLK → GPIO3, LRC → GPIO8, DIN → GPIO16.

> Never connect MAX98357A OUT− to GND. The INMP441 must be powered from 3.3V.

## Project Structure

01_speaker_test/ — MAX98357A output test  
02_mic_test/ — INMP441 input test  
03_loopback/ — microphone-to-speaker loopback  
04_internet_radio/ — Wi-Fi audio playback

## Technical Highlights

- ESP32-S3 I2S RX/TX configuration
- 16-bit speaker playback and 32-bit INMP441 capture
- DMA buffering and PSRAM support
- Real-time audio signal processing
- Noise-gate processing
- Peak monitoring and feedback detection
- Wi-Fi audio streaming

## Getting Started

Run the examples in this order:

Speaker test → Microphone test → Loopback → Internet radio

Open the corresponding Arduino sketch, configure the ESP32-S3 board, connect the hardware, and upload.

## Arduino IDE

Recommended configuration:
- ESP32S3 Dev Module
- Huge APP / 3MB partition
- OPI PSRAM
- USB CDC on Boot enabled
- Hardware CDC and JTAG
- 921600 upload speed

## Troubleshooting

If microphone amplitude is zero, check 3.3V power, the L/R-to-GND connection, GPIO mapping, and 32-bit I2S capture.

If there is no speaker output, check amplifier power, speaker wiring, and BCLK/LRC/DIN configuration.

If feedback occurs, move the microphone and speaker apart and reduce output gain.

## Learning Outcomes

Embedded audio, digital I2S interfaces, DMA buffering, real-time signal processing, hardware debugging, and resource-aware ESP32 development.

## License

See LICENSE.


## Development and quality

[![Repository quality](https://github.com/AnasHussaink/ESP32-S3-I2S-Audio-System-Microphone-Speaker-Loopback/actions/workflows/quality.yml/badge.svg)](https://github.com/AnasHussaink/ESP32-S3-I2S-Audio-System-Microphone-Speaker-Loopback/actions/workflows/quality.yml)

See [CONTRIBUTING.md](CONTRIBUTING.md) for branch/PR workflow, local checks and CI coverage.
