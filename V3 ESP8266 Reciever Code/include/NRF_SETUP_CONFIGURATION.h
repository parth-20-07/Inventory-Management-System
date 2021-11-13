#ifndef NRF_SETUP_CONFIGURATION_H
#define NRF_SETUP_CONFIGURATION_H
#include <PIN_CONNECTION.h>
//====================================================//
/*
! Connection of NRF24L01 with ESP8266

* NRF24L01 CE   ============================ D0 ESP8266
* NRF24L01 CSN  ============================ D4 ESP8266
* NRF24L01 SCK  ============================ D5 ESP8266
* NRF24L01 MISO ============================ D6 ESP8266
* NRF24L01 MOSI ============================ D7 ESP8266
*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h> //Library: TMRh20/RF24, https://github.com/tmrh20/RF24/

RF24 radio(NRF24_CE, NRF24_CSN); // CE, CSN
byte BROADCAST_RECIEVER_ADDRESS[6] = "boxit";
String communication_array_device_id[100];        // Stores Device ID after reading from eeprom
String communication_array_communication_id[100]; // Stores Communication ID after reading from eeprom
int saved_boxes;                                  // regsiters how many boxes have been linked in eeprom

#endif