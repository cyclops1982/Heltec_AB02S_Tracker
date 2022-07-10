# Heltec AB02S GPS Tracker

Simple implementation of a GPS tracker that sends an update every 5 minutes.

Please note:

- Project is created with platformio.
- Lots of hardcoded values.
- Some configured values in `platformio.ini` are not used and are hardcoded.

## Message format

We send a lora message with the following format:

| What            | Description                                                                                                                                                                                               | DataType |
| --------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------- |
| Message Version | Used by most of my LoRaMessages. A prefix that indicates Device Type (1st byte) and Message format (2nd byte).                                                                                            | 2 bytes  |
| Battery Voltage | Raw value from Battery measurement. Untested! Not sure how this calculates to an actual voltage.                                                                                                          | uint16_t |
| DateTime        | UnixTime for the date/time received by the GPS. This is in UTC.                                                                                                                                           | uint32_t |
| Latitude        | The latitude of the GPS fix. The TinyGPS provides this as a `double` which we divide by 10000000 and cast to an int32. This means you'll have to `* 10000000` on the receiving end to get the real value. | uint32_t |
| Longitude       | The longitude. See Latitude for how it's send.                                                                                                                                                            | uint32_t |
| Altitude        | The altitude of the GPS fix. This is converted from the `double` received by the TinyGPS library.                                                                                                         | int16    |
| Satelites       | Nr of satelites used to get the GPS fix. Casted from a uint32_t received from the TinyGPS library.                                                                                                        | uint8    |
| Fix Time        | Number of seconds it took to get a GPS fix.                                                                                                                                                               | uint8_t  |
