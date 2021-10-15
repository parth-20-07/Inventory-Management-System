#ifndef MICROSD_CONFIGURATION_H
#define MICROSD_CONFIGURATION_H
//========================================================//
/*
! MicroSD Card Module Connection to ESP8266
MicroSD Card Module CS   ====================== D8 ESP8266
MicroSD Card Module MOSI ====================== D7 ESP8266
MicroSD Card Module SCK  ====================== D5 ESP8266
MicroSD Card Module MISO ====================== D6 ESP8266
*/
//========================================================//

#include <SPI.h>
#include <SD.h>
#include <string.h>
const int chip_select = D8;
int sd_card_status;
String AWS_BACKLOG_FILE = "AWS_Backlog_file.txt";
String file_extension = ".txt";
//========================================================//
/*
! Directory Formats

* data_log directory: Date Format to store daily logs: Year / Month / Date
* "AWS_Backlog_file.txt": To store data on a file which isn't uploaded on AWS Server

! Data Format in File

* Date,BOX-ID,C,T,B

! Process

* Write data on home with file name "AWS_Backlog_file.txt"
* When AWS is connected, data from "AWS_Backlog_file.txt" is uploaded to AWS and to data_log directory
*/
//========================================================//

#endif