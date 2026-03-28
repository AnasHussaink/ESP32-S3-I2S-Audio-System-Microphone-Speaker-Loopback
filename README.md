# ESP32-S3 I2S Audio System — Microphone + Speaker + Loopback

A complete beginner-friendly guide to working with I2S audio on **ESP32-S3** using an **INMP441 microphone** and **MAX98357A amplifier + speaker**. Includes individual component testing, wiring diagrams, full working code, expected Serial Monitor output, and troubleshooting for every step.

---

## Table of Contents

- [Hardware Required](#hardware-required)
- [Arduino IDE Settings](#arduino-ide-settings)
- [Project Structure](#project-structure)
- [Part 1 — Speaker Test](#part-1--speaker-test)
- [Part 2 — Microphone Test](#part-2--microphone-test)
- [Part 3 — Mic → Speaker Loopback](#part-3--mic--speaker-loopback)
- [Part 4 — Internet Radio via WiFi](#part-4--internet-radio-via-wifi)
- [Troubleshooting](#troubleshooting)
- [What I Learned](#what-i-learned)

---

## Hardware Required

| Component | Model | Notes |
|-----------|-------|-------|
| Microcontroller | ESP32-S3 Dev Module | Must have OPI PSRAM |
| I2S Amplifier | MAX98357A | 3W mono Class-D amplifier |
| I2S Microphone | INMP441 | MEMS omnidirectional microphone |
| Speaker | 4Ω or 8Ω | 2W–5W recommended |
| Jumper wires | Male-to-male | For breadboard connections |
| USB Cable | Matches your board | For uploading and power |

---

## Arduino IDE Settings

> ⚠️ Set ALL of these before uploading any code — wrong settings cause compile errors and upload failures.

Go to **Tools** in Arduino IDE and set:

| Setting | Value |
|---------|-------|
| Board | **ESP32S3 Dev Module** |
| Partition Scheme | **Huge APP (3MB No OTA/1MB SPIFFS)** |
| PSRAM | **OPI PSRAM** |
| USB CDC On Boot | **Enabled** |
| Upload Mode | **UART0/Hardware CDC** |
| USB Mode | **Hardware CDC and JTAG** |
| Upload Speed | **921600** |
| Flash Frequency | **80MHz** |

> ⚠️ **Partition Scheme must be Huge APP** — the audio library is too large for the default 1.3MB partition and will give a "Sketch too big" error.

> ⚠️ **PSRAM must be OPI PSRAM** — audio buffers require ~720KB which only fits in PSRAM. Without this you get "OOM: failed to allocate" error.

### If Upload Fails — Boot Mode Fix

If you see `Wrong boot mode` or `Waiting for download`:

1. Hold **BOOT** button on ESP32-S3
2. Click **Upload** in Arduino IDE
3. Wait for `Connecting....` in output
4. Release **BOOT** button
5. Wait for `Writing at 0x00010000...`

---

## Project Structure

```
ESP32-S3-Audio-System/
├── README.md
├── LICENSE
├── 01_speaker_test/
│   └── speaker_test.ino
├── 02_mic_test/
│   └── mic_test.ino
├── 03_loopback/
│   └── loopback.ino
└── 04_internet_radio/
    └── internet_radio.ino
```

> ✅ **Recommended order:** Test speaker first → test mic second → run loopback third. Never skip the individual tests.

---

## Part 1 — Speaker Test

Test the MAX98357A amplifier and speaker completely independently before connecting the microphone. This confirms your I2S wiring, amplifier power, and speaker are all working.

### Speaker Wiring

```
ESP32-S3          MAX98357A
────────          ─────────
5V        ───►   VIN        ← must be 5V, NOT 3.3V
GND       ───►   GND
GPIO 3    ───►   BCLK
GPIO 8    ───►   LRC
GPIO 16   ───►   DIN
(SD pin)         leave floating  ← floating = amp always ON
(GAIN pin)       leave floating  ← default 9dB gain

MAX98357A         Speaker
─────────         ───────
OUT+      ───►   + wire
OUT-      ───►   - wire
```

> ⚠️ **Never connect speaker OUT- to GND.** It must go only to the OUT- pin on the amp.

> ⚠️ **VIN must be 5V.** Connecting to 3.3V gives zero sound with no error.

### Speaker Test Code

```cpp
#include "driver/i2s.h"
#include <math.h>

#define I2S_BCLK 3
#define I2S_LRC  8
#define I2S_DOUT 16

#define SAMPLE_RATE 16000

void playBeep(float frequency, int repeat, const char* label);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=============================");
  Serial.println(" MAX98357A Speaker Test");
  Serial.println("=============================\n");

  i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = 0,
    .dma_buf_count        = 8,
    .dma_buf_len          = 1024,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
    .fixed_mclk           = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num   = I2S_BCLK,
    .ws_io_num    = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK) {
    Serial.println("ERROR: I2S driver install failed!");
    while(1);
  }
  if (i2s_set_pin(I2S_NUM_0, &pin_config) != ESP_OK) {
    Serial.println("ERROR: I2S pin config failed!");
    while(1);
  }

  i2s_zero_dma_buffer(I2S_NUM_0);
  delay(500);

  playBeep(500,  20, "Beep 1 -  500 Hz (Low)");
  delay(400);
  playBeep(1000, 20, "Beep 2 - 1000 Hz (Mid)");
  delay(400);
  playBeep(2000, 20, "Beep 3 - 2000 Hz (High)");

  Serial.println("\n=============================");
  Serial.println(" RESULTS");
  Serial.println("=============================");
  Serial.println(" Heard 3 beeps → PASS ✓");
  Serial.println(" No sound      → FAIL ✗");
  Serial.println("=============================");
  Serial.println("\n Test complete!");
}

void loop() {}

void playBeep(float frequency, int repeat, const char* label) {
  Serial.printf("Playing: %s\n", label);
  int16_t tone_buf[512];
  size_t bw = 0;
  for (int j = 0; j < repeat; j++) {
    for (int i = 0; i < 512; i++) {
      tone_buf[i] = (int16_t)(20000 * sin(2 * M_PI * frequency * i / SAMPLE_RATE));
    }
    i2s_write(I2S_NUM_0, tone_buf, sizeof(tone_buf), &bw, portMAX_DELAY);
  }
}
```

### Expected Serial Output — Speaker

Open Serial Monitor at **115200 baud**:

```
=============================
 MAX98357A Speaker Test
=============================

Playing: Beep 1 -  500 Hz (Low)
Playing: Beep 2 - 1000 Hz (Mid)
Playing: Beep 3 - 2000 Hz (High)

=============================
 RESULTS
=============================
 Heard 3 beeps → PASS ✓
 No sound      → FAIL ✗
=============================

 Test complete!
```

You should **hear 3 beeps** at different pitches from the speaker.

### Speaker Troubleshooting

| Problem | Cause | Fix |
|---------|-------|-----|
| No sound | VIN connected to 3.3V | Move VIN to 5V |
| No sound | SD pin connected to GND | Leave SD floating |
| Very quiet | Low gain | Connect GAIN pin to 3.3V for +12dB |
| Crackling | Speaker wired wrong | Check OUT- is NOT connected to GND |
| I2S install failed | GPIO conflict | Check no other peripheral using same pins |
| Nothing on serial | Wrong baud rate | Set Serial Monitor to 115200 |

---

## Part 2 — Microphone Test

Test the INMP441 microphone independently. Shows a live volume bar and prints PASS or FAIL with amplitude statistics.

> ⚠️ **Key technical detail:** INMP441 outputs 24-bit audio packed inside 32-bit I2S frames. You must read 32-bit and shift right by 14 bits. Reading as 16-bit gives only zeros.

### Microphone Wiring

```
ESP32-S3          INMP441
────────          ───────
3.3V      ───►   VDD     ← 3.3V only — 5V permanently damages mic
GND       ───►   GND
GPIO 18   ───►   SCK     ← Serial clock
GPIO 17   ───►   WS      ← Word select
GPIO 15   ───►   SD      ← Serial data output
GND       ───►   L/R     ← Must be GND — selects LEFT channel
```

> ⚠️ **L/R pin must be connected to GND.** If floating, mic outputs nothing at all.

> ⚠️ **VDD must be 3.3V only.** 5V will permanently damage the INMP441.

### Microphone Test Code

```cpp
#include "driver/i2s.h"

#define I2S_SCK  18
#define I2S_WS   17
#define I2S_SD   15

#define SAMPLE_RATE 16000

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=============================");
  Serial.println(" INMP441 Microphone Test");
  Serial.println("=============================\n");

  i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,  // mandatory for INMP441
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,  // L/R = GND = LEFT
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = 1024,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num   = I2S_SCK,
    .ws_io_num    = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S_SD
  };

  if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK) {
    Serial.println("ERROR: I2S driver install failed!");
    while(1);
  }
  if (i2s_set_pin(I2S_NUM_0, &pin_config) != ESP_OK) {
    Serial.println("ERROR: I2S pin config failed!");
    while(1);
  }

  i2s_zero_dma_buffer(I2S_NUM_0);
  delay(500);

  Serial.println("Listening for 5 seconds...");
  Serial.println(">>> SPEAK OR CLAP NOW <<<\n");

  int32_t  raw_buf[64];
  int      audioEvents  = 0;
  int32_t  maxAmplitude = 0;
  int64_t  sumAmplitude = 0;
  int      totalSamples = 0;

  unsigned long startTime = millis();

  while (millis() - startTime < 5000) {
    size_t bytes_read = 0;
    i2s_read(I2S_NUM_0, raw_buf, sizeof(raw_buf), &bytes_read, portMAX_DELAY);

    int samples_read = bytes_read / 4;
    for (int i = 0; i < samples_read; i++) {
      int16_t sample    = (int16_t)(raw_buf[i] >> 14);
      int32_t absSample = abs(sample);
      sumAmplitude += absSample;
      totalSamples++;
      if (absSample > maxAmplitude) maxAmplitude = absSample;
      if (absSample > 200) audioEvents++;
    }

    int16_t latest = (int16_t)(raw_buf[0] >> 14);
    int bar = map(abs(latest), 0, 5000, 0, 20);
    bar = constrain(bar, 0, 20);
    Serial.print("Level [");
    for (int b = 0; b < 20; b++) Serial.print(b < bar ? "=" : " ");
    Serial.printf("] amp: %d\n", abs(latest));
  }

  i2s_driver_uninstall(I2S_NUM_0);
  int32_t avgAmplitude = totalSamples > 0 ? (int32_t)(sumAmplitude / totalSamples) : 0;

  Serial.println("\n=============================");
  Serial.println(" RESULTS");
  Serial.println("=============================");
  Serial.printf(" Max amplitude : %d\n", (int)maxAmplitude);
  Serial.printf(" Avg amplitude : %d\n", (int)avgAmplitude);
  Serial.printf(" Audio events  : %d\n", audioEvents);
  Serial.println("-----------------------------");

  if (audioEvents == 0 && maxAmplitude == 0) {
    Serial.println(" FAIL ✗ — Amplitude = 0");
    Serial.println(" SD pin not driving signal");
    Serial.println(" Check wiring or replace mic");
  } else if (audioEvents < 5) {
    Serial.println(" WARNING — Very weak signal");
    Serial.println(" Speak louder or check L/R pin");
  } else {
    Serial.println(" PASS ✓ — Microphone working!");
  }

  Serial.println("=============================");
  Serial.println("\n Restarting in 3 seconds...");
  delay(3000);
  ESP.restart();
}

void loop() {}
```

### Expected Serial Output — Microphone

```
=============================
 INMP441 Microphone Test
=============================

Listening for 5 seconds...
>>> SPEAK OR CLAP NOW <<<

Level [                    ] amp: 12
Level [===                 ] amp: 980
Level [========            ] amp: 2340
Level [===============     ] amp: 4100
Level [====================] amp: 6200
Level [========            ] amp: 2100

=============================
 RESULTS
=============================
 Max amplitude : 6200
 Avg amplitude : 312
 Audio events  : 1247
-----------------------------
 PASS ✓ — Microphone working!
=============================

 Restarting in 3 seconds...
```

**What the numbers mean:**

| Value | Meaning |
|-------|---------|
| Max amplitude > 1000 | Strong healthy signal |
| Max amplitude 100–1000 | Weak but working |
| Max amplitude = 0 | Mic not working at all |
| Audio events > 100 | Healthy signal |
| Audio events = 0 | No signal detected |

### Microphone Troubleshooting

| Problem | Cause | Fix |
|---------|-------|-----|
| Amplitude = 0 always | Dead mic | Test SD pin with multimeter — should fluctuate |
| Amplitude = 0 always | L/R pin floating | Solder L/R to GND |
| Amplitude = 0 always | Reading 16-bit | Use I2S_BITS_PER_SAMPLE_32BIT |
| Amplitude = 0 always | VDD = 5V | Move to 3.3V — mic is permanently damaged at 5V |
| Very low amplitude | SD and WS swapped | Swap GPIO 15 and GPIO 17 in code |
| Very low amplitude | Poor solder joint | Resolder all 6 pins |
| Distorted audio | Wrong bit shift | Use >> 14 |

---

## Part 3 — Mic → Speaker Loopback

The full working system. Speak into the mic and hear yourself instantly from the speaker. Includes noise gate, feedback detection, and automatic mute protection.

### Full Wiring (Both Together)

```
ESP32-S3          MAX98357A (Speaker)
────────          ──────────────────
5V        ───►   VIN
GND       ───►   GND
GPIO 3    ───►   BCLK
GPIO 8    ───►   LRC
GPIO 16   ───►   DIN

ESP32-S3          INMP441 (Microphone)
────────          ───────────────────
3.3V      ───►   VDD
GND       ───►   GND
GPIO 18   ───►   SCK
GPIO 17   ───►   WS
GPIO 15   ───►   SD
GND       ───►   L/R
```

> ⚠️ **Keep the speaker physically away from the microphone** to prevent acoustic feedback.

### Key Tuning Parameters

| Parameter | Default | Purpose |
|-----------|---------|---------|
| `BIT_SHIFT` | 14 | Mic signal scaling — adjusts input volume |
| `NOISE_GATE` | 1000 | Removes background noise below this level |
| `VOLUME_MULT` | 1 | Output gain — keep at 1 to avoid feedback |
| `FEEDBACK_LIMIT` | 20000 | Peak threshold — triggers auto mute |

### Loopback Code

```cpp
#include "driver/i2s.h"

// Speaker pins (MAX98357A)
#define SPK_BCLK  3
#define SPK_LRC   8
#define SPK_DOUT  16

// Microphone pins (INMP441)
#define MIC_SCK   18
#define MIC_WS    17
#define MIC_SD    15

// Tuning
#define SAMPLE_RATE      16000
#define BUFFER_FRAMES    256
#define BIT_SHIFT        14
#define NOISE_GATE       1000    // raise to 1500 or 2000 if feedback starts
#define VOLUME_MULT      1       // keep at 1 — raising causes feedback
#define FEEDBACK_LIMIT   20000   // if peak exceeds this, mute that chunk

static int32_t mic_buf[BUFFER_FRAMES];
static int16_t spk_buf[BUFFER_FRAMES];

static int16_t peak_history[8] = {0};
static int     peak_idx = 0;

bool isFeedbackDetected() {
  int count = 0;
  for (int i = 0; i < 8; i++) {
    if (peak_history[i] > FEEDBACK_LIMIT) count++;
  }
  return count >= 6;  // 6 out of 8 chunks clipping = feedback
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=============================");
  Serial.println("  Mic → Speaker Loopback");
  Serial.println("  (Feedback Protected)");
  Serial.println("=============================\n");

  // Speaker — I2S_NUM_0 TX
  i2s_config_t spk_cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = 0,
    .dma_buf_count        = 8,
    .dma_buf_len          = 512,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
    .fixed_mclk           = 0
  };
  i2s_pin_config_t spk_pins = {
    .bck_io_num   = SPK_BCLK,
    .ws_io_num    = SPK_LRC,
    .data_out_num = SPK_DOUT,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };
  if (i2s_driver_install(I2S_NUM_0, &spk_cfg, 0, NULL) != ESP_OK) {
    Serial.println("ERROR: Speaker I2S install failed!"); while(1);
  }
  if (i2s_set_pin(I2S_NUM_0, &spk_pins) != ESP_OK) {
    Serial.println("ERROR: Speaker pin config failed!"); while(1);
  }
  i2s_zero_dma_buffer(I2S_NUM_0);
  Serial.println("[OK] Speaker  → I2S_NUM_0");

  // Microphone — I2S_NUM_1 RX
  i2s_config_t mic_cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = 512,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };
  i2s_pin_config_t mic_pins = {
    .bck_io_num   = MIC_SCK,
    .ws_io_num    = MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = MIC_SD
  };
  if (i2s_driver_install(I2S_NUM_1, &mic_cfg, 0, NULL) != ESP_OK) {
    Serial.println("ERROR: Mic I2S install failed!"); while(1);
  }
  if (i2s_set_pin(I2S_NUM_1, &mic_pins) != ESP_OK) {
    Serial.println("ERROR: Mic pin config failed!"); while(1);
  }
  i2s_zero_dma_buffer(I2S_NUM_1);
  Serial.println("[OK] Microphone → I2S_NUM_1");

  Serial.println("\n>>> Speak into mic. Keep speaker AWAY from mic. <<<\n");
}

void loop() {
  size_t bytes_read    = 0;
  size_t bytes_written = 0;

  // 1. Read mic
  i2s_read(I2S_NUM_1, mic_buf, sizeof(mic_buf), &bytes_read, portMAX_DELAY);
  int samples = bytes_read / 4;

  // 2. Find peak of this chunk
  int16_t chunk_peak = 0;
  for (int i = 0; i < samples; i++) {
    int16_t s = (int16_t)(mic_buf[i] >> BIT_SHIFT);
    int16_t a = abs(s);
    if (a > chunk_peak) chunk_peak = a;
  }

  // 3. Store in rolling peak history
  peak_history[peak_idx % 8] = chunk_peak;
  peak_idx++;

  // 4. Feedback detection
  bool feedback = isFeedbackDetected();

  if (feedback) {
    memset(spk_buf, 0, samples * sizeof(int16_t));
    static unsigned long last_warn = 0;
    if (millis() - last_warn > 1000) {
      Serial.println("⚠ FEEDBACK DETECTED — output muted. Move speaker away from mic!");
      last_warn = millis();
    }
  } else {
    // 5. Normal processing — noise gate + volume
    for (int i = 0; i < samples; i++) {
      int16_t sample = (int16_t)(mic_buf[i] >> BIT_SHIFT);

      if (abs(sample) < NOISE_GATE) {
        spk_buf[i] = 0;
        continue;
      }

      int32_t boosted = (int32_t)sample * VOLUME_MULT;
      if (boosted >  32767) boosted =  32767;
      if (boosted < -32768) boosted = -32768;
      spk_buf[i] = (int16_t)boosted;
    }
  }

  // 6. Write to speaker
  i2s_write(I2S_NUM_0, spk_buf, samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);

  // 7. Serial level meter (every 32 chunks)
  static int call_count = 0;
  if (++call_count % 32 == 0) {
    int16_t peak = 0;
    for (int i = 0; i < samples; i++) {
      int16_t a = abs(spk_buf[i]);
      if (a > peak) peak = a;
    }
    int bar = map(peak, 0, 32767, 0, 30);
    bar = constrain(bar, 0, 30);

    const char* status;
    if (feedback)           status = "MUTED-FEEDBACK";
    else if (peak == 0)     status = "SILENCE       ";
    else if (peak < 5000)   status = "LOW           ";
    else if (peak < 20000)  status = "GOOD  ✓       ";
    else                    status = "HIGH          ";

    Serial.print("Level [");
    for (int b = 0; b < 30; b++) Serial.print(b < bar ? "=" : " ");
    Serial.printf("] %5d  %s\n", peak, status);
  }
}
```

### Expected Serial Output — Loopback

```
=============================
  Mic → Speaker Loopback
  (Feedback Protected)
=============================

[OK] Speaker  → I2S_NUM_0
[OK] Microphone → I2S_NUM_1

>>> Speak into mic. Keep speaker AWAY from mic. <<<

Level [                              ]     0  SILENCE
Level [=========                     ]  3200  LOW
Level [===================           ] 12400  GOOD  ✓
Level [============================  ] 28000  HIGH
⚠ FEEDBACK DETECTED — output muted. Move speaker away from mic!
Level [                              ]     0  MUTED-FEEDBACK
Level [==============                ]  8100  GOOD  ✓
```

### Loopback Troubleshooting

| Problem | Cause | Fix |
|---------|-------|-----|
| Constant feedback/muting | Speaker too close to mic | Move speaker at least 30cm away |
| No sound output | Wiring issue | Re-run speaker test individually |
| Mic not picking up | Wiring issue | Re-run mic test individually |
| Distorted output | VOLUME_MULT too high | Keep VOLUME_MULT = 1 |
| Always muted | FEEDBACK_LIMIT too low | Increase FEEDBACK_LIMIT to 25000 |
| Background hiss | NOISE_GATE too low | Increase NOISE_GATE to 1500 or 2000 |

---

## Part 4 — Internet Radio via WiFi

Stream live internet radio stations over WiFi with crystal clear audio through the speaker.

> ⚠️ **Note:** ESP32-S3 does NOT support Bluetooth Classic (A2DP). It only supports BLE 5.0. WiFi streaming is the correct approach for ESP32-S3.

### Additional Library Required

In Arduino IDE go to **Tools → Manage Libraries** and install:
- **ESP32-audioI2S** by schreibfaul1

### Internet Radio Code

```cpp
#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"

#define WIFI_SSID     "your_wifi_name"       // change this
#define WIFI_PASSWORD "your_wifi_password"   // change this

#define I2S_DOUT 16
#define I2S_BCLK 3
#define I2S_LRC  8

Audio audio;

void setup() {
  Serial.begin(115200);

  if (!psramFound()) {
    Serial.println("ERROR: PSRAM not found!");
    Serial.println("Go to Tools → PSRAM → OPI PSRAM");
    while(1);
  }
  Serial.printf("PSRAM OK — Free: %d bytes\n", ESP.getFreePsram());

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(18); // 0 to 21

  audio.connecttohost("http://stream.live.vc.bbcmedia.co.uk/bbc_world_service");
}

void loop() {
  audio.loop();
}

void audio_info(const char *info)            { Serial.printf("INFO: %s\n", info); }
void audio_showstation(const char *info)     { Serial.printf("Station: %s\n", info); }
void audio_showstreamtitle(const char *info) { Serial.printf("Now playing: %s\n", info); }
```

### Radio Stations to Try

```cpp
// BBC World Service (recommended — very stable)
audio.connecttohost("http://stream.live.vc.bbcmedia.co.uk/bbc_world_service");

// Jazz Radio
audio.connecttohost("http://jazz.streamr.ru:8000/jazz-64");

// Classical Music
audio.connecttohost("http://live-radio01.mediahubaustralia.com/2ABCStreamAAC");

// Antenne Thueringen
audio.connecttohost("http://stream.antennethueringen.de/live/aac-64/stream.antennethueringen.de/");
```

### Expected Serial Output — Internet Radio

```
PSRAM OK — Free: 8388608 bytes
Connecting to WiFi.....
WiFi connected!
192.168.1.x
INFO: Connect to http://stream.live.vc.bbcmedia.co.uk/bbc_world_service
INFO: AAC stream found
Station: BBC World Service
Now playing: BBC News Hour
```

### Internet Radio Troubleshooting

| Problem | Cause | Fix |
|---------|-------|-----|
| OOM error | PSRAM disabled | Tools → PSRAM → OPI PSRAM |
| Sketch too big | Wrong partition | Tools → Partition → Huge APP |
| WiFi stuck connecting | Wrong password or hotspot off | Check exact SSID and password |
| No audio after connecting | Wrong pins | Check BCLK/LRC/DOUT wiring |
| Stream not found | Station offline | Try a different station URL |

---

## Troubleshooting — Common Upload Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `Sketch too big` | Default partition | Tools → Partition Scheme → Huge APP |
| `OOM: failed to allocate` | PSRAM not enabled | Tools → PSRAM → OPI PSRAM |
| `Wrong boot mode` | Board not in download mode | Hold BOOT → click Upload → release BOOT |
| `Waiting for download` | Same as above | Hold BOOT + press EN, then upload |
| Garbage in Serial Monitor | Wrong baud rate | Set Serial Monitor to 115200 |
| Port not found | Wrong COM port | Check Device Manager for correct port |

---

## What I Learned

- **INMP441** sends 24-bit audio packed inside 32-bit I2S frames — reading 16-bit gives only zeros. Must read 32-bit and shift right by 14 bits.
- **INMP441 L/R pin** must be physically connected to GND — floating gives zero output always.
- **MAX98357A VIN** must be 5V — 3.3V gives zero sound with no error message.
- **MAX98357A SD pin** left floating = always on which is correct for basic use.
- **ESP32-S3 does not support Bluetooth Classic** — only BLE 5.0. Bluetooth A2DP audio streaming from phone is not possible on ESP32-S3.
- **Partition Scheme must be Huge APP** — audio library is too large for the default 1.3MB partition.
- **PSRAM must be OPI PSRAM** — audio buffer needs ~720KB which only fits in PSRAM.
- **GPIO 46 on ESP32-S3** is a strapping pin — avoid using it for I2S or critical signals.
- **Test each component individually first** — always test speaker alone, then mic alone, then combine. Debugging combined systems is much harder.
- **Feedback is physical** — keeping speaker and mic physically separated is more effective than any software fix.

---

## License

MIT License — free to use, modify and share.

---

## Author

Built and documented by **Huzaifa** while learning ESP32-S3 I2S audio development from scratch.
