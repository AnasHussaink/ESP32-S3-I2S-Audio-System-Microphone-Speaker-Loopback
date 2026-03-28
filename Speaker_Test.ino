#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"

// Your WiFi
#define WIFI_SSID     "YourWifiname"
#define WIFI_PASSWORD "YourWifiPassword"

#define I2S_DOUT 16
#define I2S_BCLK 3
#define I2S_LRC  8

Audio audio;

void setup() {
  Serial.begin(115200);

  // Enable PSRAM
  if (!psramFound()) {
    Serial.println("PSRAM not found!");
  } else {
    Serial.printf("PSRAM found! Free: %d bytes\n", ESP.getFreePsram());
  }

  // Connect WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());

  // Setup audio
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(18);

  // Connect to stream
  audio.connecttohost("http://stream.live.vc.bbcmedia.co.uk/bbc_world_service");
}

void loop() {
  audio.loop();
}

void audio_info(const char *info) {
  Serial.printf("INFO: %s\n", info);
}

void audio_showstation(const char *info) {
  Serial.printf("Station: %s\n", info);
}

void audio_showstreamtitle(const char *info) {
  Serial.printf("Title: %s\n", info);
}