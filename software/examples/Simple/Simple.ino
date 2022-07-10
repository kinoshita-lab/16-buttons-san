#include <Arduino.h>
#include "ShiftregisterSwScanner.h"

constexpr size_t NUM_SW = 16;
constexpr uint32_t PIN_SERIAL_OUT = 4;
constexpr uint32_t PIN_NPL = 5;
constexpr uint32_t PIN_CP = 6;

int buffer[NUM_SW] = {};
void onSwitchChanged(const uint32_t switch_index, const int off_on) {
  buffer[switch_index] = off_on;

  for (auto i = 0u; i < NUM_SW; ++i) {
    Serial.print(buffer[i] ? 1 : 0);
  }
  Serial.println("");
}

kinoshita_lab::ShiftregisterSwScanner swScanner(PIN_NPL, PIN_CP, PIN_SERIAL_OUT,
                                                onSwitchChanged, NUM_SW);

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
}

void loop() {
  swScanner.update();
  // delay(100);
  //  put your main code here, to run repeatedly:
}