#ifndef HTML_FILE_H
#define HTML_FILE_H
#include <Arduino.h>

const char *input_parameter1 = "input_ssid";
const char *input_parameter2 = "input_password";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
  <html>
    <head>
      <title>ESP32 Add WiFi Network</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        html {font-family: Times New Roman; display: inline-block; text-align: center;}
        h2 {font-size: 3.0rem; color: #FF0000;}
      </style>
    </head>
    
    <body>
      <h2>Add WiFi Network Details</h2> 
      <form action="/get">
        SSID: <input type="text" name="input_ssid">
        <input type="submit" value="Submit">
      </form><br>

      <form action="/get">
        Password: <input type="text" name="input_password">
        <input type="submit" value="Submit">
      </form><br>

    </body>
  </html>
)rawliteral";

#endif
