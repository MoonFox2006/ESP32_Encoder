#include "Encoder.h"

Encoder::~Encoder() {
  stop();
  if (_queue) {
    vQueueDelete(_queue);
  }
}

bool Encoder::start() {
  if (! _started) {
    if (! _queue) { // First start
      _queue = xQueueCreate(QUEUE_SIZE, sizeof(state_t));
      if (! _queue) {
        return false;
      }
      pinMode(_clkPin, INPUT_PULLUP);
      pinMode(_dtPin, INPUT_PULLUP);
      if (_btnPin >= 0)
        pinMode(_btnPin, INPUT_PULLUP);
    }

    _btnPressed = false;
    attachInterruptArg(_clkPin, encISR, this, CHANGE);
    attachInterruptArg(_dtPin, encISR, this, CHANGE);
    if (_btnPin >= 0)
      attachInterruptArg(_btnPin, btnISR, this, CHANGE);
    _started = true;
  }
  return true;
}

void Encoder::stop() {
  if (_started) {
    if (_btnPin >= 0)
      detachInterrupt(_btnPin);
    detachInterrupt(_dtPin);
    detachInterrupt(_clkPin);
    _started = false;
  }
}

bool Encoder::read(state_t &state, uint32_t timeout) {
  if (_queue)
    return xQueueReceive(_queue, &state, timeout == FOREVER ? portMAX_DELAY : pdMS_TO_TICKS(timeout)) == pdTRUE;
  return false;
}

bool Encoder::peek(state_t &state, uint32_t timeout) {
  if (_queue)
    return xQueuePeek(_queue, &state, timeout == FOREVER ? portMAX_DELAY : pdMS_TO_TICKS(timeout)) == pdTRUE;
  return false;
}

void Encoder::reset() {
  if (_queue)
    xQueueReset(_queue);
}

void ARDUINO_ISR_ATTR Encoder::encISR(void *arg) {
  const int8_t ENC_STATES[] = {
    0, 1, -1, 0,
    -1, 0, 0, 1,
    1, 0, 0, -1,
    0, -1, 1, 0
  };

  Encoder *_this = (Encoder*)arg;

  static uint8_t ab = 0x03;
  static int8_t e = 0;

  state_t state = ENC_CLICK; // Wrong state

  ab <<= 2;
  ab |= (digitalRead(_this->_dtPin) << 1);
  ab |= digitalRead(_this->_clkPin);

  e += ENC_STATES[ab & 0x0F];
  if (e < -3) {
    state = (_this->_btnPin >= 0) && (digitalRead(_this->_btnPin) == LOW) ? ENC_CCWBTN : ENC_CCW;
    e = 0;
  } else if (e > 3) {
    state = (_this->_btnPin >= 0) && (digitalRead(_this->_btnPin) == LOW) ? ENC_CWBTN : ENC_CW;
    e = 0;
  }
  if (state != ENC_CLICK) {
    xQueueSendFromISR(_this->_queue, &state, NULL);
  }
  _this->_btnPressed = false;
}

void ARDUINO_ISR_ATTR Encoder::btnISR(void *arg) {
  Encoder *_this = (Encoder*)arg;

  static uint32_t lastTime = 0;

  if (digitalRead(_this->_btnPin)) { // Released
    if (_this->_btnPressed) { // Was pressed
      uint32_t duration = millis() - lastTime;
      state_t state = ENC_CW; // Wrong state

      _this->_btnPressed = false;
      if (duration >= LONGCLICK_TIME) {
        state = ENC_LONGCLICK;
      } else if (duration >= DEBOUNCE_TIME) {
        state = ENC_CLICK;
      }
      if (state != ENC_CW) {
        xQueueSendFromISR(_this->_queue, &state, NULL);
      }
    }
  } else { // Pressed
    _this->_btnPressed = true;
  }
  lastTime = millis();
}

EncoderCb::~EncoderCb() {
  stop();
  if (_task) {
    vTaskDelete(_task);
  }
}

bool EncoderCb::start() {
  if (! _started) {
    if (! Encoder::start())
      return false;
    if (! _task) { // First start
      if (xTaskCreate(cbTask, "cbTask", STACK_SIZE, this, TASK_PRIORITY, &_task) != pdPASS) {
        return false;
      }
    } else {
      vTaskResume(_task);
    }
  }
  return true;
}

void EncoderCb::stop() {
  if (_started) {
    if (_task) {
      vTaskSuspend(_task);
    }
    Encoder::stop();
  }
}

void EncoderCb::cbTask(void *arg) {
  EncoderCb *_this = (EncoderCb*)arg;

  while (1) {
    state_t state;

    while (_this->read(state, FOREVER)) {
      if (_this->_cb)
        _this->_cb(state);
    }
  }
}
