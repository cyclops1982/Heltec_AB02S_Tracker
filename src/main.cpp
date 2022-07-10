
#include <Arduino.h>
#include <LoRaWanMinimal_APP.h>
#include "configuration.h"
#include "LoraWan_config.h"
#include "GPS_Air530Z.h"
#include <time.h>

Air530ZClass GPS; // or Air530Class

static TimerEvent_t sleepTimer;
bool lowpower = false;

union Coordinates
{
  float fl;
  uint8_t uints[4];
};

void printGPSInof()
{
  Serial.print("Date/Time: ");
  if (GPS.date.isValid())
  {
    Serial.printf("%d/%02d/%02d", GPS.date.year(), GPS.date.day(), GPS.date.month());
  }
  else
  {
    Serial.print("INVALID");
  }

  if (GPS.time.isValid())
  {
    Serial.printf(" %02d:%02d:%02d.%02d", GPS.time.hour(), GPS.time.minute(), GPS.time.second(), GPS.time.centisecond());
  }
  else
  {
    Serial.print(" INVALID");
  }
  Serial.println();

  Serial.print("LAT: ");
  Serial.print(GPS.location.lat(), 6);
  Serial.print(", LON: ");
  Serial.print(GPS.location.lng(), 6);
  Serial.print(", ALT: ");
  Serial.print(GPS.altitude.meters());

  Serial.println();

  Serial.print("SATS: ");
  Serial.print(GPS.satellites.value());
  Serial.print(", HDOP: ");
  Serial.print(GPS.hdop.hdop());
  Serial.print(", AGE: ");
  Serial.print(GPS.location.age());
  Serial.print(", COURSE: ");
  Serial.print(GPS.course.deg());
  Serial.print(", SPEED: ");
  Serial.println(GPS.speed.kmph());
  Serial.println();
}

uint32_t getUnixTimeFromGPS()
{
  time_t rawtime = time(0); // we give it a dummy value to allocate the mem and such.
  struct tm *timeinfo;

  timeinfo = gmtime(&rawtime);

  /* now modify the timeinfo to the given date: */
  timeinfo->tm_year = GPS.date.year() - 1900;
  timeinfo->tm_mon = GPS.date.month() - 1; // months since January - [0,11]
  timeinfo->tm_mday = GPS.date.day();      // day of the month - [1,31]
  timeinfo->tm_hour = GPS.time.hour();     // hours since midnight - [0,23]
  timeinfo->tm_min = GPS.time.minute();    // minutes after the hour - [0,59]
  timeinfo->tm_sec = GPS.time.second();    // seconds after the minute - [0,59]

  return (uint32_t)mktime(timeinfo);
}

static uint16_t getBatteryVoltageFloat(void)
{
  float temp = 0;
  pinMode(VBAT_ADC_CTL, OUTPUT);
  digitalWrite(VBAT_ADC_CTL, LOW);

  delay(50);

  for (int i = 0; i < 50; i++)
  { // read 50 times and get average
    temp += analogReadmV(ADC);
  }
  pinMode(VBAT_ADC_CTL, INPUT);
  return (uint16_t)((temp / 50) * 2);
}

uint32_t gpsUpdate(uint32_t timeout)
{
  uint32_t starttime, fixtime = 0;
  Serial.println("GPS Searching...");

  GPS.begin();
  GPS.setmode(MODE_GLONASS);
  starttime = millis();
  while ((millis() - starttime) < timeout)
  {
    while (GPS.available() > 0)
    {
      GPS.encode(GPS.read());
    }

    // gps fixed in a second
    if (GPS.location.age() < 1000)
    {
      fixtime = millis() - starttime;
      Serial.printf("We got a GPS fix in: %d\n", fixtime);
      break;
    }
  }

  // if gps fixed update gps and print gps info every 1 second
  if (GPS.location.age() < 1000)
  {
    printGPSInof();
    GPS.end();
    return fixtime;
  }
  else // if gps no fixed waited for 2s in to lowpowermode
  {
    Serial.println("GPS search timeout.");
    GPS.end();
    return fixtime;
  }
}

void onWakeUp()
{
  Serial.printf("Woke up!\n");
  TimerStop(&sleepTimer);
  lowpower = false;
}

void setup()
{
  Serial.begin(115200);
  delay(5000);
  Serial.printf("Booting version: %s\n", VERSION);

  LoRaWAN.begin(loraWanClass, loraWanRegion);

  LoRaWAN.setAdaptiveDR(false);
  LoRaWAN.setFixedDR(5);

  while (1)
  {
    Serial.print("Joining... ");
    LoRaWAN.joinOTAA(appEui, appKey);
    if (!LoRaWAN.isJoined())
    {
      // In this example we just loop until we're joined, but you could
      // also go and start doing other things and try again later
      Serial.println("JOIN FAILED! Sleeping for 5 seconds");
      delay(5000);
    }
    else
    {
      Serial.println("JOINED");
      break;
    }
  }

  TimerInit(&sleepTimer, onWakeUp);
}

void loop()
{
  const uint8 appDataSize = 20;
  uint8_t appData[appDataSize];
  if (lowpower)
  {
    lowPowerHandler();
  }
  else
  {
    Serial.println("Gathering all data to send.");
    uint32_t fixtime = gpsUpdate(120000);
    if (fixtime > 0)
    {
      // Message version number
      appData[0] = 0x01; // First device
      appData[1] = 0x01; // v1 of the message

      uint16_t batVol1 = getBatteryVoltageFloat();
      appData[2] = batVol1 >> 8;
      appData[3] = batVol1;

      uint32_t date = getUnixTimeFromGPS();
      appData[4] = date >> (3 * 8);
      appData[5] = date >> (2 * 8);
      appData[6] = date >> (1 * 8);
      appData[7] = date;

      int32_t lat = (GPS.location.lat() * 10000000);
      appData[8] = lat >> 24;
      appData[9] = lat >> 16;
      appData[10] = lat >> 8;
      appData[11] = lat;

      int32_t lng = (GPS.location.lng() * 10000000);
      appData[12] = lng >> 24;
      appData[13] = lng >> 16;
      appData[14] = lng >> 8;
      appData[15] = lng;

      int16 altitude = (int16)GPS.altitude.meters();
      appData[16] = altitude >> 8;
      appData[17] = altitude;

      uint8 sats = (uint8)GPS.satellites.value();
      appData[18] = sats;
      appData[19] = (fixtime / 1000);

      Serial.printf("Sending data: BatVol: %d Date: %d Lat:%d Long:%d Alt: %d sats: %d Fixtime:%d\n", batVol1, date, lat, lng, altitude, sats, fixtime);

      bool sendDone = false;
      while (!sendDone)
      {
        sendDone = LoRaWAN.send(appDataSize, appData, appPort, true);
        Serial.printf("Result of send: %d\n", sendDone);
      }
      delay(100);
      TimerSetValue(&sleepTimer, 300000);
      TimerStart(&sleepTimer);
      lowpower = true;
    }
  }
}
