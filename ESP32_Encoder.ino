#include <Arduino.h>
#include "Encoder.h"

//#define CALLBACK_DEMO

#define ENC_CLK_PIN     3
#define ENC_DT_PIN      4
#define ENC_BTN_PIN     5

#define BOOT_BTN_PIN    9

static const char* const STATES[] = {
  "CLICK",
  "LONG CLICK",
  "CW",
  "CW + BTN",
  "CCW",
  "CCW + BTN"
};

#ifdef CALLBACK_DEMO
EncoderCb enc(ENC_CLK_PIN, ENC_DT_PIN, ENC_BTN_PIN);
#else
Encoder enc(ENC_CLK_PIN, ENC_DT_PIN, ENC_BTN_PIN);
#endif
volatile bool btnClicked = false;

static void ARDUINO_ISR_ATTR bootBtnISR() {
  const uint32_t DEBOUNCE_TIME = 100; // 100 ms.

  static uint32_t lastTime = 0;

  if (millis() - lastTime >= DEBOUNCE_TIME) {
    btnClicked = true;
  }
  lastTime = millis();
}

void setup() {
  Serial.begin(115200);

  pinMode(BOOT_BTN_PIN, INPUT_PULLUP);
  attachInterrupt(BOOT_BTN_PIN, bootBtnISR, FALLING);

#ifdef CALLBACK_DEMO
  enc.onChange([](Encoder::state_t state) {
    Serial.println(STATES[state]);
  });
#endif
  if (! enc.start()) {
    Serial.println("Encoder init error!");
    Serial.flush();
    esp_deep_sleep_start();
  }
}

void loop() {
  if (btnClicked) {
    if (enc.isStarted()) {
      enc.stop();
      Serial.println("*STOP*");
    } else {
      enc.start();
      Serial.println("*START*");
    }
    btnClicked = false;
  }
#ifndef CALLBACK_DEMO
  Encoder::state_t state;

  while (enc.read(state)) {
    Serial.println(STATES[state]);
  }
#endif
}
