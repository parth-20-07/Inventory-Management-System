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
#include "USER_CONFIGURATION.h"
#include "DEVICE_CONFIGURATION.h"
#include "ERROR_LOG.h"

//! WiFi and NTP Setup
//Reference (NTP Setup): https://lastminuteengineers.com/esp32-ntp-server-date-time-tutorial/
//Reference (ESP32 Hotspot): https://lastminuteengineers.com/creating-esp32-web-server-arduino-ide/
//Reference (ESP32 Server): https://randomnerdtutorials.com/esp32-esp8266-input-data-html-form/
#include <WiFi.h>
#include "time.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);
char ssid[20];
char password[20];
int wifi_connection_status = 0; // 0-> WiFi Disconnected, 1-> WiFi Connected
const char *ntpServer = "asia.pool.ntp.org";
const long gmtOffset_sec = 19800; // +5:30 = (5*60*60) + (30*60) = 19800
const int daylightOffset_sec = 0; // India doesn't observe DayLight Saving
uint16_t year;
uint8_t month;
uint8_t date;
uint8_t hour;
uint8_t minutes;
uint8_t seconds;

const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pwd";

// HTML web page to handle 3 input fields (input1, input2, input3)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Wi-Fi Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    SSID: <input type="text" name="ssid">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    Password: <input type="text" name="pwd">
    <input type="submit" value="Submit">
  </form><br>
</body></html>)rawliteral";

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

//! Menu Setup
// Code for Main Screen List
int lastMillis;
int main_screen_count = 1;
String latest_error_log = "";
#define CHARACTER_LENGTH 9 // Maximum Character Length allowed in a line on OLED

// Code for Menu List
#define MAIN_MENU_LIST 7
#define SUB_MENU_LEVEL 2
#define SUB_MENU_LIST 5
#define OLED_TEXT_SIZE 2
#define OLED_MENU_TEXT_SIZE 2
String main_menu[MAIN_MENU_LIST][SUB_MENU_LEVEL][SUB_MENU_LIST] = {
    {{"< Back"}, {}},
    {{"Connected\n Boxes"}, {"< Back"}},
    {{"Connect\n New WiFi"}, {"< Back"}},
    {{"Reciever\n Details"}, {"< Back"}},
    {{"Ring\n Bright."}, {"<Back"}},
    {{"Error Log"}, {"< Back"}},
    {{"Contact\n Us"}, {"< Back"}},
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

byte BROADCAST_RECIEVER_ADDRESS[COMMUNICATION_ID_LENGTH] = "boxit";
String BOX_DETAILS[MAX_BOXES][3];

//! SD Card Setup
//Reference (SD Card Module): https://randomnerdtutorials.com/esp32-microsd-card-arduino/
//Reference: (ReadLines Library): https://github.com/mykeels/ReadLines/blob/master/examples/print-line-and-index/print-line-only.ino
#include "FS.h"
#include "SD.h"
#include <ReadLines.h>
const char AWS_BACKLOG_FILE[] = "/AWS_Backlog_file.txt";
const char NRF_COMMUNICATION_ADDRESS_FILE[] = "/NRF_Communication_file.txt";
char BOX_ID_DETAILS_FILE[] = "/Box_ID_Details.txt";
char ERROR_LOG_FILE[] = "/Error_Log.txt";
char SSID_FILE[] = "/SSID.txt";
char PASSWORD_FILE[] = "/Password.txt";
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
#define DELAY_ONE_MINUTES 1 * 60     // 1 min periodic count
#define DELAY_FIVE_MINUTES 5 * 60    // 5 mins periodic count
#define DELAY_TEN_MINUTES 10 * 60    // 10 mins periodic count
#define DELAY_THIRTY_MINUTES 30 * 60 // 30 mins periodic count
#define DELAY_SIXTY_MINUTES 60 * 60  // 60 mins periodic count
#define ESP32_CLOCK 80000000         // 80MHz Clock on ESP32
#define PRESCALAR 80                 // Prescalar creates a timer to set the alarm for 4 min 16 sec
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
#define EEPROM_SIZE 10
#define CONNECTED_BOXES_STORED_LOCATION 0
#define LED_RING_BRIGHTNESS_STORED_LOCATION 5

//! Function Definition
void configure_timer(void);
void IRAM_ATTR onTime(void);
void pin_setup(void);
void wifi_ntp_setup(void);
void rtc_setup(void);
void oled_setup(void);
void sd_setup(void);
void rgb_ring_setup(int led_brightness = 110);
void aws_setup(void);
void print_company_logo(void);
void update_oled(String text1, String text2, String text3);
void oled_menu_update(String item1, String item2, String item3);
void printLocalTime(void);
void rtc_get_time(void);
void solid_rgb_ring(uint32_t color);
void halt_rgb_ring(int led_number);
void change_led_ring_brightness(void);
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void createDir(fs::FS &fs, const char *path);
void deleteFile(fs::FS &fs, const char *path);
void readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void read_rotary_encoder(void);
void add_new_box(String box_id, String dt);
void calibrate_box(String box_id, byte *ptr_box_address);
void update_box_data(String box_id, byte *ptr_box_address);
void change_box_setting(String box_id, byte *ptr_box_address, String dt, String st, String bt);
void sound_buzzer(String box_id, byte *ptr_box_address);
void write_radio(byte *ptr_transmission_address, String transmission_message);
String read_radio(byte *ptr_recieving_address);
void regular_box_update(int counter);
String fetch_box_address(String box_id);
void read_box_details_from_sd_card(void);
void handleEachLine(char line[], int lineIndex);
void handleEachErrorLine(char line[], int lineIndex);
void handleEachSSIDLine(char line[], int lineIndex);
void handleEachPasswordLine(char line[], int lineIndex);
void write_error_log(String error_log_tag);
void messageReceived(String &topic, String &payload);
void lwMQTTErr(lwmqtt_err_t reason);
void lwMQTTErrConnection(lwmqtt_return_code_t reason);
void connectToMqtt(bool nonBlocking);
void connectToWiFi(String init_str);
void checkWiFiThenMQTT(void);
void checkWiFiThenMQTTNonBlocking(void);
void checkWiFiThenReboot(void);
void send_Success_Data(String box_id, String command, int success_status, String param1, String param2, String param3);
void sendData(String box_id, String message);
void connect_to_new_wifi(void);

//! Function Declaration

//TODO: Timer Functions
/**
 * @brief Configuring Timer for Alarm.
 */
void configure_timer(void)
{
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
    solid_rgb_ring(YELLOW_COLOR);
    WiFi.disconnect(); // Disconnect to any connected wifi if any
    RL.readLines(SSID_FILE, &handleEachSSIDLine);
    RL.readLines(PASSWORD_FILE, &handleEachPasswordLine);
    Serial.println("STR SSID: " + (String)ssid);
    Serial.println("STR Password: " + (String)password);
    if ((String)ssid == "")
        connect_to_new_wifi();
    update_oled("Wi-Fi", "Connecting", ssid);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    wifi_connection_status = 0;
    int connection_attempts = 0;
    int max_attempts = 500;
    while (connection_attempts < max_attempts)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            delay(10);
            Serial.print(".");
            connection_attempts++;
        }
        else
        {
            wifi_connection_status = 1;
            break;
        }
    }
    rtc_setup();

    if (wifi_connection_status == 1)
    {
        solid_rgb_ring(GREEN_COLOR);
        update_oled("Wifi", "Connected", ssid);
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // Configure NTP
        printLocalTime();                                         //Fetching time from NTP
    }
    else
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("Wifi not", "Connected", ssid);
        rtc_get_time();                      // Fetching time from RTC
        write_error_log(WIFI_NOT_CONNECTED); // Error Log
    }
    return;
}

/**
 * @brief Setup RTC
 */
void rtc_setup(void)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("RTC", "Connection", "In Progress");

    if (!rtc.begin())
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("RTC", "Connection", "Failed");
    }

    solid_rgb_ring(GREEN_COLOR);
    update_oled("RTC", "Setup", "Done");
    return;
}

/**
 * @brief Setting Up OLED Module
 */
void oled_setup(void)
{
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
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
void rgb_ring_setup(int led_brightness)
{
    pixels.begin();
    if (led_brightness > 100)
        led_brightness = EEPROM.read(LED_RING_BRIGHTNESS_STORED_LOCATION);
    Serial.println("LED Brightness: " + (String)led_brightness);
    pixels.setBrightness(led_brightness * 255 / 100);
    return;
}

/**
 * @brief Setup AWS
 * 
 */
void aws_setup(void)
{
    Serial.println("Setting up AWS");
    if (wifi_connection_status == 0)
        wifi_ntp_setup();
    else
    {
        solid_rgb_ring(YELLOW_COLOR);
        update_oled("AWS", "Setup", "In Progress");
        net.setCACert(cacert);
        net.setCertificate(client_cert);
        net.setPrivateKey(privkey);
        client.begin(MQTT_HOST, MQTT_PORT, net);
        client.onMessage(messageReceived);
        connectToMqtt(false);
        solid_rgb_ring(GREEN_COLOR);
        update_oled("AWS", "Setup", "Complete");
    }
}

//TODO: OLED Functions

/**
 * @brief Print Company Logo on OLED and turn the switch LED Ring to BLUE Color
 */
void print_company_logo(void)
{
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
    display.clearDisplay();
    display.setTextSize(OLED_TEXT_SIZE);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println(text1);
    display.setCursor(0, SCREEN_HEIGHT / 3);
    display.println(text2);
    display.setCursor(0, 2 * SCREEN_HEIGHT / 3);
    display.println(text3);
    display.display();
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

    display.setCursor(10, 0);
    display.println(item1);
    if (item1.length() <= CHARACTER_LENGTH)
    {
        display.setCursor(0, SCREEN_HEIGHT / 3);
        display.println(item2);
        if (item2.length() <= CHARACTER_LENGTH)
        {
            display.setCursor(0, 2 * SCREEN_HEIGHT / 3);
            display.println(item3);
        }
    }
    else
    {
        display.setCursor(15, 2 * SCREEN_HEIGHT / 3);
        display.println(item2);
    }

    display.display();

    return;
}

//TODO: WiFi and NTP Functions
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
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("NTP", "Update", "Failed");
        Serial.println("NTP Failed to Update");
        rtc_get_time();
        write_error_log(NTP_NOT_UPDATED); // Error Log
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

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

/**
 * @brief Setup ESP as WiFi Router and get new login credentials
 * Reference: https://randomnerdtutorials.com/esp32-esp8266-input-data-html-form/
 */
void connect_to_new_wifi(void)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Connect", "To New", "WiFi");
    delay(500);
    deleteFile(SD, SSID_FILE);
    deleteFile(SD, PASSWORD_FILE);

    IPAddress local_ip(192, 168, 1, 1);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.mode(WIFI_AP);
    char temp_ssid[] = "ESP32";
    char temp_pwd[] = "123456789";
    WiFi.softAP(temp_ssid, temp_pwd);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);
    Serial.print("IP address: 192.168.1.1");

    // Send web page with input fields to client
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", index_html); });

    // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
    server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  String inputMessage;
                  String inputParam;
                  // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
                  if (request->hasParam(PARAM_INPUT_1))
                  {
                      inputMessage = request->getParam(PARAM_INPUT_1)->value();
                      inputParam = PARAM_INPUT_1;
                      for (size_t i = 0; i < inputMessage.length(); i++)
                          ssid[i] = inputMessage[i];
                      writeFile(SD, SSID_FILE, ssid);
                      Serial.println((String)ssid);
                  }
                  // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
                  else if (request->hasParam(PARAM_INPUT_2))
                  {
                      inputMessage = request->getParam(PARAM_INPUT_2)->value();
                      inputParam = PARAM_INPUT_2;
                      for (size_t i = 0; i < inputMessage.length(); i++)
                          password[i] = inputMessage[i];
                      writeFile(SD, PASSWORD_FILE, password);
                      Serial.println((String)password);
                  }
                  else
                  {
                      inputMessage = "No message sent";
                      inputParam = "none";
                  }
                  Serial.println(inputMessage);
                  request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" + inputParam + ") with value: " + inputMessage + "<br><a href=\"/\">Return to Home Page</a>");
              });
    server.onNotFound(notFound);
    server.begin();
    int i = 0;
    while (1)
    {
        if (i < 100)
            update_oled("SSID:", (String)temp_ssid, "");
        else if (i < 200)
            update_oled("Password:", (String)temp_pwd, "");
        else if (i < 300)
            update_oled("Connect @", "https://192.168.1.1/", "");
        else
            i = 0;
        i++;
        delay(1);
    }
}

//TODO: RTC Functions
/**
 * @brief Fetch Time from RTC and save it in global time variables
 * 
 */
void rtc_get_time(void)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("RTC", "Update", "In Progress");
    DateTime now = rtc.now();
    solid_rgb_ring(GREEN_COLOR);
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

void change_led_ring_brightness(void)
{
    delay(500);
    String item1 = "Bright.:";
    String item2 = "";
    String item3 = "";
    int current_led_brightness = EEPROM.read(LED_RING_BRIGHTNESS_STORED_LOCATION);
    item3 = (String)current_led_brightness + '%';
    oled_menu_update(item1, item2, item3);
    aLastState = digitalRead(outputA);
    while (1)
    {
        aState = digitalRead(outputA); // Reads the "current" state of the outputA
        if (aState != aLastState)
        {
            // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
            if (digitalRead(outputB) != aState)
                current_led_brightness++;
            else
                current_led_brightness--;

            if (current_led_brightness > 100)
                current_led_brightness = 100; // Ensures that the menu list does not go below the coded list to avoid menu display error
            else if (current_led_brightness <= 0)
                current_led_brightness = 0; // Ensures that the menu list does not go above the coded list to avoid menu display error

            item1 = "Bright.:";
            item2 = "";
            item3 = (String)current_led_brightness + '%';
            rgb_ring_setup(current_led_brightness);
            solid_rgb_ring(BLUE_COLOR);
            oled_menu_update(item1, item2, item3);
        }
        aLastState = aState; // Updates the previous state of the outputA with the current state
        if (digitalRead(outputS) == HIGH)
        {
            delay(DEBOUNCE_TIME);
            if (digitalRead(outputS) == HIGH)
            {
                EEPROM.write(LED_RING_BRIGHTNESS_STORED_LOCATION, current_led_brightness);
                EEPROM.commit();
                oled_menu_update("Saving", "To", "Memory");
                delay(500);
                break;
            }
        }
    }
    Serial.println("Exit");
    return;
}

//TODO: MicroSD Card Functions3
/**
 * @brief List All the Available Directories
 * 
 * @param fs 
 * @param dirname 
 * @param levels 
 */
void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);
    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

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
 * @brief Delete the File
 * 
 * @param fs SD Object
 * @param path 
 */
void deleteFile(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path))
    {
        Serial.println("File deleted");
    }
    else
    {
        Serial.println("Delete failed");
    }
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
    String read_line = "";
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Reading", "File from", "MicroSD");
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        solid_rgb_ring(RED_COLOR);
        update_oled("File", "Open", "Failed");
    }

    Serial.print("Read from file: ");
    while (file.available())
        read_line = file.read();
    file.close();
    solid_rgb_ring(GREEN_COLOR);
    update_oled("File", "Read", "Successful");
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

    int main_list_pos = 0;            // Position of the cursor on the main menu
    int sub_list_pos = 0;             // Position of the cursor on the sub list
    int menu_level = 0;               // Level of the cursor on the menu level
    int menu_item = 0;                // The value of the selected menu item
    int menu_length = MAIN_MENU_LIST; // Variable to store the number of items at the current menu level
    aLastState = digitalRead(outputA);

    // 3 items to display on the oled
    String item1 = main_menu[main_list_pos + 0][menu_level][sub_list_pos];
    String item2 = main_menu[main_list_pos + 1][menu_level][sub_list_pos];
    String item3 = main_menu[main_list_pos + 2][menu_level][sub_list_pos];
    oled_menu_update(item1, item2, item3);
    aLastState = digitalRead(outputA);
    while (1)
    {
        item1 = "";
        item2 = "";
        item3 = "";

        aState = digitalRead(outputA); // Reads the "current" state of the outputA
        if (aState != aLastState)
        {
            // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
            if (digitalRead(outputB) != aState)
                counter += 0.5;
            else
                counter -= 0.5;

            if ((int)counter >= menu_length - 1)
                counter = menu_length - 1; // Ensures that the menu list does not go below the coded list to avoid menu display error
            else if ((int)counter < 0)
                counter = 0; // Ensures that the menu list does not go above the coded list to avoid menu display error
        }
        aLastState = aState; // Updates the previous state of the outputA with the current state

        if (digitalRead(outputS) == HIGH)
        {
            delay(3 * DEBOUNCE_TIME);
            if (digitalRead(outputS) == HIGH)
            {
                if ((int)counter == 0)
                {
                    if (menu_level == 0)
                        break;
                    else
                    {
                        menu_level--;
                        menu_item = 0;
                    }
                }
                else
                {
                    counter = 0;
                    menu_item = 10 * main_list_pos;
                    if (menu_item != 10)
                    {
                        menu_level++;
                        if (menu_item == 40)
                        {
                            change_led_ring_brightness();
                            menu_item = 0;
                            menu_level--;
                        }
                    }
                }
            }
        }
        if (menu_level == 0)
        {
            main_list_pos = (int)counter;
            item1 = main_menu[main_list_pos][menu_level][0];
            if ((int)counter < menu_length - 1)
                item2 = main_menu[main_list_pos + 1][menu_level][0];
            if ((int)counter < menu_length - 2)
                item3 = main_menu[main_list_pos + 2][menu_level][0];
        }
        else if (menu_level == 1)
        {
            item1 = main_menu[main_list_pos][1][0];
            if (menu_item != 10)
            {
                counter = 0;
                switch (menu_item)
                {

                case 20: // Connect to New Wifi
                    connect_to_new_wifi();
                    break;

                case 30: // Reciever Details
                    item2 = "Rec. ID";
                    item3 = RECIEVER_ID;
                    break;

                case 40: // Change LED Ring Brightness
                    break;

                case 50: // Error Log
                    item2 = "Err. Log";
                    item3 = latest_error_log;
                    break;

                case 60: // Contact us
                    item2 = contact_us_link;
                    break;

                default:
                    break;
                }
            }
            else if (menu_item == 10)
            {
                sub_list_pos = (int)counter;
                if (sub_list_pos == 0)
                    item1 = main_menu[main_list_pos][1][0];
                else
                    item1 = BOX_DETAILS[sub_list_pos][0];

                if ((int)counter < menu_length - 1)
                    item2 = BOX_DETAILS[sub_list_pos + 1][0];
                if ((int)counter < menu_length - 2)
                    item3 = BOX_DETAILS[sub_list_pos + 2][0];
            }
        }
        oled_menu_update(item1, item2, item3);
    }
    solid_rgb_ring(BLUE_COLOR);
    return;
}
//TODO: Menu Operation Functions

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
void add_new_box(String box_id, String dt)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Adding", "New Box", box_id);

    // Generate Random Address for Communication
    srand(time(0));

    byte communication_address[COMMUNICATION_ID_LENGTH]; // Communication address to be sent for NRF Communication
    char save_sd_address[COMMUNICATION_ID_LENGTH];       // Address saved in order to save on sd card because byte format cannot be converted to string
    for (int i = 0; i < COMMUNICATION_ID_LENGTH; i++)
    {
        save_sd_address[i] = alphabet[rand() % (AVAILABLE_CHARACTERS - 1)];
        communication_address[i] = save_sd_address[i];
    }

    // Write the message in format "pair,<box_id>,<communication_address>"
    String message = "pair," + box_id + "," + (String)save_sd_address;
    write_radio(BROADCAST_RECIEVER_ADDRESS, message);

    // Wait for the Box to return a message
    String recieved_message = read_radio(communication_address);
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
        char sd_message[BOX_ID_LENGTH + COMMUNICATION_ID_LENGTH + dt.length()];
        for (int i = 0; i < (BOX_ID_LENGTH + COMMUNICATION_ID_LENGTH + dt.length()); i++)
        {
            if (i < BOX_ID_LENGTH)
                sd_message[i] = box_id[i];
            else if (i < COMMUNICATION_ID_LENGTH)
                sd_message[i] = save_sd_address[i - BOX_ID_LENGTH];
            else
                sd_message[i] = dt[i];
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
        send_Success_Data((String)box_id, "add", connection_status, (String)save_sd_address, "", "");
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
void calibrate_box(String box_id, byte *ptr_box_address)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Calibrating", "Box", box_id);

    // Writing the NRF with message "cali,<box_id>"
    String message = "cali," + box_id;
    write_radio(ptr_box_address, message);

    // Check if the message recieved is in required format or not
    String recieved_message = read_radio(ptr_box_address);
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
        send_Success_Data((String)box_id, "calibrate", connection_status, "", "", "");
    }

    if (connection_status == 1)
    {
        delay(30 * 1000);                                        // Wait for calibration to complete
        write_radio(ptr_box_address, "calibration_data_update"); // Write to radio fpr updated values
        String update_message = read_radio(ptr_box_address);     // Read the radio
        if (update_message != "")                                // Check if the radio message is not empty
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
            send_Success_Data((String)box_id, "calibration_update", connection_status, update_message, "", "");
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
void update_box_data(String box_id, byte *ptr_box_address)
{

    // Write radio with the address recieved
    String write_message = "updt";
    write_radio(ptr_box_address, write_message);

    // Read the radio
    String message = read_radio(ptr_box_address);
    if (message != "")
    { // Correct format: "<count>,<temperature>,<battery>,<offset>,<average_wesight>"

        String str_sd_message = hour + '/' + minutes + '/' + seconds + box_id + message;
        int msg_size = str_sd_message.length(); // Calculating the message size
        char sd_message[msg_size];              // Str to Char conversion
        for (int i = 0; i < msg_size; i++)
            sd_message[i] = str_sd_message[i];

        // Creating Directory of Format YYYY/MM/
        String str_dir = '/' + (String)year + '/' + (String)month;
        int dir_size = str_dir.length();
        char dir[dir_size]; // Str to Char conversion
        for (int i = 0; i < dir_size; i++)
            dir[i] = str_dir[i];
        createDir(SD, dir);

        // Creating File Path
        String str_file_path = '/' + (String)year + '/' + (String)month + '/' + (String)date + file_extension;
        int path_size = str_file_path.length();
        char file_path[path_size]; // Str to Char conversion
        for (int i = 0; i < path_size; i++)
            file_path[i] = str_file_path[i];
        appendFile(SD, file_path, sd_message);

        if (wifi_connection_status == 1)
        {
            if (!client.connected())
                checkWiFiThenMQTT();
            else
            {
                client.loop();
                sendData(box_id, message);
            }
        }
        else
            appendFile(SD, AWS_BACKLOG_FILE, sd_message);
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
void change_box_setting(String box_id, byte *ptr_box_address, String dt, String st, String bt)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Change", "Setting", box_id);
    int success_code = 0;

    String message = "chng," + dt + st + bt;
    write_radio(ptr_box_address, message);

    String recieved_message = read_radio(ptr_box_address);
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
        send_Success_Data(box_id, "change", success_code, dt, st, bt);
    }
}

/**
 * @brief Buzz the box
 * 
 * @param box_id 
 * @param box_address 
 */
void sound_buzzer(String box_id, byte *ptr_box_address)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Buzz", "Box", box_id);

    String message = "buzz";
    write_radio(ptr_box_address, message);

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
void write_radio(byte *ptr_transmission_address, String transmission_message)
{
    update_oled("Sending", "Message", "via NRF");

    radio.openWritingPipe(*ptr_transmission_address);
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
String read_radio(byte *ptr_recieving_address)
{
    update_oled("Recieving", "Message", "via NRF");

    radio.openReadingPipe(0, *ptr_recieving_address);
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
void regular_box_update(int counter)
{
    timerStop(timer);
    timerAlarmDisable(timer);

    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Box", "Polling", "In Progress");
    delay(10000);

    rtc_get_time();
    connected_boxes = EEPROM.read(CONNECTED_BOXES_STORED_LOCATION);
    for (int i = 0; i < connected_boxes; i++)
    {
        halt_rgb_ring(i);

        String str_delay_time = BOX_DETAILS[i][2];
        char char_delay_time[2];
        for (int i = 0; i < 2; i++)
        {
            char_delay_time[i] = str_delay_time[i];
        }
        int dt = atoi(char_delay_time);
        Serial.println("dt: " + (String)dt);
        if (counter % dt == 0)
        {
            String box_id = BOX_DETAILS[i][0];
            String address = BOX_DETAILS[i][1];
            byte box_address[COMMUNICATION_ID_LENGTH];
            for (size_t i = 0; i < COMMUNICATION_ID_LENGTH; i++)
                box_address[i] = address[i];

            update_oled("Contacting", "Box", box_id);
            update_box_data(box_id, box_address);
        }
    }

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
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Read", "Box Details", "From SD");
    RL.readLines(BOX_ID_DETAILS_FILE, &handleEachLine);
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
    char dt[2];

    for (int i = 0; i < (BOX_ID_LENGTH + COMMUNICATION_ID_LENGTH + 2); i++)
    {
        if (i < BOX_ID_LENGTH)
        {
            box_id[i] = line[i];
        }
        else if (i < COMMUNICATION_ID_LENGTH)
        {
            address[i - BOX_ID_LENGTH] = line[i];
        }
        else
        {
            dt[i - BOX_ID_LENGTH - COMMUNICATION_ID_LENGTH] = line[i];
        }
    }

    BOX_DETAILS[lineIndex][0] = (String)box_id;
    BOX_DETAILS[lineIndex][1] = (String)address;
    BOX_DETAILS[lineIndex][2] = (String)dt;
    Serial.println("Box ID: " + BOX_DETAILS[lineIndex][0] + '\n' + "Box Address: " + BOX_DETAILS[lineIndex][1] + '\n' + "Delay Time: " + BOX_DETAILS[lineIndex][2]);
    return;
}

/**
 * @brief Read Error Log File Line by Line and save the latest error log on the variable
 * 
 * @param line 
 * @param lineIndex 
 */
void handleEachErrorLine(char line[], int lineIndex)
{
    char error_log[ERROR_LOG_LENGTH];

    for (int i = 0; i < ERROR_LOG_LENGTH; i++)
    {
        error_log[i] = line[i];
    }

    latest_error_log = (String)error_log;
    return;
}

/**
 * @brief Read Line by Line for SSID File
 * 
 * @param line 
 * @param lineIndex 
 */
void handleEachSSIDLine(char line[], int lineIndex)
{
    for (int i = 0; i < SSID_CHAR_LENGTH; i++)
    {
        ssid[i] = line[i];
        if (line[i] == NULL)
            return;
    }
}

/**
 * @brief Read Line by Line for Password File
 * 
 * @param line 
 * @param lineIndex 
 */
void handleEachPasswordLine(char line[], int lineIndex)
{
    for (int i = 0; i < PASSWORD_CHAR_LENGTH; i++)
    {
        password[i] = line[i];
        if (line[i] == NULL)
            return;
    }
}

/**
 * @brief Write Error Log to SD Card in ERROR_LOG_FILE
 * Format: YYYY/MM/DD-HH:MM:SS-#<XXXX>
 * <XXXX> - Error Code
 * @param error_log_tag 
 */
void write_error_log(String error_log_tag)
{
    //Writing Error
    String str_error_log = (String)hour + ':' + (String)minutes + ' ' + error_log_tag;
    latest_error_log = str_error_log;
    char error_log[str_error_log.length()];
    for (int i = 0; i < str_error_log.length(); i++)
        error_log[i] = str_error_log[i];

    writeFile(SD, ERROR_LOG_FILE, error_log);
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

    String address = fetch_box_address(box_id);

    // Perform action as per the command from AWS
    if (cmd == "add") // Add new Box
    {
        String dt = doc["dt"];
        add_new_box(box_id, dt);
    }
    else
    {
        String address = fetch_box_address(box_id);
        byte box_address[COMMUNICATION_ID_LENGTH];
        for (size_t i = 0; i < COMMUNICATION_ID_LENGTH; i++)
            box_address[i] = address[i];

        if (cmd == "calibrate") //Calibrate box parameters
            calibrate_box(box_id, box_address);
        else if (cmd == "ask") //Ask box for updated values
        {
            rtc_get_time();
            solid_rgb_ring(YELLOW_COLOR);
            update_oled("Update", "Box", "In Progress");
            update_box_data(box_id, box_address);
            solid_rgb_ring(GREEN_COLOR);
            update_oled("Update", "Box", "Successful");
        }
        else if (cmd == "change_setting") // Change box settings
        {
            String dt = doc["dt"];
            String st = doc["st"];
            String bt = doc["bt"];
            change_box_setting(box_id, box_address, dt, st, bt);
        }
        else if (cmd == "buzz")
        {
            sound_buzzer(box_id, box_address);
        }
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
    {
        write_error_log("#002");
        Serial.print("Buffer too short");
    }
    else if (reason == lwmqtt_err_t::LWMQTT_VARNUM_OVERFLOW)
    {
        write_error_log("#003");
        Serial.print("Varnum overflow");
    }
    else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_CONNECT)
    {
        write_error_log("#004");
        Serial.print("Network failed connect");
    }
    else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_TIMEOUT)
    {
        write_error_log("#005");
        Serial.print("Network timeout");
    }
    else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_READ)
    {
        write_error_log("#006");
        Serial.print("Network failed read");
    }
    else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_WRITE)
    {
        write_error_log("#007");
        Serial.print("Network failed write");
    }
    else if (reason == lwmqtt_err_t::LWMQTT_REMAINING_LENGTH_OVERFLOW)
    {
        write_error_log("#008");
        Serial.print("Remaining length overflow");
    }
    else if (reason == lwmqtt_err_t::LWMQTT_REMAINING_LENGTH_MISMATCH)
    {
        write_error_log("#009");
        Serial.print("Remaining length mismatch");
    }
    else if (reason == lwmqtt_err_t::LWMQTT_MISSING_OR_WRONG_PACKET)
    {
        write_error_log("#010");
        Serial.print("Missing or wrong packet");
    }
    else if (reason == lwmqtt_err_t::LWMQTT_CONNECTION_DENIED)
    {
        write_error_log("#011");
        Serial.print("Connection denied");
    }
    else if (reason == lwmqtt_err_t::LWMQTT_FAILED_SUBSCRIPTION)
    {
        write_error_log("#012");
        Serial.print("Failed subscription");
    }
    else if (reason == lwmqtt_err_t::LWMQTT_SUBACK_ARRAY_OVERFLOW)
    {
        write_error_log("#013");
        Serial.print("Suback array overflow");
    }
    else if (reason == lwmqtt_err_t::LWMQTT_PONG_TIMEOUT)
    {
        write_error_log("#014");
        Serial.print("Pong timeout");
    }
}

void lwMQTTErrConnection(lwmqtt_return_code_t reason)
{
    if (reason == lwmqtt_return_code_t::LWMQTT_CONNECTION_ACCEPTED)
        Serial.print("Connection Accepted");
    else if (reason == lwmqtt_return_code_t::LWMQTT_UNACCEPTABLE_PROTOCOL)
    {
        write_error_log("#015");
        Serial.print("Unacceptable Protocol");
    }
    else if (reason == lwmqtt_return_code_t::LWMQTT_IDENTIFIER_REJECTED)
    {
        write_error_log("#016");
        Serial.print("Identifier Rejected");
    }
    else if (reason == lwmqtt_return_code_t::LWMQTT_SERVER_UNAVAILABLE)
    {
        write_error_log("#017");
        Serial.print("Server Unavailable");
    }
    else if (reason == lwmqtt_return_code_t::LWMQTT_BAD_USERNAME_OR_PASSWORD)
    {
        write_error_log("#018");
        Serial.print("Bad UserName/Password");
    }
    else if (reason == lwmqtt_return_code_t::LWMQTT_NOT_AUTHORIZED)
    {
        write_error_log("#019");
        Serial.print("Not Authorized");
    }
    else if (reason == lwmqtt_return_code_t::LWMQTT_UNKNOWN_RETURN_CODE)
    {
        write_error_log("#020");
        Serial.print("Unknown Return Code");
    }
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
void send_Success_Data(String box_id, String command, int success_status, String param1, String param2, String param3)
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
        doc["dt"] = param1;
        doc["st"] = param2;
        doc["bt"] = param3;
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
    EEPROM.begin(EEPROM_SIZE);
    rgb_ring_setup(110);
    oled_setup();
    sd_setup();
    radio.begin();
    pin_setup();
    wifi_ntp_setup();
    // aws_setup();
    // read_box_details_from_sd_card();
    RL.readLines(ERROR_LOG_FILE, &handleEachErrorLine);
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
    lastMillis = millis();
}

//TODO: Loop for the code
void loop()
{

    if (digitalRead(outputS) == HIGH) // Detects when the rotary encoder button is pressed
    {
        timerStop(timer);
        timerAlarmDisable(timer);
        delay(3 * DEBOUNCE_TIME); // Debounce time
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

    // if ((count != 0) && ((count % DELAY_ONE_MINUTES) == 0)) // When Count variable overflows the Timer Value, the periodic box polling is done
    // {
    //     solid_rgb_ring(YELLOW_COLOR);
    //     regular_box_update(count);
    //     if (count >= DELAY_SIXTY_MINUTES)
    //         count = 0;

    //     print_company_logo();
    // }

    if ((millis() - lastMillis) > MAIN_SCREEN_REFRESH_TIME)
    {
        lastMillis = millis();

        if (main_screen_count == 1) // Print Company Logo
            print_company_logo();

        else if (main_screen_count == 2) //Print Connected Boxes
        {
            connected_boxes = EEPROM.read(CONNECTED_BOXES_STORED_LOCATION);
            update_oled("Connected", "Boxes:", (String)connected_boxes);
        }

        else if (main_screen_count == 3) // Print WiFi Status
        {
            String wifi_connection_status = "Disconnected";
            if (WiFi.status() == WL_CONNECTED)
                wifi_connection_status = "Connected";
            update_oled("WiFi", ssid, wifi_connection_status);
        }

        else if (main_screen_count == 4) // Print Date and Time
        {
            DateTime now = rtc.now();
            year = now.year();
            month = now.month();
            seconds = now.second();
            hour = now.hour();
            minutes = now.minute();
            seconds = now.second();

            String day = (String)year + '/' + (String)month + '/' + (String)date;
            String time = (String)hour + ':' + (String)minutes + ':' + (String)seconds;
            update_oled("Date Time", day, time);
        }

        else if (main_screen_count == 5) // Print Cloud Status
        {
            String mqtt_connection_status = "Disconnected";
            if (client.connected())
                mqtt_connection_status = "Connected";
            update_oled("MQTT", mqtt_connection_status, "");
        }

        else if (main_screen_count == 6) // Print Latest Error Log
            update_oled("Latest", "Error", latest_error_log);

        if (main_screen_count >= MENU_SCREEN_LIST)
            main_screen_count = 1;
        else
            main_screen_count++;
    }
}
