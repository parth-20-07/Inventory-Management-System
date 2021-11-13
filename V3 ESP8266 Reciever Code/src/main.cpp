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
#include "RANDOM_ADDRESS_GENERATOR.h" //Contains all the characters needed to generate random address
#include "EEPROM_CONFIGURATION.h"     // Contains configuration files for EEPROM
#include "PIN_CONNECTION.h"
//================================================================//

//! Function Definition
int connect_to_wifi(void);
int connect_to_ntp(void);
String update_time_via_ntp(void);
void setup_nrf24l01(void);
void setup_nrf_in_writing_mode(byte transmitting_address[], char transmission_message[], size_t message_length);
String setup_nrf_in_listening_mode(byte recieving_address[]);
void upload_to_aws(String data_json);
String string_to_json_converter(String data_string);
String json_to_string_converter(String data_json);
String create_formatted_string(String raw_ntp_date_time, String data_string);
void setup_eeprom(void);
void generate_random_communication_address_and_save_to_eeprom(char box_or_reciever_code, String recieved_device_id);
void update_communication_id_array(void);
void set_reset_pin(void);
void reset_device(void);
void save_and_upload_data(String raw_time_and_date, String recieved_message);
void check_sd_module_connection(void);
void sd_data_upload_aws_offline(String raw_ntp_date_time, String data_string);
void sd_data_write_and_read_then_upload_to_aws(String formatted_string_for_sd_card);
String get_formatted_time(String raw_ntp_date_time);
String create_formatted_string(String formatted_date_time, String data_string);
void sd_card_data_write(String file_name, String formatted_data);

int wifi_connection_attempts = 0;

// ! Initial Setup
void setup()
{
        set_reset_pin();
        Serial.begin(115200);
        connect_to_ntp();
        // setup_nrf24l01();
        // check_sd_module_connection();
        // setup_eeprom();
}

// ! Looped Code
void loop()
{
        String raw_time_and_date = update_time_via_ntp();
        Serial.println("Raw Time: " + raw_time_and_date);
        get_formatted_time(raw_time_and_date);
        //        setup_nrf_in_writing_mode(BROADCAST_RECIEVER_ADDRESS, periodic_data_update_request, sizeof(periodic_data_update_request));
        //        String recieved_message = setup_nrf_in_listening_mode(BROADCAST_RECIEVER_ADDRESS);
        //        save_and_upload_data(raw_time_and_date, recieved_message);
        delay(1000);
}

//! Funtion Declaration

/**
 * @brief This funciton searches for wifi and tries connecting it.
 * If it connects to wifi then it displays "Wifi Connection success!", else it displays "Wifi Connection Error!"
 *
 * @return Int: Wifi Connection Status Flag
 */
int connect_to_wifi(void)
{
        Serial.println("\n\nAttempting to connect to Wifi");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        int connection_attempt = 0;
        while ((WiFi.status() != WL_CONNECTED) & (connection_attempt < 100))
        {
                delay(50);
                Serial.print(".");
                connection_attempt++;
        }
        Serial.println();
        switch (WiFi.status())
        {
        case 3: // WL_CONNECTED = 3: Connection Successful
                Serial.println("Wifi connected to SSID: " + String(WIFI_SSID));
                break;
        case 1: // WL_NO_SSID_AVAIL = 1: Wifi not in range
                Serial.println("Cannot Find SSID " + (String)WIFI_SSID + ", Check if router in Range!");
                break;
        case 4: // WL_CONNECT_FAILED = 4: Connection failed
                Serial.println("Wifi Connection Failed!");
                break;
        default:
                Serial.println("Unable to connect to Wifi");
                break;
        }
        wifi_connection_attempts++;
        Serial.println("COnnection Attempt: " + wifi_connection_attempts);
        if (wifi_connection_attempts > 5)
        {
                Serial.println("Resetting Device");
                reset_device();
        }
        return WiFi.status();
}

/**
 * @brief This function connects to NTP Server and return formatted time.
 *
 * @return String: Formatted Time: YYYY/MM/DD-HH:MM:SS
 */
int connect_to_ntp()
{
        int wifi_status = 0;
        Serial.println("Starting NTP Connection");
        if (WiFi.status() != WL_CONNECTED)
        { // Checks if wifi is connected
                Serial.println("Wifi disconnected");
                wifi_status = connect_to_wifi();
        }
        if (WiFi.status() == WL_CONNECTED) // Wifi Connection Successful
        {
                Serial.println("Wifi Connected");
                timeClient.begin();
        }
        else
        {
                Serial.println("Unable to connect to wifi. Time Update Failed");
                String default_time_if_ntp_fails = "2020/01/01-00:00:00"; // A default time returned if ntp fails to update. Update the time here when RTC is connected
        }
        return wifi_status;
}

/**
 * @brief Connects to NTP server and updates to latest time
 *
 * @return String: Return Time Format: YYYY/MM/DD-HH:MM:SS
 */
String update_time_via_ntp(void)
{
        Serial.println("Updating NTP Time");
        delay(50);
        if (WiFi.status() != WL_CONNECTED)
        { // Checks if wifi is connected
                Serial.println("Wifi not connected");
                connect_to_wifi();
        }
        String raw_date_and_time;
        if (WiFi.status() == WL_CONNECTED) // Wifi Connection Successful
        {
                // Updates to latest time
                for (size_t i = 0; i < 3; i++)
                { // 3 updates to avoid getting wrong time
                        timeClient.update();
                }
                raw_date_and_time = timeClient.getFormattedDate();
        }
        return raw_date_and_time;
}

/**
 * @brief Set the up nrf24l01 connection
 *
 */
void setup_nrf24l01(void)
{
        Serial.println("Setting up NRF");
        radio.begin();
        Serial.println("NRF Setup");
}

/**
 * @brief Set the up nrf in writing mode object
 *
 * @param uint64_t: transmitting_address
 * @param String: transmission_message
 */
void setup_nrf_in_writing_mode(byte transmitting_address[], char transmission_message[], size_t message_length)
{
        byte address[6] = "";
        char message[32] = "";
        for (size_t i = 0; i < 6; i++)
        {
                address[i] = transmitting_address[i];
        }
        for (size_t i = 0; i < message_length; i++)
        {
                message[i] = transmission_message[i];
        }
        radio.openWritingPipe(address);
        radio.setPALevel(RF24_PA_MIN);
        radio.stopListening();
        Serial.println(message);
        radio.write(&message, message_length);
        delay(50);
}

/**
 * @brief Set the up nrf in listening mode
 *ḍ
 * @param uint64_t: recieving_address
 * @return string: recieved message
 */
String setup_nrf_in_listening_mode(byte recieving_address[])
{
        byte address[6] = "";
        for (size_t i = 0; i < 6; i++)
        {
                address[i] = recieving_address[i];
        }
        radio.openReadingPipe(0, address);
        radio.setPALevel(RF24_PA_MIN);
        radio.startListening();
        char recieved_message[32] = "";
        while (!radio.available())
        {
        }
        radio.read(&recieved_message, sizeof(recieved_message));
        return (String)recieved_message;
}

/**
 * @brief This function checks if AWS is connected,
 * if its connected, then it uploads the data to AWS and saves it to the default folder.
 * If AWS is not connected, then the data is stored in the AWS_Backlog_file.txt
 *
 * @param String: raw_ntp_date_time
 * @param String: data_string
 */
void save_and_upload_data(String raw_time_and_date, String recieved_message)
{
        String formatted_time = get_formatted_time(raw_time_and_date);
        String file_name = formatted_time.substring(0, 9) + file_extension;
        String formatted_string_for_sd_card = create_formatted_string(formatted_time, recieved_message);
        //! if () Check AWS Connection
        int AWS_Connected = 0;
        if (AWS_Connected) // if AWS is connected
        {
                sd_data_write_and_read_then_upload_to_aws(formatted_string_for_sd_card);
                sd_card_data_write(file_name, formatted_string_for_sd_card);
        }
        else // AWS is offline
        {
                Serial.println("AWS is offline, uploading to backlog file and the default location");
                sd_card_data_write(AWS_BACKLOG_FILE, formatted_string_for_sd_card);
                sd_card_data_write(file_name, formatted_string_for_sd_card);
        }
}

/**
 * @brief Checks whether SD Card is present or not.
 *
 * @return Int: connection_status
 * Success: 1
 * Failure: 0
 */
void check_sd_module_connection(void)
{
        if (!SD.begin(chip_select))
        {
                Serial.println("SD Card failed, or not present");
        }
        Serial.println("SD Card Initialized.");
}

/**
 * @brief This function is used when the AWS is offline and we write on AWS_Backlog_file.txt
 *
 * @param String raw_ntp_date_time
 * @param String data_string_recieved_from_box
 */

/**
 * @brief This function is used when the AWS is online
 *  First the data recieved is saved into AWS_Backlog_file in write mode
 * Then the file is opened again in read mode and line by line all the previous data is converted to json
 * the json is then uploaded to aws
 *
 * @param String raw_ntp_date_time
 * @param String data_string_recieved_from_box
 */
void sd_data_write_and_read_then_upload_to_aws(String formatted_string_for_sd_card)
{
        File dataFile = SD.open(AWS_BACKLOG_FILE, FILE_READ);
        while (dataFile.available())
        {
                char data_string = dataFile.read();
                String upload_json = string_to_json_converter(formatted_string_for_sd_card);
                upload_to_aws(upload_json);
        }
        dataFile.close();            // CLose the file
        SD.remove(AWS_BACKLOG_FILE); // Remove the file after all the data is being retrieved and uploaded
}

String get_formatted_time(String raw_ntp_date_time)
{
        int current_characters[4] = {4, 7, 10, 19}; //2021-10-21T15:18:13Z
        char replacement_characters[4] = {'/', '/', '-', ' '};
        for (size_t i = 0; i < 4; i++)
        {
                raw_ntp_date_time[current_characters[i]] = replacement_characters[i];
        }
        Serial.println("Formatted Time: " + raw_ntp_date_time);
        return raw_ntp_date_time;
}

/**
 * @brief Creates the string in the required format by joining time and the data_string
 *
 * @param String: raw_ntp_date_time
 * @param String: data_string
 * @return String: Formatted String to be saved into file
 */
String create_formatted_string(String formatted_date_time, String data_string)
{
        String formatted_time = formatted_date_time.substring(11, 18);
        String formatted_string = formatted_time + data_string;
        Serial.println("Formatted String: " + formatted_string);
        return formatted_string;
}

/**
 * @brief This function opens a file and writes to it
 *
 * @param String raw_ntp_date_time
 * @param String file_name
 * @param String recieved_data
 */
void sd_card_data_write(String file_name, String formatted_data)
{
        if (file_name != AWS_BACKLOG_FILE)
        {
                String file_directory = file_name.substring(0, 7); // YYYY/MM/DD-HH:MM:SS => YYYY/MM/
                Serial.println("File Directory: " + file_directory);
                Serial.println(formatted_data);
                if (!SD.exists(file_directory))
                {
                        SD.mkdir(file_directory);
                }
        }
        File dataFile = SD.open(file_name, FILE_WRITE);
        dataFile.println(formatted_data);
        dataFile.close();
}

/**
 * @brief Uploads the data to AWS Server
 *
 * @param String: data_json
 */
void upload_to_aws(String data_json)
{
}

/**
 * @brief Converts the data from string format to json format
 *
 * @param String: data_string
 * @return String: data_json
 */
String string_to_json_converter(String data_string)
{
        String data_json = "";
        return data_json;
}

/**
 * @brief Converts the data from json format to String format
 *
 * @param String: data_json
 * @return String: data_string
 */
String json_to_string_converter(String data_json)
{
        String data_string = "";
        return data_string;
}

/**
 * @brief Set the up eeprom
 *
 */
void setup_eeprom(void)
{
        EEPROM.begin(512); // Initialize EEPROM
}

// /**
//  * @brief Generate a random 5 character address and save it in eeprom
//  * It receives an input id and box_or_reciever_code which is either "b" or "r"
//  * If box_or_reciever_code = "b": It retireves the number of boxes already registered and goes to +1 position to add the new random address
//  * If box_or_reciever_code = "r": It generates a random communication code with is used when the box is in broadcast receiver mode.
//  *
//  * @param char: box_or_reciever_code - b:box, r:reciever
//  * @param String: recieved_device_id - device ID
//  */
// void generate_random_communication_address_and_save_to_eeprom(char box_or_reciever_code, String recieved_device_id)
// {
//         int eeprom_command_position;
//         int eeprom_address_position;
//         String device_id = recieved_device_id.substring(0, 8); // Removes \n from the end
//         String communication_address[5];

//         for (int i = 0; i < 5; i++)
//         {
//                 int rand_char_pos = random(1, MAX);
//                 communication_address[i] = alphabet[rand_char_pos];
//         }

//         String save_string = recieved_device_id + ":" + communication_address; // BBBBBBBBB:CCCCC 15 character code

//         if (box_or_reciever_code == 'r') // r => Reciever address at position 1
//         {
//                 eeprom_command_position = 2;
//         }

//         else // b+> Box Adddress starts from position
//         {
//                 int registered_boxes;
//                 registered_boxes = int(EEPROM.read(eeprom_starting_address)); // Read one by one with starting address of 0x0F
//                 eeprom_command_position = 2 + (eeprom_data_length * registered_boxes) + eeprom_data_length;
//                 registered_boxes++;
//                 EEPROM.write(eeprom_starting_address, registered_boxes); // Writing zero on first position
//                 EEPROM.commit();                                         // Store data to EEPROM
//         }
//         eeprom_address_position = eeprom_starting_address + eeprom_command_position;
//         for (int i = 0; i < 15; i++)
//         {
//                 EEPROM.write(eeprom_address_position + i, save_string[i]); // Write one by one with starting address of 0x0F
//         }
//         EEPROM.commit(); // Store data to EEPROM
// }

// /**
//  * @brief Reads EEPROM byte by byte and updates the Array storing Communication ID and Box ID
//  *
//  */
// void update_communication_id_array(void)
// {
//         String eeprom_read_string;
//         saved_boxes = int(EEPROM.read(eeprom_starting_address)); // Read one by one with starting address of 0x0F
//         for (int i = 0; i < saved_boxes + 1; i++)
//         {
//                 double eeprom_position = eeprom_starting_address + (i * eeprom_data_length);
//                 for (int i = 0; i < eeprom_data_length; i++)
//                 {
//                         eeprom_read_string[i] = char(EEPROM.read(eeprom_position + i + 2)); // Read one by one with starting address of 0x0F
//                 }
//                 communication_array_device_id[i] = eeprom_read_string.substring(0, 8);          // XXXXXXXXX
//                 communication_array_communication_id[i] = eeprom_read_string.substring(10, 14); // CCCCC
//         }
// }

void set_reset_pin(void)
{
        pinMode(rst_pin, OUTPUT);
        digitalWrite(rst_pin, HIGH);
}
void reset_device(void)
{
        digitalWrite(rst_pin, LOW);
}