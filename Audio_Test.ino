#include "driver/i2s.h"

// ── Speaker pins (MAX98357A) ─────────────────────────
#define SPK_BCLK  3
#define SPK_LRC   8
#define SPK_DOUT  16

// ── Microphone pins (INMP441) ────────────────────────
#define MIC_SCK   18
#define MIC_WS    17
#define MIC_SD    15

// ── Tuning ───────────────────────────────────────────
#define SAMPLE_RATE      16000
#define BUFFER_FRAMES    256
#define BIT_SHIFT        14
#define NOISE_GATE       1000    // raise this if feedback starts — try 1500, 2000
#define VOLUME_MULT      1       // keep at 1 — DO NOT raise, causes feedback
#define FEEDBACK_LIMIT   20000   // if peak exceeds this, mute output that chunk

static int32_t mic_buf[BUFFER_FRAMES];
static int16_t spk_buf[BUFFER_FRAMES];

// Tracks recent peak history to detect runaway feedback
static int16_t peak_history[8] = {0};
static int     peak_idx = 0;

bool isFeedbackDetected() {
  // If last 4 consecutive peaks are all above FEEDBACK_LIMIT → feedback
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

  // ── Speaker I2S_NUM_0 TX ─────────────────────────────
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

  // ── Mic I2S_NUM_1 RX ─────────────────────────────────
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

  // ── 1. Read mic ──────────────────────────────────────
  i2s_read(I2S_NUM_1, mic_buf, sizeof(mic_buf), &bytes_read, portMAX_DELAY);
  int samples = bytes_read / 4;

  // ── 2. Find peak of this raw chunk ───────────────────
  int16_t chunk_peak = 0;
  for (int i = 0; i < samples; i++) {
    int16_t s = (int16_t)(mic_buf[i] >> BIT_SHIFT);
    int16_t a = abs(s);
    if (a > chunk_peak) chunk_peak = a;
  }

  // Store in rolling history
  peak_history[peak_idx % 8] = chunk_peak;
  peak_idx++;

  // ── 3. Feedback detection — mute if runaway ──────────
  bool feedback = isFeedbackDetected();

  if (feedback) {
    // Zero out speaker output completely
    memset(spk_buf, 0, samples * sizeof(int16_t));
    static unsigned long last_warn = 0;
    if (millis() - last_warn > 1000) {
      Serial.println("⚠ FEEDBACK DETECTED — output muted. Move speaker away from mic!");
      last_warn = millis();
    }
  } else {
    // ── 4. Normal processing ─────────────────────────
    for (int i = 0; i < samples; i++) {
      int16_t sample = (int16_t)(mic_buf[i] >> BIT_SHIFT);

      // Noise gate
      if (abs(sample) < NOISE_GATE) {
        spk_buf[i] = 0;
        continue;
      }

      // Volume (keep VOLUME_MULT=1 to avoid feedback)
      int32_t boosted = (int32_t)sample * VOLUME_MULT;
      if (boosted >  32767) boosted =  32767;
      if (boosted < -32768) boosted = -32768;
      spk_buf[i] = (int16_t)boosted;
    }
  }

  // ── 5. Write to speaker ──────────────────────────────
  i2s_write(I2S_NUM_0, spk_buf, samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);

  // ── 6. Level meter ───────────────────────────────────
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
    if (feedback)            status = "MUTED-FEEDBACK";
    else if (peak == 0)      status = "SILENCE       ";
    else if (peak < 5000)    status = "LOW           ";
    else if (peak < 20000)   status = "GOOD  ✓       ";
    else                     status = "HIGH          ";

    Serial.print("Level [");
    for (int b = 0; b < 30; b++) Serial.print(b < bar ? "=" : " ");
    Serial.printf("] %5d  %s\n", peak, status);
  }
}