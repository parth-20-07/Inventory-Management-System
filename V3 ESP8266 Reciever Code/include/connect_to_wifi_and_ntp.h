#ifndef connect_to_wifi_and_ntp_h
#define connect_to_wifi_and_ntp_h

#include "login_credentials.h"
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const long utcOffsetInSeconds = 19800; //+ 5:30 hr = 19800 sec

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void connect_to_wifi_and_ntp(void)
{
    WiFi.begin(wifi_ssid, wifi_password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(50);
        Serial.print(".");
    }
    timeClient.begin();
}

void update_ntp_time(void)
{
    for (size_t i = 0; i < 3; i++)
    {
        timeClient.update();
    }
    Serial.print(daysOfTheWeek[timeClient.getDay()]);
    Serial.print(", ");
    Serial.print(timeClient.getHours());
    Serial.print(":");
    Serial.print(timeClient.getMinutes());
    Serial.print(":");
    Serial.println(timeClient.getSeconds());
    Serial.println(timeClient.getFormattedTime());
    delay(1000);
}

#endif