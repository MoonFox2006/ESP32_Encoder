#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <Arduino.h>
#include <functional>

class Encoder {
public:
  enum state_t : uint8_t { ENC_CLICK, ENC_LONGCLICK, ENC_CW, ENC_CWBTN, ENC_CCW, ENC_CCWBTN };

  static const uint32_t FOREVER = (uint32_t)-1;

  Encoder(uint8_t clkPin, uint8_t dtPin, int8_t btnPin = -1) : _queue(nullptr), _clkPin(clkPin), _dtPin(dtPin), _btnPin(btnPin), _started(false), _btnPressed(false) {}
  ~Encoder();

  bool isStarted() const {
    return _started;
  }
  bool start();
  void stop();
  bool read(state_t &state, uint32_t timeout = 0);
  bool peek(state_t &state, uint32_t timeout = 0);
  void reset();

protected:
  static const uint16_t QUEUE_SIZE = 32;
  static const uint32_t DEBOUNCE_TIME = 50; // 50 ms.
  static const uint32_t LONGCLICK_TIME = 500; // 0.5 sec.

  static void ARDUINO_ISR_ATTR encISR(void *arg);
  static void ARDUINO_ISR_ATTR btnISR(void *arg);

  QueueHandle_t _queue;
  uint8_t _clkPin, _dtPin;
  int8_t _btnPin;
  bool _started;
  volatile bool _btnPressed;
};

class EncoderCb : public Encoder {
public:
  typedef std::function<void(state_t state)> callback_t;

  EncoderCb(uint8_t clkPin, uint8_t dtPin, int8_t btnPin = -1) : Encoder(clkPin, dtPin, btnPin), _task(nullptr), _cb(nullptr) {}
  ~EncoderCb();

  void onChange(callback_t cb) {
    _cb = cb;
  }
  bool start();
  void stop();

protected:
  static const uint32_t STACK_SIZE = 3072;
  static const UBaseType_t TASK_PRIORITY = 5;

  static void cbTask(void *arg);

  TaskHandle_t _task;
  callback_t _cb;
};
