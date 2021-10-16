#ifndef EEPROM_CONFIGURATION_H
#define EEPROM_CONFIGURATION_H

#include <EEPROM.h>

//================================================//
/*
! EEPROM Data Storage Addresses:
Address 00-01  ================== "No. of boxes already registered"
Address 10-25  ================== "Reciever ID:Reciever Code"
Address 26-40  ================== "BOX 1 ID:Box 1 Code"
Address 41-55  ================== "BOX 2 ID:Box 2 Code"
.
.
.

Address x-y ================== "BOX n ID:Box n Code"
*/
//================================================//

int eeprom_starting_address = 0x0F; // Address position to save data on eeprom
int eeprom_data_length = 15;        // Length of each saved data string "<9-digit-box-id>:<5-digit-communication-code>""

#endif