// This file is meant to store the data to wonnect to wifi and the aws configuration data
#ifndef login_credentials_h // In order to avoid double mentions and compiler errors
#define login_credentials_h

//! AWS IOT config details. These are the credentials to connect with AWS and with the Wifi:
char wifi_ssid[] = "project105";
char wifi_password[] = "project105";
char aws_endpoint[] = "ayru2i5e2th7e-ats.iot.us-east-2.amazonaws.com";
char aws_key[] = "AKIAQTTS4PUTBSHUMA47";
char aws_secret[] = "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";
char aws_region[] = "ap-south-1";
const char *aws_topic = "$aws/things/ESP8266-Console/shadow/name/IoT_Core/update"; // Used to update value to AWS server
int port = 443;
const int MQTT_PORT = 8883;
const char MQTT_SUB_TOPIC[] = "aws/command22/123456789";
const char MQTT_PUB_TOPIC[] = "$aws/rules/123456789/aws/tel/123456789";
#endif