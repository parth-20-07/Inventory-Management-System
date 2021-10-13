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
#include "connect_to_wifi_and_ntp.h"
//#include "AWS_Setup.h"
void setup()
{
        //! New Code
        Serial.begin(115200);
        delay(3000);
        Serial.println("\n\n");
        Serial.println("Attempting to connect to Wifi");
        connect_to_wifi_and_ntp();
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
        update_ntp_time();
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