/**
 * @file NTP_SETUP_CONFIGURATION.h
 * @author your name (you@domain.com)
 * @brief https://microcontrollerslab.com/current-date-time-esp8266-nodemcu-ntp-server/
 * @version 0.1
 * @date 2021-10-18
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef NTP_SETUP_CONFIGURATION_H
#define NTP_SETUP_CONFIGURATION_H
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <string.h> // Used for formatting the date time to required format

//! Setting up NTP Requirements
// Define NTP Client to get time
const long utcOffsetInSeconds = 19800; //+ 5:30 hr = 19800 sec
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

#endif