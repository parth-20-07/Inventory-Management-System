/*
! References:
./
! Tasks to be completed

TODO 1. Create AWS IoT MQTT Connection
TODO 2. Create JSON to String Conversion function
TODO 3. Create String to JSON Conversion function
TODO 4. Get Lastest Time from NTP
        * Convert the data in dstring format
TODO 5. Create a random number generator for creating secret communication code
TODO 6. Save the code to EEPROM
TODO 7. Recieve Data via NRF24L01 and convert it to required parameters
TODO 8. Save Data in MicroSD Card
        * New File for every day
        * Save in Format Time,BOX-ID,C:,T:,B:,O:
TODO 9. Upload Data to AWS
        * Send it from a new file for the unuploaded data and delete it after use
        * Take the data from Memory Card and send it to AWS
        * Create a new file to save data if upload fails
*/

#include <Arduino.h>
//================================================================//
//! Including headers for basic setup
#include "CONFIGURATION.h"            // Contains SSID and password for wifi and other per client setting for AWS
#include "WIFI_SETUP_CONFIGURATION.h" // COntains libraries for wifi conneciton and basic definition
#include "NTP_SETUP_CONFIGURATION.h"  // Contains libraries for ntp connection and basic definition
#include "NRF_SETUP_CONFIGURATION.h"  //Contains libraries for NRF connection and wire connection details
#include "COMMUNICATION_PROTOCOLS.h"  //Contains all the standard communication protocols required
#include "MICROSD_CONFIGURATION.h"    // Contains all the SD Card Protocols
//================================================================//

//! Function Definition
void connect_to_ntp(void);
String update_time_via_ntp(void);
int connect_to_wifi(void);
void setup_nrf24l01(void);
void setup_nrf_in_writing_mode(uint64_t transmitting_address, char *transmission_message);
char *setup_nrf_in_listening_mode(uint64_t recieving_address);
int check_sd_module_connection(void);
void upload_data_to_sd_card(String formatted_ntp_date_time, char *data_string);
void sd_data_upload_aws_offline(String formatted_date_and_time, String data_string);
void sd_data_write_and_read_then_upload_to_aws(String formatted_date_and_time, String data_string);
void sd_card_data_write(String formatted_date_and_time, String file_name, String recieved_data);
void write_to_default_location_on_sd(String formatted_data_time, String data_string);
void upload_to_aws(String data_json);
String string_to_json_converter(String data_string);
String json_to_string_converter(String data_json);
String create_formatted_string(String formatted_date_and_time, String data_string);

// ! Initial Setup
void setup()
{
        Serial.begin(115200);
#ifdef WIFI_ON
        wifi_connection_status_flag = connect_to_wifi();
        connect_to_ntp();
        sd_card_status = check_sd_module_connection();
#endif
}

// ! Looped Code
void loop()
{
#ifdef WIFI_ON
        update_time_via_ntp();
#endif
        setup_nrf24l01();
        setup_nrf_in_writing_mode((uint64_t)BROADCAST_RECIEVER_ADDRESS, periodic_data_update_request);
        *recieved_message = setup_nrf_in_listening_mode((uint64_t)BROADCAST_RECIEVER_ADDRESS);
}

//! Funtion Declaration
/**
 * @brief This function tries to connects with ntp server
 *
 */
void connect_to_ntp(void)
{
        if (WiFi.status() != WL_CONNECTED)
        { // Checks if wifi is connected
                wifi_connection_status_flag = connect_to_wifi();
        }
        if (wifi_connection_status_flag == WL_CONNECTED) // Wifi Connection Successful
        {
                timeClient.begin();
                update_time_via_ntp();
        }
        else
        {
                Serial.println("Time Update Failed");
        }
}

/**
 * @brief Checks if wifi is connected and updates to latest time
 *
 */
String update_time_via_ntp(void)
{
        if (WiFi.status() != WL_CONNECTED)
        { // Checks if wifi is connected
                wifi_connection_status_flag = connect_to_wifi();
        }
        String raw_date_and_time;
        if (wifi_connection_status_flag == WL_CONNECTED) // Wifi Connection Successful
        {
                // Updates to latest time
                for (size_t i = 0; i < 3; i++)
                { // 3 updates to avoid getting wrong time
                        timeClient.update();
                }
                // This whole section is to convert the raw date time format from YYYY-MM-DDTHH:MM:SSZ to YYYY/MM/DD-HH:MM:SS
                raw_date_and_time = timeClient.getFormattedTime(); // Raw Time Format: YYYY-MM-DDTHH:MM:SSZ =  characters
                String unwanted_characters = "-TZ";
                String replacement_characters = "/- ";
                for (size_t j = 0; j < 2; j++) // This loop checks each of the unwanted character and replaces it with replacement character
                {
                        for (int i = 0; i < 19; i++)
                        {
                                if (raw_date_and_time[i] == unwanted_characters[j])
                                        raw_date_and_time[i] = replacement_characters[j];
                        }
                }
        }
        return raw_date_and_time;
}

/**
 * @brief This funciton searches for wifi and tries connecting it.
 * If it connects to wifi then it displays "Wifi Connection success!", else it displays "Wifi Connection Error!"
 *
 * @return int Wifi Connection Status Flag
 */
int connect_to_wifi(void)
{
        Serial.begin(115200);
        delay(3000);
        Serial.println("\n\nAttempting to connect to Wifi");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        int connection_attempt = 0;
        while ((WiFi.status() != WL_CONNECTED) & (connection_attempt < 8))
        {
                delay(50);
                Serial.print(".");
                connection_attempt++;
        }
        switch (WiFi.status())
        {
        case 3: // WL_CONNECTED = 3: Connection Successful
                Serial.println("Wifi connect success of SSID" + String(WIFI_SSID));
                break;
        case 1: // WL_NO_SSID_AVAIL = 1: Wifi not in range
                Serial.println("Cannot Find SSID, Check if router in Range!");
                break;
        case 4: // WL_CONNECT_FAILED = 4: Connection failed
                Serial.println("Wifi Connection Failed!");
                break;
        default: // Cannot find the error due to which wifi cannot be connected
                Serial.println("Wifi Connection Error! Cannot Diagnose Issue");
                break;
        }
        return WiFi.status();
}

/**
 * @brief Set the up nrf24l01 object
 *
 */
void setup_nrf24l01(void)
{
        radio.begin();
        radio.setPALevel(RF24_PA_MIN);
}

/**
 * @brief Set the up nrf in writing mode object
 *
 * @param transmitting_address
 * @param transmission_message
 */
void setup_nrf_in_writing_mode(uint64_t transmitting_address, char *transmission_message)
{
        radio.openWritingPipe(transmitting_address);
        radio.stopListening();
        radio.write(&transmission_message, sizeof(transmission_message));
        delay(50);
}

/**
 * @brief Set the up nrf in listening mode
 *
 * @param string: recieving_address
 * @return string: recieved message
 */
char *setup_nrf_in_listening_mode(uint64_t recieving_address)
{
        radio.openReadingPipe(0, recieving_address);
        radio.startListening();
        while (!radio.available())
        {
        }
        char *recieved_text[32] = {};
        radio.read(&recieved_text, sizeof(recieved_text));
        Serial.println(*recieved_text);
        return *recieved_text;
}

/**
 * @brief This function checks if AWS is connected,
 * if its connected, then it uploads the data to AWS and saves it to the default folder.
 * If AWS is not connected, then the data is stored in the AWS_Backlog_file.txt
 *
 * @return int In Backlog file = 1, In AWS File = 0
 */
void upload_data_to_sd_card(String formatted_ntp_date_time, char *data_string)
{
        //* Get the lastest NTP Time
        //! if () Check AWS Connection
        int AWS_Connected = 0;
        if (AWS_Connected) // if AWS is connected
        {
                sd_data_write_and_read_then_upload_to_aws(formatted_ntp_date_time, data_string);
                write_to_default_location_on_sd(formatted_ntp_date_time, data_string);
        }
        else // AWS is offline
        {
                sd_data_upload_aws_offline(formatted_ntp_date_time, data_string);
                write_to_default_location_on_sd(formatted_ntp_date_time, data_string);
        }

        //       return storage_location;
}

/**
 * @brief Checks whether SD Card is present or not.
 *
 * @return connection_status
 */
int check_sd_module_connection(void)
{
        if (!SD.begin(chip_select))
        {
                Serial.println("SD Card failed, or not present");
                return chip_select;
        }
        Serial.println("SD Card Initialized.");
        return chip_select;
}

void sd_data_upload_aws_offline(String formatted_date_and_time, String data_string) // This function is used when the AWS is offline and we write on AWS_Backlog_file.txt
{
        sd_card_data_write(formatted_date_and_time, AWS_BACKLOG_FILE, data_string);
}

void sd_data_write_and_read_then_upload_to_aws(String formatted_date_and_time, String data_string)
{
        sd_card_data_write(formatted_date_and_time, AWS_BACKLOG_FILE, data_string);
        File dataFile = SD.open(AWS_BACKLOG_FILE, FILE_READ);
        while (dataFile.available())
        {
                char data_string = dataFile.read();
                String upload_json = string_to_json_converter((String)data_string);
                upload_to_aws(upload_json);
        }
        dataFile.close();            // CLose the file
        SD.remove(AWS_BACKLOG_FILE); // Remove the file after all the data is being retrieved and uploaded
}

void sd_card_data_write(String formatted_date_and_time, String file_name, String recieved_data) // This function opens a file and writes to it
{
        String data_string = create_formatted_string(formatted_date_and_time, recieved_data);
        File dataFile = SD.open(AWS_BACKLOG_FILE, FILE_WRITE);
        dataFile.println(data_string);
        dataFile.close();
}

void write_to_default_location_on_sd(String formatted_data_time, String data_string)
{
        String year_directory = formatted_data_time.substring(0, 7); // YYYY/MM/DD-HH:MM:SS => YYYY/MM/
        String date = formatted_data_time.substring(8, 9);           // YYYY/MM/DD-HH:MM:SS => DD.txt
        String time_stamp = formatted_data_time.substring(11, 18);   // YYYY/MM/DD-HH:MM:SS => HH:MM:SS
        String date_file = date + file_extension;
        Serial.println(year_directory);
        Serial.println(date_file);
        if (!SD.exists(year_directory))
        {
                SD.mkdir(year_directory);
                sd_card_data_write(formatted_data_time, date_file, data_string);
        }
}

//* look for the default directory, if it doesn't exist, create a new directory and file name with today's date
//* Convert the input data from string format to json format

void upload_to_aws(String data_json)
{
}

String string_to_json_converter(String data_string)
{
        String data_json = "";
        return data_json;
}

String json_to_string_converter(String data_json)
{
        String data_string = "";
        return data_string;
}

String create_formatted_string(String formatted_date_and_time, String data_string)
{
        String formatted_string = "";
        return formatted_string;
}