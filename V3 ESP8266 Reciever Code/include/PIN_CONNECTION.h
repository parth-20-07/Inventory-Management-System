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
#define NRF24_CE D0
#define I2C_SCL D1
#define I2C_SDA D2
#define RTC_CS D3
#define NRF24_CSN D4
#define RGB_Ring_D1 D5
#define Rotary_Encoder_CLK D6
#define Rotary_Encoder_SW D7
#define Serial_Clock SCK
#define SD_Card_CS CMD
#define Rotary_Encoder_DT D9
#define RTC_IO D10
#endif