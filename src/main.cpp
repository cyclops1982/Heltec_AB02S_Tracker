#include <Arduino.h>
#include "configuration.h"

static TimerEvent_t wakeUp;
bool lowpower = false;

void onWakeUp()
{
  Serial.println("Woke up!");
  TimerStop(&wakeUp);
  lowpower = false;
}

void setup()
{
  Serial.begin(9600);
  Serial.printf("Booting version: %s\n", VERSION);
  TimerInit(&wakeUp, onWakeUp);
}

void loop()
{
  Serial.println("We're in the loop. Let's sleep");
  TimerSetValue(&wakeUp, 60000);
  TimerStart(&wakeUp);
  lowPowerHandler();
}
