/*
! References:
./
! Tasks to be completed

TODO 1. Create AWS IoT MQTT Connection
TODO 2. Create JSON to String Conversion function
TODO 3. Create String to JSON Conversion function
TODO 4. Get Lastest Time from NTP
        * Convert the data in dstring format
TODO 5. Create a random number generator for creating secret communication code
TODO 6. Save the code to EEPROM
TODO 7. Recieve Data via NRF24L01 and convert it to required parameters
TODO 8. Save Data in MicroSD Card
        * New File for every day
        * Save in Format Time| BOX-ID | C: | T: | B: | O: | 
TODO 9. Upload Data to AWS
        * Send it from a new file for the unuploaded data and delete it after use
        * Take the data from Memory Card and send it to AWS
        * Create a new file to save data if upload fails
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "CONFIGURATION.h"
//#include "AWS_Setup.h"

// Define NTP Client to get time
const long utcOffsetInSeconds = 19800; //+ 5:30 hr = 19800 sec
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// ! Function Definition
void connect_to_wifi(void);
void connect_to_ntp(void);
void update_time_via_ntp(void);

void setup()
{
//! New Code
#ifdef WIFI_ON
        connect_to_wifi();
        connect_to_ntp();
#endif
        //! Old Code
        // NTPConnect();
        // net.setCACert(cacert);
        // net.setCertificate(client_cert);
        // net.setPrivateKey(privkey);

        // client.begin(MQTT_HOST, MQTT_PORT, net);
        // client.onMessage(messageReceived);

        // connectToMqtt();

        // /////////////////////////////////////////////////////
        // display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
        // display.clearDisplay();
        // OLED_print("WELCOME ", "TO", "BOXIT");
}

//String serial_msg;

void loop()
{
#ifdef WIFI_ON
        update_time_via_ntp();
#endif
        // now = time(nullptr);
        // if (!client.connected())
        // {
        //     checkWiFiThenMQTT();
        //     //checkWiFiThenMQTTNonBlocking();
        //     //checkWiFiThenReboot();
        // }
        // else
        // {
        //     client.loop();
        //     if (Serial.available())
        //     {
        //         serial_msg = Serial.readString();
        //         OLED_sprint(serial_msg + "dtime:" + get_time_format());
        //         sendData(serial_msg);
        //         delay(2000);
        //         lastMillis = millis();
        //     }

        //     OLED_print("MSG Sent", String((millis() - lastMillis) / 1000) + "sec", "AGO");
        // }
}
void connect_to_wifi(void)
{ /**
 * @brief This funciton searches for wifi and tries connecting it.
 * If it connects to wifi then it displays "Wifi Connection success!", else it displays "Wifi Connection Error!"
 * 
 */
        Serial.begin(115200);
        delay(3000);
        Serial.println("\n\nAttempting to connect to Wifi");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        int connection_attempt = 0;
        while ((WiFi.status() != WL_CONNECTED) & (connection_attempt < 8))
        {
                delay(50);
                Serial.print(".");
                connection_attempt++;
        }
        switch (WiFi.status())
        {
        case 3: //WL_CONNECTED = 3: Connection Successful
                Serial.println("Wifi connect success of SSID" + String(WIFI_SSID));
                break;
        case 1: //WL_NO_SSID_AVAIL = 1: Wifi not in range
                Serial.println("Cannot Find SSID, Check if router in Range!");
                break;
        case 4: //WL_CONNECT_FAILED = 4: Connection failed
                Serial.println("Wifi Connection Failed!");
                break;
        default: // Cannot find the error due to which wifi cannot be connected
                Serial.println("Wifi Connection Error! Cannot Diagnose Issue");
                break;
        }
}

void connect_to_ntp()
{
        timeClient.begin();
}

void update_time_via_ntp()
{
        for (size_t i = 0; i < 3; i++)
        {
                timeClient.update();
        }
        Serial.print(daysOfTheWeek[timeClient.getDay()]);
        Serial.print(", ");
        Serial.println(timeClient.getFormattedTime());
        delay(1000);
}
