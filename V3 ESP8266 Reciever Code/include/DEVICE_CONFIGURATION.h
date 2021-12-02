#ifndef DEVICE_CONFIGURATION_H
#define DEVICE_CONFIGURATION_H

//! Device Configuration
#define LED_BRIGHTNESS 100 // Brightness percentage of the LED Ring

//! WebURL Configuration
String contact_us_link = "bit.ly/3nIlHXx";
String add_wifi_link = "Add Wifi";

//! Box Configuration
#define BOX_ID_LENGTH 9           // Length of the Box ID
#define MAX_BOXES 50              // Maximum Number of boxes connected
#define COMMUNICATION_ID_LENGTH 6 // Length of communication ID for NRF24
#define UPDATE_PARAMETERS 5       // Parameters recieved from Box
int connected_boxes = 0;

//! Main Screen Configuration
#define MENU_SCREEN_LIST 6            // Number of main menu items
#define MAIN_SCREEN_REFRESH_TIME 2000 // Time for main screen refresh in ms
#define ERROR_LOG_LENGTH 10           // [HH:MM #XXX] 10 digit Error Code

//! Soft AP Setup
IPAddress local_IP(192, 168, 4, 22);
IPAddress gateway(192, 168, 4, 9);
IPAddress subnet(255, 255, 255, 0);
const char *esp32_ssid = "ESP32 SSID";

//! Connection Setup
#define SSID_CHAR_LENGTH 10
#define PASSWORD_CHAR_LENGTH 20
#endif