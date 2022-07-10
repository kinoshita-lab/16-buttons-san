/**
 * @file ShiftregisterSwScanner.h
 * @brief
 *
 * @author Kazuki Saita <saita@kinoshita-lab.com>
 *
 * Copyright (c) 2022 Kinoshita Lab. All rights reserved.
 *
 */

#pragma once
#ifndef SHIFTREGISTERSWSCANNER_H
#define SHIFTREGISTERSWSCANNER_H

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <Arduino.h>

namespace kinoshita_lab {

  class ShiftregisterSwScanner {
   public:
    typedef void (*SwitchHandler)(uint32_t switch_index, const int off_on);
    enum {
      DEFAULT_NUM_SWITCH = 16,
      DEFAULT_SCAN_PERIOD_MSEC = 4,
    };

    enum InternalState {
      Init,
      LoadStart,
      ReadEachBits,
      WaitNext,
      UnknownState = 0xff,
    };

    ShiftregisterSwScanner(
        const uint8_t npl_pin, const uint8_t clock_pin,
        const uint8_t output_pin, SwitchHandler handler = nullptr,
        const size_t num_switches = DEFAULT_NUM_SWITCH,
        const unsigned int scan_period = DEFAULT_SCAN_PERIOD_MSEC)
        : pins_(npl_pin, clock_pin, output_pin),
          handler_(handler),
          num_switches_(num_switches) {
      scan_buffers_ = new uint8_t[num_switches_];
      former_scan_buffers_ = new uint8_t[num_switches_];
      switch_status_ = new uint8_t[num_switches_];

      // initialize switch status
      for (auto i = 0u; i < num_switches_; ++i) {
        scan_buffers_[i] = 1;
        former_scan_buffers_[i] = 1;
        switch_status_[i] = 1;
      }
      pinMode(npl_pin, OUTPUT);
      pinMode(clock_pin, OUTPUT);
      pinMode(output_pin, INPUT_PULLUP);

      setState(Init);
    }

    virtual ~ShiftregisterSwScanner() {
      delete[] scan_buffers_;
      delete[] former_scan_buffers_;
      delete[] switch_status_;
    }

    void update() {
      switch (status_) {
        case Init:
          setState(LoadStart);
          break;
        case LoadStart:
          setState(ReadEachBits);
          break;
        case ReadEachBits: {
          for (auto i = 0u; i < num_switches_; ++i) {
            digitalWrite(pins_.clock_pin, LOW);
            const auto read_data = digitalRead(pins_.output_pin);
            scan_buffers_[num_switches_ - i - 1] = read_data;
            digitalWrite(pins_.clock_pin, HIGH);
          }
          updateSwitchStatus();
          setState(WaitNext);
          break;
          case WaitNext: {
            const auto current = millis();
            const auto delta = current - wait_start_;
            if (delta > scan_period_) {
              setState(LoadStart);
            }
          } break;
          default:
            break;
        }
      }
    };

   protected:
    uint8_t status_ = UnknownState;
    size_t will_read_switch_ = 0;

    void setState(const int status) {
      status_ = status;
      assert(pins_.npl_pin != Pins::INVALID_PIN_CONFIGURATION);
      assert(pins_.clock_pin != Pins::INVALID_PIN_CONFIGURATION);
      assert(pins_.output_pin != Pins::INVALID_PIN_CONFIGURATION);

      switch (status_) {
        case Init:
          digitalWrite(pins_.npl_pin, HIGH);
          digitalWrite(pins_.clock_pin, LOW);
          break;
        case LoadStart:
          digitalWrite(pins_.npl_pin, LOW);
          digitalWrite(pins_.clock_pin, LOW);
          will_read_switch_ = 0;
          break;
        case ReadEachBits:
          digitalWrite(pins_.clock_pin, LOW);
          digitalWrite(pins_.npl_pin, HIGH);
          break;
        case WaitNext:
          wait_start_ = millis();
          digitalWrite(pins_.npl_pin, HIGH);
          digitalWrite(pins_.clock_pin, LOW);
          break;
        default:
          break;
      }
    }

    void updateSwitchStatus() {
      for (auto i = 0u; i < num_switches_; ++i) {
        if (scan_buffers_[i] == former_scan_buffers_[i]) {
          const auto new_status = scan_buffers_[i];
          auto current_status = switch_status_[i];
          if (current_status != new_status) {
            switch_status_[i] = (new_status);
            const auto notification_status =
                !switch_status_[i];  // NOTE: inverted!! off = HIGH, on = LOW
            if (handler_) {
              (*handler_)(i, notification_status);
            }
          }
        }
        former_scan_buffers_[i] = scan_buffers_[i];
      }
    }
    struct Pins {
      enum {
        INVALID_PIN_CONFIGURATION = 0xff,
      };
      Pins(const uint8_t npl_pin, const uint8_t clock_pin,
           const uint8_t input_pin)
          : npl_pin(npl_pin), clock_pin(clock_pin), output_pin(input_pin) {}

      Pins() = default;

      uint8_t npl_pin = INVALID_PIN_CONFIGURATION;
      uint8_t clock_pin = INVALID_PIN_CONFIGURATION;
      uint8_t output_pin = INVALID_PIN_CONFIGURATION;
    };
    Pins pins_;
    SwitchHandler handler_ = nullptr;
    uint32_t num_switches_ = DEFAULT_NUM_SWITCH;

    uint8_t * scan_buffers_ = nullptr;
    uint8_t * former_scan_buffers_ = nullptr;
    uint8_t * switch_status_ = nullptr;
    uint32_t wait_start_ = 0;
    uint32_t scan_period_ = 0;

   private:
    ShiftregisterSwScanner(const ShiftregisterSwScanner &) {}
  };

}  // namespace kinoshita_lab
#endif  // SHIFTREGISTERSWSCANNER_H
