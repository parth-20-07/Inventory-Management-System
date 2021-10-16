// This file is meant to store the data to wonnect to wifi and the aws configuration data
#ifndef CONFIGURATION_H // In order to avoid double mentions and compiler errors
#define CONFIGURATION_H

#define WIFI_ON // In order to determine is model has Wifi turned on

//! AWS IOT config details. These are the credentials to connect with AWS and with the Wifi:
#ifdef WIFI_ON
char WIFI_SSID[] = "NRF_Wifi";
char WIFI_PASSWORD[] = "123456789";
#endif
char AWS_ENDPOINT[] = "ayru2i5e2th7e-ats.iot.us-east-2.amazonaws.com";
char AWS_KEY[] = "AKIAQTTS4PUTBSHUMA47";
char AWS_SECRET[] = "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";
char AWS_REGION[] = "ap-south-1";
const char *AWS_TOPIC = "$aws/things/ESP8266-Console/shadow/name/IoT_Core/update"; // Used to update value to AWS server
int PORT = 443;
const int MQTT_PORT = 8883;
const char MQTT_SUB_TOPIC[] = "aws/command22/123456789";
const char MQTT_PUB_TOPIC[] = "$aws/rules/123456789/aws/tel/123456789";
#endif