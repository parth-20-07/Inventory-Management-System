/**
 * @file main.cpp
 * @author Parth Pate; (parth.pmech@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021-11-23
 * @copyright Copyright (c) 2021
 */

//! Include Header Files
#include <Arduino.h>
#include "PIN_CONNECTION.h"
#include "CONFIGURATION.h"

//! WiFi and NTP Setup
//Reference (NTP Setup): https://lastminuteengineers.com/esp32-ntp-server-date-time-tutorial/
#include <WiFi.h>
#include "time.h"
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
const char *ntpServer = "asia.pool.ntp.org";
const long gmtOffset_sec = 19800; // +5:30 = (5*60*60) + (30*60) = 19800
const int daylightOffset_sec = 0; // India doesn't observe DayLight Saving
uint16_t year;
uint8_t month;
uint8_t date;
uint8_t hour;
uint8_t minutes;
uint8_t seconds;

//! RTC DS3231 Setup
//Reference (RTC DS3231): https://how2electronics.com/esp32-ds3231-based-real-time-clock/
//Reference (RTCLib Library): //https://adafruit.github.io/RTClib/html/index.html/
#include <SPI.h>
#include <Wire.h>
#include "RTClib.h"
RTC_DS3231 rtc;

//! OLED Setup
//Reference (OLED): https://randomnerdtutorials.com/esp32-ssd1306-oled-display-arduino-ide/
//Reference (Display LOGO on OLED): Reference: https://create.arduino.cc/projecthub/Arnov_Sharma_makes/displaying-your-own-photo-on-oled-display-5a8e8b
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "COMPANY_LOGO.c"
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define MAIN_MENU_LIST 21
#define SUB_MENU_LEVEL 2
#define SUB_MENU_LIST 7
#define OLED_MENU_TEXT_SIZE 2
String main_menu[MAIN_MENU_LIST][SUB_MENU_LEVEL][SUB_MENU_LIST] = {
    {{"< Back"}, {}},
    {{"Item 01"}, {"< Back", "1.1", "1.2", "1.3", "1.4", "1.5", "1.6"}},
    {{"Item 02"}, {"< Back", "2.1", "2.2", "2.3", "2.4", "2.5", "2.6"}},
    {{"Item 03"}, {"< Back", "3.1", "3.2", "3.3", "3.4", "3.5", "3.6"}},
    {{"Item 04"}, {"< Back", "4.1", "4.2", "4.3", "4.4", "4.5", "4.6"}},
    {{"Item 05"}, {"< Back", "5.1", "5.2", "5.3", "5.4", "5.5", "5.6"}},
    {{"Item 06"}, {"< Back", "6.1", "6.2", "6.3", "6.4", "6.5", "6.6"}},
    {{"Item 07"}, {"< Back", "7.1", "7.2", "7.3", "7.4", "7.5", "7.6"}},
    {{"Item 08"}, {"< Back", "8.1", "8.2", "8.3", "8.4", "8.5", "8.6"}},
    {{"Item 09"}, {"< Back", "9.1", "9.2", "9.3", "9.4", "9.5", "9.6"}},
    {{"Item 10"}, {"< Back", "10.1", "10.2", "10.3", "10.4", "10.5", "10.6"}},
    {{"Item 11"}, {"< Back", "11.1", "11.2", "11.3", "11.4", "11.5", "11.6"}},
    {{"Item 12"}, {"< Back", "12.1", "12.2", "12.3", "12.4", "12.5", "12.6"}},
    {{"Item 13"}, {"< Back", "13.1", "13.2", "13.3", "13.4", "13.5", "13.6"}},
    {{"Item 14"}, {"< Back", "14.1", "14.2", "14.3", "14.4", "14.5", "14.6"}},
    {{"Item 15"}, {"< Back", "15.1", "15.2", "15.3", "15.4", "15.5", "15.6"}},
    {{"Item 16"}, {"< Back", "16.1", "16.2", "16.3", "16.4", "16.5", "16.6"}},
    {{"Item 17"}, {"< Back", "17.1", "17.2", "17.3", "17.4", "17.5", "17.6"}},
    {{"Item 18"}, {"< Back", "18.1", "18.2", "18.3", "18.4", "18.5", "18.6"}},
    {{"Item 19"}, {"< Back", "19.1", "19.2", "19.3", "19.4", "19.5", "19.6"}},
    {{"Item 20"}, {"< Back", "20.1", "20.2", "20.3", "20.4", "20.5", "20.6"}},
};

//! NRF24 Setup
//Reference (NRF24): https://howtomechatronics.com/tutorials/arduino/arduino-wireless-communication-nrf24l01-tutorial/
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(NRF24_CE, NRF24_CSN);
const int AVAILABLE_CHARACTERS = 62;
char alphabet[AVAILABLE_CHARACTERS] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U',
    'V', 'W', 'X', 'Y', 'Z', 'a', 'b',
    'c', 'd', 'e', 'f', 'g', 'h', 'i',
    'j', 'k', 'l', 'm', 'n', 'o', 'p',
    'q', 'r', 's', 't', 'u', 'v', 'w',
    'x', 'y', 'z', '1', '2', '3', '4',
    '5', '6', '7', '8', '9', '0'};
char BROADCAST_RECIEVER_ADDRESS[5] = {'b', 'o', 'x', 'i', 't'};
String BOX_DETAILS[MAX_BOXES][2];

//! SD Card Setup
//Reference (SD Card Module): https://randomnerdtutorials.com/esp32-microsd-card-arduino/
//Reference: (ReadLines Library): https://github.com/mykeels/ReadLines/blob/master/examples/print-line-and-index/print-line-only.ino
#include "FS.h"
#include "SD.h"
#include <ReadLines.h>
const char AWS_BACKLOG_FILE[] = "AWS_Backlog_file.txt";
const char NRF_COMMUNICATION_ADDRESS_FILE[] = "NRF_Communication_file.txt";
char BOX_ID_DETAILS_FILE[] = "Box_ID_Details.txt";
char file_extension[] = ".txt";
char line1[RL_MAX_CHARS];

//! Encoder Setup
//Reference (Encoder): https://howtomechatronics.com/tutorials/arduino/rotary-encoder-works-use-arduino/
#define outputA RE_DT
#define outputB RE_CLK
#define outputS RE_SW
#define DEBOUNCE_TIME 50
float counter = 0;
int aState;
int aLastState;

//! WS2812 Neopixel Ring
//Reference (WS2812 NeoPixel LED Ring): https://create.arduino.cc/projecthub/robocircuits/neopixel-tutorial-1ccfb9
#include <Adafruit_NeoPixel.h>
#define NUMPIXELS 8 //Number of LEDs on NeoPixel
Adafruit_NeoPixel pixels(NUMPIXELS, RGB_D1, NEO_GRB + NEO_KHZ800);
#define RED_COLOR pixels.Color(255, 0, 0)
#define GREEN_COLOR pixels.Color(0, 255, 0)
#define BLUE_COLOR pixels.Color(0, 0, 255)
#define YELLOW_COLOR pixels.Color(255, 255, 0)

//! AWS Setup
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h>
WiFiClientSecure net;
MQTTClient client;

//! Setting Up Timers
/**
* Reference (Timer Alarm): https://diyprojects.io/esp32-timers-alarms-interrupts-arduino-code/#.YZjLhaJBzDe
*@brief 
*Timer of 5 mins needs to be made
*That is 5 * 60 sec = 300 sec.
*Prescalar possible : 
* -2 ^ 8 = 256 = 4 mins 16 secs -> We are choosing this option as this is closest to the required timer
* -2 ^ 9 = 512 = 8 mins 32 secs *
*/
#define DELAY_IN_MINUTES 5          // Time between each counting
#define TIMER DELAY_IN_MINUTES * 60 // Delay in Seconds
#define ESP32_CLOCK 80000000        // 80MHz Clock on ESP32
#define PRESCALAR 80                // Prescalar creates a timer to set the alarm for 4 min 16 sec
hw_timer_t *timer = NULL;
/**
 * @brief The count variable increments everytime the timer overflows,
 * When count > timer delay, the polling of boxes is done.
 */
volatile int count;

//! Setting up EEPROM
/**
 * @brief We are using EEPROM to store the number of boxes connected with the reciever
 * Reference (EEPROM): https://randomnerdtutorials.com/esp32-flash-memory/
 */
#include <EEPROM.h>
#define EEPROM_SIZE 1
#define CONNECTED_BOXES_STORED_LOCATION 0

//! Function Definition
void configure_timer(void);
void IRAM_ATTR onTime(void);
void pin_setup(void);
void wifi_ntp_setup(void);
void rtc_setup(void);
void oled_setup(void);
void sd_setup(void);
void rgb_ring_setup(void);
void aws_setup(void);
void print_company_logo(void);
void update_oled(String text1, String text2, String text3);
void oled_menu_update(String item1, String item2, String item3);
void printLocalTime(void);
void rtc_get_time(void);
void solid_rgb_ring(uint32_t color);
void halt_rgb_ring(int led_number);
void createDir(fs::FS &fs, const char *path);
void readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void read_rotary_encoder(void);
void add_new_box(String box_id);
void calibrate_box(String box_id, String box_address);
void update_box_data(String box_id, String box_address);
void change_box_setting(String box_id, String box_address, String dt, String st, String bt);
void sound_buzzer(String box_id, String box_address);
void write_radio(char transmission_address[], String transmission_message);
String read_radio(char recieving_address[]);
void regular_box_update(void);
String fetch_box_address(String box_id);
void read_box_details_from_sd_card(void);
void handleEachLine(char line[], int lineIndex);
void messageReceived(String &topic, String &payload);
void lwMQTTErr(lwmqtt_err_t reason);
void lwMQTTErrConnection(lwmqtt_return_code_t reason);
void connectToMqtt(bool nonBlocking);
void connectToWiFi(String init_str);
void checkWiFiThenMQTT(void);
void checkWiFiThenMQTTNonBlocking(void);
void checkWiFiThenReboot(void);
void send_Success_Data(String box_id, String command, int success_status, String param1, String param2);
void sendData(String box_id, String message);

//! Function Declaration

//TODO: Timer Functions
/**
 * @brief Configuring Timer for Alarm.
 */
void configure_timer(void)
{
    Serial.println("Configuring Timers");
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Setting", "Up", "Timer");

    timer = timerBegin(0, PRESCALAR, true);
    timerAttachInterrupt(timer, &onTime, true);
    timerAlarmWrite(timer, (ESP32_CLOCK / PRESCALAR), true);

    solid_rgb_ring(GREEN_COLOR);
    update_oled("Timer", "Setup", "Done");
    return;
}

/**
 * @brief This Funtion is activated when the timer overflows. Each time the timer overflows,
 * the count variable increments.
 */
void IRAM_ATTR onTime(void)
{
    count++;
    return;
}

//TODO: Basic Setup for the code
/**
 * @brief Setting up the pins INPUT/OUTPUT
 */
void pin_setup(void)
{
    pinMode(outputA, INPUT);
    pinMode(outputB, INPUT);
    pinMode(outputS, INPUT);
    return;
}

/**
 * @brief Setup WiFi and NTP
 */
void wifi_ntp_setup(void)
{
    //connect to WiFi
    Serial.println("Setting up WiFi and NTP");
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Wifi", "Connecting", ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(50);
        Serial.print(".");
    }
    Serial.println("CONNECTED");
    solid_rgb_ring(GREEN_COLOR);
    update_oled("Wifi", "Connected", ssid);
    delay(500);

    // Configure NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    rtc_setup();
    printLocalTime();
    return;
}

/**
 * @brief Setup RTC
 */
void rtc_setup(void)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("RTC", "Connection", "In Progress");
    Serial.println("Setting up RTC");

    if (!rtc.begin())
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("RTC", "Connection", "Failed");
        Serial.println("Couldn't find RTC");
    }

    Serial.println("RTC Setup Done");
    solid_rgb_ring(GREEN_COLOR);
    update_oled("RTC", "Setup", "Done");
    return;
}

/**
 * @brief Setting Up OLED Module
 */
void oled_setup(void)
{
    Serial.println("Setting up OLED");
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
        Serial.println("SSD1306 allocation failed");

    display.clearDisplay();

    print_company_logo();
    delay(500);
    solid_rgb_ring(RED_COLOR);
    delay(500);
    solid_rgb_ring(YELLOW_COLOR);
    delay(500);
    solid_rgb_ring(GREEN_COLOR);
    delay(500);
    return;
}

/**
 * @brief Setup SD Card
 * 
 */
void sd_setup(void)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Setting", "up", "MicroSD");
    Serial.println("Setting up SD Card");
    if (!SD.begin(SD_CS))
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("MicroSD", "Mount", "Failed");
        Serial.println("Card Mount Failed");
        return;
    }
    solid_rgb_ring(GREEN_COLOR);
    update_oled("MicroSD", "Setup", "Done");
    return;
}

/**
 * @brief Setting up the NeoPixels LED Ring
 */
void rgb_ring_setup(void)
{
    Serial.println("Setting up RGB Ring");
    pixels.begin();
    pixels.setBrightness(BRIGHTNESS * 255 / 100);
    return;
}

/**
 * @brief Setup AWS
 * 
 */
void aws_setup(void)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("AWS", "Setup", "In Progress");
    Serial.println("Setting up AWS");
    net.setCACert(cacert);
    net.setCertificate(client_cert);
    net.setPrivateKey(privkey);
    client.begin(MQTT_HOST, MQTT_PORT, net);
    client.onMessage(messageReceived);
    connectToMqtt(false);
    solid_rgb_ring(GREEN_COLOR);
    update_oled("AWS", "Setup", "Complete");
}

//TODO: OLED Functions

/**
 * @brief Print Company Logo on OLED and turn the switch LED Ring to BLUE Color
 */
void print_company_logo(void)
{
    Serial.println("Printing Company Logo");

    display.clearDisplay();
    display.drawBitmap(0, 0, COMPANY_LOGO, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    display.display();

    solid_rgb_ring(BLUE_COLOR);
    return;
}

/**
 * @brief Display String on OLED
 * 
 * @param text1 Text on Line 1
 * @param text2 Text on Line 2
 * @param text3 Text on Line 3
 */
void update_oled(String text1, String text2, String text3)
{
    Serial.println("Updating OLED to:\n" + text1 + '\t' + text2 + '\t' + text3);

    display.clearDisplay();
    display.setTextSize(OLED_MENU_TEXT_SIZE);
    display.setTextColor(WHITE);

    display.setCursor(0, 0);
    display.println(text1);

    display.setCursor(0, SCREEN_HEIGHT / 3);
    display.println(text2);

    display.setCursor(0, 2 * SCREEN_HEIGHT / 3);
    display.println(text3);

    display.display();
    delay(1000);
    return;
}

/**
 * @brief Display Menu Items on the OLED
 * 
 * @param item1 Menu Item 1 on Line 1
 * @param item2 Menu Item 2 on Line 2
 * @param item3 Menu Item 3 on Line 3
 */
void oled_menu_update(String item1, String item2, String item3)
{
    display.clearDisplay();

    display.setTextSize(OLED_MENU_TEXT_SIZE);
    display.setTextColor(WHITE);

    display.setCursor(0, 0);
    display.println("-");

    display.setCursor(15, 0);
    display.println(item1);

    display.setCursor(15, SCREEN_HEIGHT / 3);
    display.println(item2);

    display.setCursor(15, 2 * SCREEN_HEIGHT / 3);
    display.println(item3);

    display.display();
    return;
}

//TODO: NTP Functions
/**
 * @brief Tries to get local time from NTP:
 * -> If it's successful, The year, month, date, hour, minutes, seconds is updated to global variables with time from NTP,
 * and the time is saved to RTC.
 * -> If it fails, the fuction is called rtc_get_time()
 */
void printLocalTime(void)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Updating", "NTP", "");
    Serial.println("Fetching Latest Time from NTP");

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("NTP", "Update", "Failed");
        Serial.println("NTP Failed to Update");
        rtc_get_time();
        return;
    }

    year = timeinfo.tm_year - 100;
    month = timeinfo.tm_mon + 1;
    date = timeinfo.tm_mday;
    hour = timeinfo.tm_hour;
    minutes = timeinfo.tm_min;
    seconds = timeinfo.tm_sec;

    solid_rgb_ring(GREEN_COLOR);
    update_oled("NTP", "Update", "Success");
    String day = (String)year + '/' + (String)month + '/' + (String)date;
    String time = (String)hour + ':' + (String)minutes + ':' + (String)seconds;
    Serial.println("Time from NTP:\n" + day + ' ' + time);
    update_oled("NTP Time", day, time);
    delay(1000);

    solid_rgb_ring(YELLOW_COLOR);
    update_oled("RTC", "Update", "In Progress");
    rtc.adjust(DateTime(year, month, date, hour, minutes, seconds));

    solid_rgb_ring(GREEN_COLOR);
    update_oled("RTC", "Update", "Successful");
    return;
}

//TODO: RTC Functions
/**
 * @brief Fetch Time from RTC and save it in global time variables
 * 
 */
void rtc_get_time(void)
{
    Serial.println("Fetching Time from RTC");
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("RTC", "Update", "In Progress");

    DateTime now = rtc.now();
    solid_rgb_ring(GREEN_COLOR);
    Serial.println("RTC Fetch Successful");
    update_oled("RTC", "Fetch", "Successful");

    year = now.year();
    month = now.month();
    seconds = now.second();
    hour = now.hour();
    minutes = now.minute();
    seconds = now.second();

    String day = (String)year + '/' + (String)month + '/' + (String)date;
    String time = (String)hour + ':' + (String)minutes + ':' + (String)seconds;
    Serial.println("Time from RTC:\n" + day + ' ' + time);
    update_oled("RTC Time", day, time);
    delay(1000);

    solid_rgb_ring(GREEN_COLOR);
    update_oled("RTC", "Update", "Successful");
    return;
}

//TODO: RGB NeoPixel Functions
/**
 * @brief Setting the RGB Ring Light to a solid color
 * 
 * @param color pixels.Color(R:0-255, G:0-255, B:0-255)
 */
void solid_rgb_ring(uint32_t color)
{
    Serial.println("Setting up RGB Color");
    pixels.clear();
    for (size_t i = 0; i < NUMPIXELS; i++)
        pixels.setPixelColor(i, color);
    pixels.show();
    return;
}

/**
 * @brief Setting the LED Ring light one led at time to display progress bar on the pixel ring.
 * 
 * @param led_number LED Ring Number
 */
void halt_rgb_ring(int led_number)
{
    pixels.clear();
    led_number = led_number % NUMPIXELS;
    pixels.setPixelColor(led_number, YELLOW_COLOR);
    pixels.show();
    return;
}

//TODO: MicroSD Card Functions
/**
 * @brief Create a Directory on SD Card
 * 
 * @param fs SD Object
 * @param path The Location Path of the folder
 */
void createDir(fs::FS &fs, const char *path)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Creating", "Dir. in", "MicroSD");
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path))
    {
        Serial.println("Dir created");
        solid_rgb_ring(GREEN_COLOR);
        update_oled("Dir.", "Create", "Successful");
    }
    else
    {
        Serial.println("mkdir failed");
        solid_rgb_ring(RED_COLOR);
        update_oled("Dir.", "Create", "Failed");
    }
    return;
}

/**
 * @brief Read a file on SD Card
 * 
 * @param fs SD Object
 * @param path The Location path of the file
 * @return int Exit Status
 */
void readFile(fs::FS &fs, const char *path)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Reading", "File from", "MicroSD");
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        solid_rgb_ring(RED_COLOR);
        update_oled("File", "Open", "Failed");
        return;
    }

    Serial.print("Read from file: ");
    while (file.available())
        Serial.write(file.read());
    file.close();
    solid_rgb_ring(GREEN_COLOR);
    update_oled("File", "Read", "Successful");
    return;
}

/**
 * @brief Write to a file on SD Card
 * 
 * @param fs SD Object
 * @param path Path of the file
 * @param message Message to be written in the file
 */
void writeFile(fs::FS &fs, const char *path, const char *message)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Writing", "File to", "MicroSD");
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("File", "Not", "Available");
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        solid_rgb_ring(GREEN_COLOR);
        update_oled("File", "Write", "Successful");
        Serial.println("File written");
    }
    else
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("File", "Write", "Failed");
        Serial.println("Write failed");
    }
    file.close();
    return;
}

/**
 * @brief Append the message to the file
 * -> If file doesn't exist, then a new file is created and then written to it.
 * 
 * @param fs SD Object
 * @param path Path of the file
 * @param message Message to be written to the file
 */
void appendFile(fs::FS &fs, const char *path, const char *message)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Appending", "File to", "MicroSD");
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("File", "Not", "Available");
        Serial.println("Failed to open file for appending");
        writeFile(SD, path, message);
        return;
    }
    if (file.print(message))
    {
        solid_rgb_ring(GREEN_COLOR);
        update_oled("Message", "Append", "Successful");
        Serial.println("Message appended");
    }
    else
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("Message", "Append", "Failed");
        Serial.println("Append failed");
    }
    file.close();
    return;
}

//TODO: Rotary Encoder Functions
/**
 * @brief Reads the Rotary Encoder and Update the OLED to interface the menu
 * After selecting the menu item, the respective function is executed.
 * The menu interface works in the below described method:
 * The menu is 2 levelled menu. The main menu and the sub menu.
 * Initially, the main menu is been scrolled through via oled, when the switch is pressed on rotary encoder,
 * the sub menu for the respective primary menu is selected.
 */
void read_rotary_encoder(void)
{
    Serial.println("Enabling Rotary Encoder");

    // 3 items to display on the oled
    String item1 = main_menu[0][0][0];
    String item2 = main_menu[1][0][0];
    String item3 = main_menu[2][0][0];
    oled_menu_update(item1, item2, item3);

    int main_list_pos = 0;            // Position of the cursor on the main menu
    int sub_list_pos = 0;             // Position of the cursor on the sub list
    int menu_level = 1;               // Level of the cursor on the menu level
    int menu_item = 0;                // The value of the selected menu item
    int menu_length = MAIN_MENU_LIST; // Variable to store the number of items at the current menu level

    aLastState = digitalRead(outputA);

    while (1)
    {
        item1 = "";
        item2 = "";
        item3 = "";
        aState = digitalRead(outputA); // Reads the "current" state of the outputA

        if (aState != aLastState) // If the previous and the current state of the outputA are different, that means a Pulse has occured
        {
            if (digitalRead(outputB) != aState) // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
                counter += 0.5;                 // increment of 0.5 is done to ensure that the the complete click of RE is measured as one turn
            else
                counter -= 0.5; // decrement of 0.5 is done to ensure that the the complete click of RE is measured as one turn
            if ((int)counter >= menu_length - 1)
                counter = menu_length - 1; // Ensures that the menu list does not go below the coded list to avoid menu display error
            else if ((int)counter < 0)
                counter = 0; // Ensures that the menu list does not go above the coded list to avoid menu display error
            if (menu_level == 1)
            {
                menu_length = MAIN_MENU_LIST; // Setting menu length equals to main menu list contents
                main_list_pos = (int)counter; // Setting counter to the main_list_pos item
                item1 = main_menu[main_list_pos][0][0];
                if (main_list_pos < (menu_length - 1))
                    item2 = main_menu[main_list_pos + 1][0][0];
                if (main_list_pos < menu_length - 2)
                    item3 = main_menu[main_list_pos + 2][0][0];
                menu_item = 10 * (int)counter; // Updating Menu Item.
            }
            else if (menu_level == 2)
            {
                sub_list_pos = (int)counter; // Setting counter to the sub_list_pos item
                item1 = main_menu[main_list_pos][1][sub_list_pos];
                if (sub_list_pos < (menu_length - 1))
                    item2 = main_menu[main_list_pos][1][sub_list_pos + 1];
                if (sub_list_pos < (menu_length - 2))
                    item3 = main_menu[main_list_pos][1][sub_list_pos + 2];
                menu_item += (int)counter;
            }
            oled_menu_update(item1, item2, item3);
            aLastState = aState; // Updates the previous state of the outputA with the current state
        }
        if (digitalRead(outputS) == HIGH)
        {
            delay(DEBOUNCE_TIME);
            if (digitalRead(outputS) == HIGH)
            {
                if ((int)counter == 0)
                {
                    main_list_pos = 0;
                    item1 = main_menu[0][0][0];
                    item2 = main_menu[1][0][0];
                    item3 = main_menu[2][0][0];
                    oled_menu_update(item1, item2, item3);
                    counter = 0;
                    if (menu_level == 1)
                        break; // Jumping back to main screen
                    else if (menu_level == 2)
                        menu_level = 1; // Jumping to main menu
                }
                else
                {
                    if (menu_level == 1)
                    {
                        menu_length = SUB_MENU_LIST;
                        item1 = main_menu[(int)counter][1][0];
                        item2 = main_menu[(int)counter][1][1];
                        item3 = main_menu[(int)counter][1][2];
                        oled_menu_update(item1, item2, item3);
                        counter = 0;
                        menu_level++;
                    }
                    else
                    {
                        switch (menu_item)
                        {
                        case 11:
                            /* code */
                            break;

                        case 12:
                            /* code */
                            break;

                        case 13:
                            /* code */
                            break;

                        case 14:
                            /* code */
                            break;

                        case 15:
                            /* code */
                            break;

                        default:
                            break;
                        }
                    }
                }
                delay(500);
            }
        }
    }
    solid_rgb_ring(BLUE_COLOR);
    return;
}

//TODO: Special Protocols
/**
 * @brief Protocol to add new box as per AWS's recieved command
 * The box is added as per the process listed below:
 * 1. AWS sends the box ID to be sent
 * 2. A random address is generated for the purpose of communication.
 * 3. Reciever broadcasts a message at the address "boxit" with message "pair,<box_id>,<random_generated_communication_address>"
 * 4. Change the reciever to the address generated for the box.
 * 5. Wait for the reciever to reply with the message "pair,ok,<box_id>"
 * 5.1 If message recieved is not same as mentioned, then connection status flag = 0 and send_success_data(<box_id>,add,connection_status) is called.
 * 5.2 If message recieved is same as mentioned:
 * 5.2.1 EEPROM is called where the total connected box data is available and the data is saved in connected_box variable.
 * 5.2.2 connected_box variable is incremented by one and then saved again to the EEPROM.
 * 5.2.3 The String in format <box_id><communicaiton_address> is saved in the SD Card
 * 5.2.4 Box_ID_Array is updated with the newly added box.
 * 5.2.5 AWS is notified by calling send_success_data(<box_id>,add,connection_status,<communication_address>)
 * 
 * @param box_id
 */
void add_new_box(String box_id)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Adding", "New Box", box_id);

    // Generate Random Address for Communication
    srand(time(0));
    char address[COMMUNICATION_ID_LENGTH];
    for (int i = 0; i < COMMUNICATION_ID_LENGTH; i++)
        address[i] = alphabet[rand() % (AVAILABLE_CHARACTERS - 1)];

    // Write the message in format "pair,<box_id>,<communication_address>"
    String message = "pair," + box_id + "," + (String)address;
    write_radio(BROADCAST_RECIEVER_ADDRESS, message);

    // Wait for the Box to return a message
    String recieved_message = read_radio(address);
    int connection_status = 0; // 0 -> Failure, 1 -> Success

    // Check if the recieved message is in format "pair,ok,<box_id>"
    if (recieved_message = ("pair,ok," + (String)box_id))
    { // TRUE
        connection_status = 1;

        // Reading the data from EEPROM about previously connected boxes
        connected_boxes = EEPROM.read(CONNECTED_BOXES_STORED_LOCATION);
        connected_boxes++;
        EEPROM.write(CONNECTED_BOXES_STORED_LOCATION, connected_boxes);
        EEPROM.commit();

        // Creating char array where the data in format<box_id><box_communication_address> is saved
        char sd_message[BOX_ID_LENGTH + COMMUNICATION_ID_LENGTH];
        for (int i = 0; i < (BOX_ID_LENGTH + COMMUNICATION_ID_LENGTH); i++)
        {
            if (i < BOX_ID_LENGTH)
                sd_message[i] = box_id[i];
            else
                sd_message[i] = address[i - BOX_ID_LENGTH];
        }

        appendFile(SD, BOX_ID_DETAILS_FILE, sd_message); // Appending the char array in SD Card
        read_box_details_from_sd_card();                 // Updating the box data and saving the new box in Box ID Array
        solid_rgb_ring(GREEN_COLOR);
        update_oled("Added", "New Box", box_id);
    }
    else // FALSE
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("Failed", "New Box", box_id);
    }

    // Uploading the data in AWS
    if (!client.connected())
        checkWiFiThenMQTT();
    else
    {
        client.loop();
        send_Success_Data((String)box_id, "add", connection_status, (String)address, "");
    }
    return;
}

/**
 * @brief Calibrate the Box
 * Box is calibrated in the following method:
 * 1. NRF is set to the box_address as recieved, and the message "cali,<box_id>" is sent.
 * 2. Checks if recieved message is in the format "cali,ok":
 * 2.1 If message is not in format, AWS is notified.
 * 2.2 If message is in format, AWS is notified, delay of 30 sec is waited.
 * 2.2.1 After the delay of 30 secs, write to radio for the updated calibration data with message "calibration_data_update"
 * 2.2.2 Read the recieved message from NRF for the updated values
 * 2.2.3 
 * 
 * @param box_id Box ID
 * @param box_address Box Communication Address
 */
void calibrate_box(String box_id, String box_address)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Calibrating", "Box", box_id);

    char address[COMMUNICATION_ID_LENGTH];
    for (int i = 0; i < COMMUNICATION_ID_LENGTH; i++)
        address[i] = box_address[i];

    // Writing the NRF with message "cali,<box_id>"
    String message = "cali," + box_id;
    write_radio(address, message);

    // Check if the message recieved is in required format or not
    String recieved_message = read_radio(address);
    int connection_status = 0; // 0 -> Failure, 1 -> Success

    // Check if the NRF connection was successful or not
    if (recieved_message = ("cali,ok"))
    {
        connection_status = 1;
        solid_rgb_ring(GREEN_COLOR);
        update_oled("Calibration", "Connect", "Successful");
    }
    else
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("Calibration", "Connect", "Failed");
    }

    // AWS is contacted and updated if calibration is successful or not
    if (!client.connected())
        checkWiFiThenMQTT();
    else
    {
        client.loop();
        send_Success_Data((String)box_id, "calibrate", connection_status, "", "");
    }

    if (connection_status == 1)
    {
        delay(30 * 1000);                                // Wait for calibration to complete
        write_radio(address, "calibration_data_update"); // Write to radio fpr updated values
        String update_message = read_radio(address);     // Read the radio
        if (update_message != "")                        // Check if the radio message is not empty
        {
            solid_rgb_ring(GREEN_COLOR);
            update_oled("Calibration", "Successful", box_id);
        }
        else
        {
            connection_status = 0;
            solid_rgb_ring(RED_COLOR);
            update_oled("Calibration", "Failed", box_id);
        }

        if (!client.connected())
            checkWiFiThenMQTT();
        else
        {
            client.loop();
            send_Success_Data((String)box_id, "calibration_update", connection_status, update_message, "");
        }
    }
    return;
}

/**
 * @brief Contact the Boxes to collect the data from it
 * 
 * @param box_id 
 * @param box_address 
 */
void update_box_data(String box_id, String box_address)
{
    char address[COMMUNICATION_ID_LENGTH];
    for (int i = 0; i < COMMUNICATION_ID_LENGTH; i++)
        address[i] = box_address[i];

    // Write radio with the address recieved
    String write_message = "updt";
    write_radio(address, write_message);

    // Read the radio
    String message = read_radio(address);
    if (message != "")
    { // Correct format: "<count>,<temperature>,<battery>,<offset>,<average_wesight>"
        String str_sd_message = box_id + message;
        int msg_size = sizeof(str_sd_message) / sizeof(str_sd_message[0]); // Calculating the message size

        char sd_message[msg_size]; // Str to Char conversion
        for (int i = 0; i < msg_size; i++)
            sd_message[i] = str_sd_message[i];

        // Creating Directory of Format YYYY/MM/
        String str_dir = (String)year + '/' + (String)month + '/';
        int dir_size = sizeof(str_dir) / sizeof(str_dir[0]);

        char dir[dir_size]; // Str to Char conversion
        for (int i = 0; i < dir_size; i++)
            dir[i] = str_dir[i];
        createDir(SD, dir);

        // Creating File Path
        String str_file_path = (String)year + '/' + (String)month + '/' + (String)date + file_extension;
        int path_size = sizeof(str_file_path) / sizeof(str_file_path[0]);

        char file_path[path_size]; // Str to Char conversion
        for (int i = 0; i < path_size; i++)
            file_path[i] = str_file_path[i];
        appendFile(SD, file_path, sd_message);

        if (!client.connected())
            checkWiFiThenMQTT();
        else
        {
            client.loop();
            sendData(box_id, message);
        }
    }
}

/**
 * @brief Change Box Settings
 * 
 * @param box_id 
 * @param box_address 
 * @param dt 
 * @param st 
 * @param bt 
 */
void change_box_setting(String box_id, String box_address, String dt, String st, String bt)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Change", "Setting", box_id);
    int success_code = 0;

    char address[COMMUNICATION_ID_LENGTH];
    for (int i = 0; i < COMMUNICATION_ID_LENGTH; i++)
        address[i] = box_address[i];

    String message = "chng," + dt + st + bt;
    write_radio(address, message);

    String recieved_message = read_radio(address);
    if (recieved_message == "chng,ok")
    {
        success_code = 1;
        solid_rgb_ring(GREEN_COLOR);
        update_oled("Setting", "Successful", box_id);
    }
    else
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("Setting", "Failed", box_id);
    }

    // Update to AWS
    if (!client.connected())
        checkWiFiThenMQTT();
    else
    {
        client.loop();
        send_Success_Data(box_id, "change", success_code, st, bt);
    }
}

/**
 * @brief Buzz the box
 * 
 * @param box_id 
 * @param box_address 
 */
void sound_buzzer(String box_id, String box_address)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Buzz", "Box", box_id);

    char address[COMMUNICATION_ID_LENGTH];
    for (int i = 0; i < COMMUNICATION_ID_LENGTH; i++)
        address[i] = box_address[i];

    String message = "buzz";
    write_radio(address, message);

    solid_rgb_ring(GREEN_COLOR);
    update_oled("Buzz", "Box", "Successful");
}

//TODO: NRF24 Functions
/**
 * @brief Write a message to the NRF Radio
 * 
 * @param transmission_address 5 byte Transmission Address
 * @param transmission_message 32 byte Transmission Message
 */
void write_radio(char transmission_address[], String transmission_message)
{
    update_oled("Sending", "Message", "via NRF");

    byte address[COMMUNICATION_ID_LENGTH];
    for (int i = 0; i < COMMUNICATION_ID_LENGTH; i++)
        address[i] = transmission_address[i];

    radio.openWritingPipe(address);
    radio.setPALevel(RF24_PA_MIN);
    radio.stopListening();
    delay(100);

    radio.write(&transmission_message, sizeof(transmission_message));
    delay(500);
    return;
}

/**
 * @brief Reading the Message from Radio
 * 
 * @param recieving_address 5 byte Recieving Address
 * @return String Recieved Message
 */
String read_radio(char recieving_address[])
{
    update_oled("Recieving", "Message", "via NRF");

    byte address[COMMUNICATION_ID_LENGTH];
    for (size_t i = 0; i < COMMUNICATION_ID_LENGTH; i++)
        address[i] = recieving_address[i];

    radio.openReadingPipe(0, address);
    radio.setPALevel(RF24_PA_MIN);
    radio.startListening();
    delay(100);

    String recieved_message = "";
    if (radio.available())
    {
        update_oled("Recieved", "Message", "via NRF");
        radio.read(&recieved_message, sizeof(recieved_message));
    }
    else
        update_oled("Failed", "Message", "via NRF");
    delay(100);

    return recieved_message;
}

/**
 * @brief This function is used to fetch the latest data from each boxes on periodic instances.
 * This is activated when the Timer is activated and increases the count variable till its greater than the timer count.
 */
void regular_box_update(void)
{
    timerStop(timer);
    timerAlarmDisable(timer);
    Serial.println("");

    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Box", "Polling", "In Progress");
    delay(10000);

    // rtc_get_time();
    // connected_boxes = EEPROM.read(CONNECTED_BOXES_STORED_LOCATION);
    // for (int i = 0; i < connected_boxes; i++)
    // {
    //     halt_rgb_ring(i);
    //     String box_id = BOX_DETAILS[i][0];
    //     String address = BOX_DETAILS[i][1];
    //     update_oled("Contacting", "Box", box_id);
    //     update_box_data(box_id, address);
    // }

    solid_rgb_ring(GREEN_COLOR);
    update_oled("Box", "Polling", "Completed");
    timerStart(timer);
    timerAlarmEnable(timer);
}

/**
 * @brief Fetch Box Address from the Box ID Array matrix
 * 
 * @param box_id 9 byte Box ID
 * @return String 5 byte Box Communication Address
 */
String fetch_box_address(String box_id)
{
    int i;
    for (i = 0; i < connected_boxes; i++)
    {
        String box = BOX_DETAILS[i][0];
        if (box == box_id)
        {
            break;
        }
    }
    String address = BOX_DETAILS[i][1];
    return address;
}

//TODO: Auxillary Functions
/**
 * @brief Reading the SD Card File which stores the "Box ID and communication Address"
 */
void read_box_details_from_sd_card(void)
{
    Serial.println("Reading Box Details from SD Card");
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Read", "Box Details", "From SD");

    RL.readLines(BOX_ID_DETAILS_FILE, &handleEachLine);

    Serial.println("File Read Complete");
    solid_rgb_ring(GREEN_COLOR);
    update_oled("Box Details", "Read", "Done");
    return;
}

/**
 * @brief Read File Line by Line
 * 
 * @param line The Line to be read and saved
 * @param lineIndex The Index of the Line to be worked on
 */
void handleEachLine(char line[], int lineIndex)
{
    char box_id[BOX_ID_LENGTH];
    char address[COMMUNICATION_ID_LENGTH];

    for (int i = 0; i < (BOX_ID_LENGTH + COMMUNICATION_ID_LENGTH); i++)
    {
        if (i < BOX_ID_LENGTH)
        {
            box_id[i] = line[i];
        }
        else
        {
            address[i - BOX_ID_LENGTH] = line[i];
        }
    }

    BOX_DETAILS[lineIndex][0] = (String)box_id;
    BOX_DETAILS[lineIndex][1] = (String)address;
    Serial.println("Box ID: " + BOX_DETAILS[lineIndex][0] + '\n' + "Box Address" + BOX_DETAILS[lineIndex][1]);
    return;
}

//TODO: AWS Functions

/**
 * @brief This function is activated when the message is recieved from AWS
 * 
 * @param topic 
 * @param payload 
 */
void messageReceived(String &topic, String &payload)
{
    timerStop(timer);
    timerAlarmDisable(timer);
    //Reference (JSON to String Converter): https://github.com/bblanchon/ArduinoJson/blob/6.x/examples/JsonParserExample/JsonParserExample.ino
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Message", "Recieved", "from AWS");

    Serial.println("Recieved [" + topic + "]: " + payload);

    StaticJsonDocument<50> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    String box_id = doc["boxid"];
    String cmd = doc["cmd"];

    // Perform action as per the command from AWS
    if (cmd == "add") // Add new Box
        add_new_box(box_id);
    else if (cmd == "calibrate") //Calibrate box parameters
    {
        String address = fetch_box_address(box_id);
        calibrate_box(box_id, address);
    }
    else if (cmd == "ask") //Ask box for updated values
    {
        rtc_get_time();
        String address = fetch_box_address(box_id);
        solid_rgb_ring(YELLOW_COLOR);
        update_oled("Update", "Box", "In Progress");
        update_box_data(box_id, address);
        solid_rgb_ring(GREEN_COLOR);
        update_oled("Update", "Box", "Successful");
    }
    else if (cmd == "change_setting") // Change box settings
    {
        String dt = doc["dt"];
        String st = doc["st"];
        String bt = doc["bt"];
        String address = fetch_box_address(box_id);
        change_box_setting(box_id, address, dt, st, bt);
    }
    else if (cmd == "buzz")
    {
        String address = fetch_box_address(box_id);
        sound_buzzer(box_id, address);
    }

    solid_rgb_ring(GREEN_COLOR);
    update_oled("AWS", "MSG Recieved", "Complete");
    timerStart(timer);
    timerAlarmEnable(timer);
}

void lwMQTTErr(lwmqtt_err_t reason)
{
    if (reason == lwmqtt_err_t::LWMQTT_SUCCESS)
        Serial.print("Success");
    else if (reason == lwmqtt_err_t::LWMQTT_BUFFER_TOO_SHORT)
        Serial.print("Buffer too short");
    else if (reason == lwmqtt_err_t::LWMQTT_VARNUM_OVERFLOW)
        Serial.print("Varnum overflow");
    else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_CONNECT)
        Serial.print("Network failed connect");
    else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_TIMEOUT)
        Serial.print("Network timeout");
    else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_READ)
        Serial.print("Network failed read");
    else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_WRITE)
        Serial.print("Network failed write");
    else if (reason == lwmqtt_err_t::LWMQTT_REMAINING_LENGTH_OVERFLOW)
        Serial.print("Remaining length overflow");
    else if (reason == lwmqtt_err_t::LWMQTT_REMAINING_LENGTH_MISMATCH)
        Serial.print("Remaining length mismatch");
    else if (reason == lwmqtt_err_t::LWMQTT_MISSING_OR_WRONG_PACKET)
        Serial.print("Missing or wrong packet");
    else if (reason == lwmqtt_err_t::LWMQTT_CONNECTION_DENIED)
        Serial.print("Connection denied");
    else if (reason == lwmqtt_err_t::LWMQTT_FAILED_SUBSCRIPTION)
        Serial.print("Failed subscription");
    else if (reason == lwmqtt_err_t::LWMQTT_SUBACK_ARRAY_OVERFLOW)
        Serial.print("Suback array overflow");
    else if (reason == lwmqtt_err_t::LWMQTT_PONG_TIMEOUT)
        Serial.print("Pong timeout");
}

void lwMQTTErrConnection(lwmqtt_return_code_t reason)
{
    if (reason == lwmqtt_return_code_t::LWMQTT_CONNECTION_ACCEPTED)
        Serial.print("Connection Accepted");
    else if (reason == lwmqtt_return_code_t::LWMQTT_UNACCEPTABLE_PROTOCOL)
        Serial.print("Unacceptable Protocol");
    else if (reason == lwmqtt_return_code_t::LWMQTT_IDENTIFIER_REJECTED)
        Serial.print("Identifier Rejected");
    else if (reason == lwmqtt_return_code_t::LWMQTT_SERVER_UNAVAILABLE)
        Serial.print("Server Unavailable");
    else if (reason == lwmqtt_return_code_t::LWMQTT_BAD_USERNAME_OR_PASSWORD)
        Serial.print("Bad UserName/Password");
    else if (reason == lwmqtt_return_code_t::LWMQTT_NOT_AUTHORIZED)
        Serial.print("Not Authorized");
    else if (reason == lwmqtt_return_code_t::LWMQTT_UNKNOWN_RETURN_CODE)
        Serial.print("Unknown Return Code");
}

void connectToMqtt(bool nonBlocking)
{
    Serial.print("MQTT connecting ");
    while (!client.connected())
    {
        if (client.connect(THINGNAME))
        {
            Serial.println("connected!");
            if (!client.subscribe(MQTT_SUB_TOPIC))
                lwMQTTErr(client.lastError());
        }
        else
        {
            Serial.print("SSL Error Code: ");
            //  Serial.println(net.getLastSSLError());
            Serial.print("failed, reason -> ");
            lwMQTTErrConnection(client.returnCode());
            if (!nonBlocking)
            {
                Serial.println(" < try again in 5 seconds");
                delay(5000);
            }
            else
            {
                Serial.println(" <");
            }
        }
        if (nonBlocking)
            break;
    }
}

void connectToWiFi(String init_str)
{
    if (init_str != emptyString)
        Serial.print(init_str);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }
    if (init_str != emptyString)
        Serial.println("ok!");
}

void checkWiFiThenMQTT(void)
{
    connectToWiFi("Checking WiFi");
    connectToMqtt(false);
}

unsigned long previousMillis = 0;
const long interval = 5000;

void checkWiFiThenMQTTNonBlocking(void)
{
    connectToWiFi(emptyString);
    if (millis() - previousMillis >= interval && !client.connected())
    {
        previousMillis = millis();
        connectToMqtt(true);
    }
}

void checkWiFiThenReboot(void)
{
    connectToWiFi("Checking WiFi");
    Serial.print("Rebooting");
    ESP.restart();
}

/**
 * @brief Upload data from special protocols to AWS
 * 
 * @param box_id 
 * @param command 
 * @param success_status 
 * @param param1 
 * @param param2 
 */
void send_Success_Data(String box_id, String command, int success_status, String param1, String param2)
{
    // Reference (String to JSON Converter):https://github.com/bblanchon/ArduinoJson/blob/6.x/examples/JsonGeneratorExample/JsonGeneratorExample.ino
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("AWS", "Upload", "InProgess");

    StaticJsonDocument<50> doc;
    doc["Box ID"] = box_id;
    doc["cmd"] = command;
    doc["success"] = success_status;

    if (command == "add")
        doc["Box Address"] = param1;
    else if (command == "calibration_update")
    {
        if (success_status == 1)
            doc["Calibration Data"] = param1;
    }
    else if (command = "change")
    {
        doc["st"] = param1;
        doc["bt"] = param2;
    }
    serializeJson(doc, Serial);
    serializeJsonPretty(doc, Serial);
    // The above line prints:
    // {
    //   "Box ID": "123456789",
    //   "cmd": add,
    //   "success": 1,
    //   "param1": "param1_data",
    //   "param2": "param2_data",
    // }

    char shadow[measureJson(doc) + 1];

    serializeJson(doc, shadow, sizeof(shadow));

    if (!client.publish(MQTT_PUB_TOPIC, shadow, false, 0))
    {
        lwMQTTErr(client.lastError());
        update_oled("AWS", command, "Failure");
    }
    else
        update_oled("AWS", command, "Success");
}

/**
 * @brief Upload data to AWS for periodic box updates
 * 
 * @param box_id 
 * @param message 
 */
void sendData(String box_id, String message)
{
    // Reference:https://github.com/bblanchon/ArduinoJson/blob/6.x/examples/JsonGeneratorExample/JsonGeneratorExample.ino
    char token = ',';
    char params[UPDATE_PARAMETERS][10] = {
        {/*Count*/},
        {/*Temperature*/},
        {/*Battery*/},
        {/*Offset*/},
        {/*Average Weight*/}};

    int j = 0;
    for (int i = 0; i < (sizeof(message) / message[0]); i++)
    {
        if (message[i] != token)
            params[j][i % 10] = message[i];
        else
            j++;
    }
    String c = (String)params[0]; //Count
    String t = (String)params[1]; //Temperature
    String b = (String)params[2]; //Battery
    String o = (String)params[3]; //Offset
    String a = (String)params[4]; //Average Weight

    StaticJsonDocument<50> doc;
    doc["time"] = (char)year + '/' + (char)month + '/' + (char)date + ' ' + (char)hour + ':' + (char)minutes + ':' + (char)seconds;
    doc["cmd"] = "ask";
    doc["c"] = c;
    doc["t"] = t;
    doc["b"] = b;
    doc["o"] = o;
    doc["a"] = a;
    serializeJson(doc, Serial);
    serializeJsonPretty(doc, Serial);
    // The above line prints:
    // {
    //   "Box ID": "123456789",
    //   "cmd": add,
    //   "success": 1,
    //   "param1": "param1_data",
    //   "param2": "param2_data",
    // }
    char shadow[measureJson(doc) + 1];
    serializeJson(doc, shadow, sizeof(shadow));

    if (!client.publish(MQTT_PUB_TOPIC, shadow, false, 0))
    {
        lwMQTTErr(client.lastError());
        update_oled("AWS", "update", "Failure");
    }
    else
        update_oled("AWS", "update", "Success");
}

//TODO: Basic Setup, Code Operation Starts here
void setup()
{
    Serial.begin(115200);
    rgb_ring_setup();
    oled_setup();
    radio.begin();
    EEPROM.begin(EEPROM_SIZE);
    pin_setup();
    wifi_ntp_setup();
    aws_setup();
    sd_setup();
    read_box_details_from_sd_card();
    // if (BOX_DETAILS[0][0] == NULL)
    // {
    //     solid_rgb_ring(RED_COLOR);
    //     update_oled("No", "Box", "Linked");
    //     Serial.println("No Box Linked");
    //     while (BOX_DETAILS[0][0] == NULL)
    //     {
    //     }
    // }
    configure_timer();
    timerAlarmEnable(timer);
    print_company_logo();
}

//TODO: Loop for the code
void loop()
{
    if (digitalRead(outputS) == HIGH) // Detects when the rotary encoder button is pressed
    {
        timerStop(timer);
        timerAlarmDisable(timer);
        delay(DEBOUNCE_TIME); // Debounce time
        if (digitalRead(outputS) == HIGH)
        {
            delay(500);
            read_rotary_encoder(); // Open Main Menu
            print_company_logo();
        }
        delay(500);
        timerStart(timer);
        timerAlarmEnable(timer);
    }
    if (count > TIMER) // When Count variable overflows the Timer Value, the periodic box polling is done
    {
        count = 0;
        regular_box_update();
        print_company_logo();
    }
}
