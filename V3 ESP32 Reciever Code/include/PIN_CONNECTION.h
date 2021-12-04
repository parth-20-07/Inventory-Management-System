#ifndef PIN_CONNECTION_H
#define PIN_CONNECTION_H

//========================================================//
/*
! Pin Connections

* ESP8266 D0  =================== NRF24 CE
* ESP8266 D1  =================== I2C SCL
* ESP8266 D2  =================== I2C SDA
* ESP8266 D3  =================== DS1302 RTC CS
* ESP8266 D4  =================== NRF24 CSN
* ESP8266 D5  =================== RGB Ring D1
* ESP8266 D6  =================== Rotary Encoder CLK
* ESP8266 D7  =================== Rotary Encoder SW
* ESP8266 D8  =================== ********
* ESP8266 SCK =================== Serial Clock
* ESP8266 SD0 =================== MISO
* ESP8266 CMD =================== SD Card CS
* ESP8266 SD1 =================== MOSI
* ESP8266 SD2 =================== Rotary Encoder DT
* ESP8266 SD3 =================== Rotary Encoder I/O
*/
#define MOSI 23
#define SCL 22
#define TXD0 1
#define RXD0 3
#define SDA 21
#define MISO 19
#define SCK 18
#define NRF24_CSN 5
#define GPIO17 17
#define GPIO16 16
#define NRF24_CE 4
#define GPIO2 2
#define SD_CS 15
#define GPIO34 34
#define GPIO35 35
#define NRF24_IRQ 32
#define RE_SW 33
#define RE_DT 25
#define RE_CLK 26
#define RGB_D1 27
#define BUZZER 14
#define GPIO12 12
#define GPIO13 13

#endif