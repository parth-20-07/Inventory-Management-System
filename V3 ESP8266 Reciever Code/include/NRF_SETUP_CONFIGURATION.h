#ifndef NRF_SETUP_CONFIGURATION_H
#define NRF_SETUP_CONFIGURATION_H

//====================================================//
/*
! Connection of NRF24L01 with ESP8266
NRF24L01 MISO ============================ D6 ESP8266
NRF24L01 MOSI ============================ D7 ESP8266
NRF24L01 SCK  ============================ D5 ESP8266
NRF24L01 CE   ============================ D4 ESP8266
NRF24L01 CSN  ============================ D2 ESP8266
*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>           //Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
char *send_message[32];     // Variable used to store the message to be sent
char *recieved_message[32]; // Variable used to store the recieved message

RF24 radio(4, 2); // CE, CSN
char BROADCAST_RECIEVER_ADDRESS[6] = "boxit";

#endif