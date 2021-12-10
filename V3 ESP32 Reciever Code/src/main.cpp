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
// Reference (NTP Setup): https://lastminuteengineers.com/esp32-ntp-server-date-time-tutorial/
// Reference (ESP32 Hotspot): https://lastminuteengineers.com/creating-esp32-web-server-arduino-ide/
// Reference (ESP32 Server): https://randomnerdtutorials.com/esp32-esp8266-input-data-html-form/
#include <WiFi.h>
#include "time.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);
char ssid[SSID_CHAR_LENGTH];
char password[PASSWORD_CHAR_LENGTH];
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

// HTML web page to handle 2 input fields (SSID, Password)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
    <html>

        <head>
            <title>ESP Wi-Fi Form</title>
            <meta name="viewport" content="width=device-width, initial-scale=1">
        </head>

        <body>

            <h1 align="center">Wi-Fi Login Credentials</h1>

            <form action="/get">
            SSID: <input type="text" name="ssid">
            <input type="submit" value="Submit">
            </form>
            <br>

            <form action="/get">
            Password: <input type="text" name="pwd">
            <input type="submit" value="Submit">
            </form>
            <br>
        </body>

    </html>
)rawliteral";

//! RTC DS3231 Setup
// Reference (RTC DS3231): https://how2electronics.com/esp32-ds3231-based-real-time-clock/
// Reference (RTCLib Library): //https://adafruit.github.io/RTClib/html/index.html/
#include <SPI.h>
#include <Wire.h>
#include "RTClib.h"
RTC_DS3231 rtc;

//! OLED Setup
// Reference (OLED): https://randomnerdtutorials.com/esp32-ssd1306-oled-display-arduino-ide/
// Reference (Display LOGO on OLED): Reference: https://create.arduino.cc/projecthub/Arnov_Sharma_makes/displaying-your-own-photo-on-oled-display-5a8e8b
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "COMPANY_LOGO.c"
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//! NRF24 Setup
// Reference (NRF24): https://howtomechatronics.com/tutorials/arduino/arduino-wireless-communication-nrf24l01-tutorial/
// Reference (NRF24 Interrupt): https://www.youtube.com/watch?v=vzWcBAWpTx0
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(NRF24_CE, NRF24_CSN);
byte BROADCAST_RECIEVER_ADDRESS[COMMUNICATION_ID_LENGTH + 1] = "boxit";
String BOX_DETAILS[MAX_BOXES][2];
int BOX_DT_TIME_DETAILS[MAX_BOXES];

//! SD Card Setup
// Reference (SD Card Module): https://randomnerdtutorials.com/esp32-microsd-card-arduino/
// Reference: (ReadLines Library): https://github.com/mykeels/ReadLines/blob/master/examples/print-line-and-index/print-line-only.ino
#include "FS.h"
#include "SD.h"
#include <ReadLines.h>
char AWS_BACKLOG_FILE[] = "/AWS_Backlog_file.txt";
char BOX_ID_DETAILS_FILE[] = "/Box_ID_Details.txt";
char ERROR_LOG_FILE[] = "/Error_Log.txt";
char SSID_FILE[] = "/SSID.txt";
char PASSWORD_FILE[] = "/Password.txt";
char file_extension[] = ".txt";
char line1[RL_MAX_CHARS];

//! Encoder Setup
// Reference (Encoder): https://howtomechatronics.com/tutorials/arduino/rotary-encoder-works-use-arduino/
#define outputA RE_DT
#define outputB RE_CLK
#define outputS RE_SW
#define DEBOUNCE_TIME 50
float counter = 0;
int aState;
int aLastState;

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
    {{" < Back   "}, {}},
    {{" Connected\n Boxes    "}, {" < Back   "}},
    {{" Connect  \n New WiFi "}, {" < Back   "}},
    {{" Reciever \n Details  "}, {" < Back   "}},
    {{" Set Ring \nBrightness"}, {" < Back   "}},
    {{" Error    \n Log      "}, {" < Back   "}},
    {{" Contact  \n Us       "}, {" < Back   "}},
};

//! WS2812 Neopixel Ring
// Reference (WS2812 NeoPixel LED Ring): https://create.arduino.cc/projecthub/robocircuits/neopixel-tutorial-1ccfb9
#include <Adafruit_NeoPixel.h>
#define NUMPIXELS 8 // Number of LEDs on NeoPixel
Adafruit_NeoPixel pixels(NUMPIXELS, RGB_D1, NEO_GRB + NEO_KHZ800);
#define RED_COLOR pixels.Color(255, 0, 0)
#define GREEN_COLOR pixels.Color(0, 255, 0)
#define BLUE_COLOR pixels.Color(0, 0, 255)
#define YELLOW_COLOR pixels.Color(255, 255, 0)

//! AWS Setup
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h>
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

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
#define DELAY_ONE_MINUTES 60      // 1 min periodic count
#define DELAY_FIVE_MINUTES 300    // 5 mins periodic count
#define DELAY_TEN_MINUTES 600     // 10 mins periodic count
#define DELAY_THIRTY_MINUTES 1800 // 30 mins periodic count
#define DELAY_SIXTY_MINUTES 3600  // 60 mins periodic count
#define ESP32_CLOCK 80000000      // 80MHz Clock on ESP32
#define PRESCALAR 80              // Prescalar creates a timer to set the alarm for 4 min 16 sec
hw_timer_t *timer = NULL;
/**
 * @brief The count variable increments everytime the timer overflows,
 * When count > timer delay, the polling of boxes is done.
 */
volatile int count;
int previous_count = 0; // Logs the previous time update loop was triggered

//! Setting up EEPROM
/**
 * @brief We are using EEPROM to store the number of boxes connected with the reciever
 * Reference (EEPROM): https://randomnerdtutorials.com/esp32-flash-memory/
 */
#include <EEPROM.h>
#define EEPROM_SIZE 10
#define LED_RING_BRIGHTNESS_STORED_LOCATION 5

//! Function Definition
void configure_timer(void);
void IRAM_ATTR onTime(void);
void pin_setup(void);
void connect_to_wifi(void);
void connect_to_ntp(void);
void nrf24_setup(void);
void rtc_setup(void);
void oled_setup(void);
void sd_setup(void);
void rgb_ring_setup(int led_brightness);
void aws_setup(void);
void print_company_logo(void);
void update_oled(String text1, String text2, String text3);
void oled_menu_update(String item1, String item2, String item3);
void update_splash_screen(void);
void printLocalTime(void);
void notFound(AsyncWebServerRequest *request);
void connect_to_new_wifi(void);
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
void add_new_box(String box_id, String box_address, String dt);
void calibrate_box(String box_id, byte *ptr_box_address);
void update_box_data(String box_id, byte *ptr_box_address, String required_params, int call_location);
void change_box_setting(String box_id, String ptr_box_address, String inventory_name, String dt, String bt, String st);
void sound_buzzer(String box_id, byte *ptr_box_address);
void box_initiated_call();
void write_radio(byte ptr_transmission_address[], String transmission_message);
void set_radio_in_read_mode(byte ptr_recieving_address[]);
String read_radio(void);
void regular_box_update(int counter);
String fetch_box_address(String box_id);
void read_box_details_from_sd_card(void);
void handleEachLine(char line[], int lineIndex);
void handleEachErrorLine(char line[], int lineIndex);
void handleEachSSIDLine(char line[], int lineIndex);
void handleEachPasswordLine(char line[], int lineIndex);
void write_error_log(String error_log_tag);
void messageReceived(String &topic, String &payload);
void read_aws_backlog_file();
void handleEachAWSBacklogLine(char line[], int lineIndex);
void lwMQTTErr(lwmqtt_err_t reason);
void lwMQTTErrConnection(lwmqtt_return_code_t reason);
void connectToMqtt(bool nonBlocking);
void checkWiFiThenMQTT(void);
void checkWiFiThenMQTTNonBlocking(void);
void checkWiFiThenReboot(void);
void send_Success_Data(String box_id, String command, int success_status, String param1);
void sendData(String box_id, String message, String date_time);

//! Function Declaration

// TODO: Timer Functions
/**
 * @brief Configuring Timer for Alarm.
 */
void configure_timer(void)
{
    solid_rgb_ring(YELLOW_COLOR);
    Serial.println("Setting up Timer!");
    update_oled("Setting", "Up", "Timer");
    delay(1000);
    timer = timerBegin(0, PRESCALAR, true);
    timerAttachInterrupt(timer, &onTime, true);
    timerAlarmWrite(timer, (ESP32_CLOCK / PRESCALAR), true);
    solid_rgb_ring(GREEN_COLOR);
    Serial.println("Timer Setup Done!");
    update_oled("Timer", "Setup", "Done");
    delay(1000);
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

// TODO: Basic Setup for the code
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
void connect_to_wifi(void)
{
    solid_rgb_ring(YELLOW_COLOR);
    Serial.println("Connecting to Wi-Fi");
    update_oled("Connecting", "to", "Wi-Fi");
    delay(1000);

    RL.readLines(SSID_FILE, &handleEachSSIDLine);
    RL.readLines(PASSWORD_FILE, &handleEachPasswordLine);
    Serial.println("Reading SSID & Password from MicroSD Card");
    Serial.println("Retrieved SSID: " + (String)ssid);
    Serial.println("Retrieved Password: " + (String)password);
    if ((String)ssid == "")
        connect_to_new_wifi();
    update_oled("Wi-Fi", "Connecting", (String)ssid);
    delay(1000);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    wifi_connection_status = 0;
    int connection_attempts = 0;
    int max_attempts = 50;
    while (connection_attempts < max_attempts)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            delay(100);
            Serial.print(".");
            connection_attempts++;
        }
        else
        {
            wifi_connection_status = 1;
            solid_rgb_ring(GREEN_COLOR);
            Serial.println("\nWi-Fi Connected!");
            update_oled("Wifi", "Connected", (String)ssid);
            break;
        }
    }
    if (wifi_connection_status == 0)
    {
        solid_rgb_ring(RED_COLOR);
        Serial.println("Wifi Not Connected to SSID: " + (String)ssid);
        update_oled("Wifi not", "Connected", (String)ssid);
        delay(1000);
        write_error_log(WIFI_NOT_CONNECTED); // Error Log
    }
    return;
}

void connect_to_ntp(void)
{
    rtc_setup();
    if (wifi_connection_status == 1)
    {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // Configure NTP
        printLocalTime();                                         // Fetching time from NTP
    }
    else
        rtc_get_time(); // Fetching time from RTC
    return;
}

void nrf24_setup(void)
{
    radio.begin();
}

/**
 * @brief Setup RTC
 */
void rtc_setup(void)
{
    solid_rgb_ring(YELLOW_COLOR);
    Serial.println("\nConnecting to RTC");
    update_oled("RTC", "Connection", "Progress");
    delay(1000);

    if (!rtc.begin())
    {
        solid_rgb_ring(RED_COLOR);
        Serial.println("RTC Connection Failed");
        update_oled("RTC", "Connection", "Failed");
        delay(1000);
    }

    solid_rgb_ring(GREEN_COLOR);
    Serial.println("RTC Connection Successful");
    update_oled("RTC", "Setup", "Done");
    delay(1000);
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
    Serial.println("Setting up MicroSD Card");
    update_oled("Setting", "up", "MicroSD");
    delay(1000);

    if (!SD.begin(SD_CS))
    {
        solid_rgb_ring(RED_COLOR);
        Serial.println("MicroSd Card Mount Failed");
        update_oled("MicroSD", "Mount", "Failed");
        delay(1000);
    }
    else
    {
        solid_rgb_ring(GREEN_COLOR);
        Serial.println("MicroSD Card Mount Successful");
        update_oled("MicroSD", "Mount", "Done");
        delay(1000);
    }
}

/**
 * @brief Setting up the NeoPixels LED Ring
 */
void rgb_ring_setup(int led_brightness)
{
    pixels.begin();
    if (led_brightness > 100)
        led_brightness = EEPROM.read(LED_RING_BRIGHTNESS_STORED_LOCATION);
    Serial.println("\nCurrent LED Brightness: " + (String)led_brightness + '%');
    pixels.setBrightness(led_brightness * 255 / 100);
}

/**
 * @brief Setup AWS
 *
 */
void aws_setup(void)
{
    Serial.println("Setting up AWS");
    if (wifi_connection_status == 0)
        connect_to_wifi();
    if (wifi_connection_status == 1)
    {
        solid_rgb_ring(YELLOW_COLOR);
        Serial.println("Setting up AWS Connection");
        update_oled("AWS", "Setup", "Progress");
        delay(1000);

        // Configure WiFiClientSecure to use the AWS IoT device credentials
        net.setCACert(cacert);
        net.setCertificate(client_cert);
        net.setPrivateKey(privkey);

        client.begin(MQTT_HOST, MQTT_PORT, net); // Connect to the MQTT broker on the AWS endpoint we defined earlier
        client.onMessage(messageReceived);       // Create a message handler
        connectToMqtt(false);
        read_aws_backlog_file(); // Upload the backlog data if any
        solid_rgb_ring(GREEN_COLOR);
        Serial.println("AWS Setup Complete");
        update_oled("AWS", "Setup", "Complete");
        delay(1000);
    }
    else
    {
        solid_rgb_ring(RED_COLOR);
        Serial.println("AWS Setup Failed");
        update_oled("AWS", "Setup", "Failed");
        delay(1000);
    }
}

// TODO: OLED Functions

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
    display.setTextColor(BLACK, WHITE);
    display.setCursor(0, 0);
    display.println(item1);
    display.setTextColor(WHITE);
    if (item1.length() < 11)
    {
        display.setCursor(0, SCREEN_HEIGHT / 3);
        display.println(item2);
        if (item2.length() < 11)
        {
            display.setCursor(0, 2 * SCREEN_HEIGHT / 3);
            display.println(item3);
        }
    }
    else
    {
        display.setCursor(0, 2 * SCREEN_HEIGHT / 3);
        display.println(item2);
    }
    display.display();
}

void update_splash_screen(void)
{
    lastMillis = millis();
    switch (main_screen_count)
    {
    case 1: // Printing Company Logo
        print_company_logo();
        break;

    case 2: // Shows the currently connected boxes
    {
        update_oled("Connected", "Boxes:", (String)connected_boxes);
    }
    break;

    case 3: // Checks the status of the Wi-Fi Connection
    {
        String wifi_connection_status = "Connected";
        if (WiFi.status() != WL_CONNECTED)
        {
            wifi_connection_status = "Disconn.";
            write_error_log(WIFI_NOT_CONNECTED);
            connect_to_wifi();
        }
        update_oled("WiFi", ssid, wifi_connection_status);
    }
    break;

    case 4: // Fetches latest time from RTC
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
    break;

    case 5: // Checks AWS Connection
    {
        String line2;
        String line3 = "Connected";
        if (client.connected())
            line2 = "Is";
        else
            line2 = "Not";
        update_oled("MQTT", line2, line3);
    }
    break;

    case 6: // Shows the latest Error Log
        update_oled("Latest", "Error", latest_error_log);
        break;

    default: // Set the main screen counter to zero
        main_screen_count = 0;
        break;
    }
    main_screen_count++;
    solid_rgb_ring(BLUE_COLOR);
    set_radio_in_read_mode(BROADCAST_RECIEVER_ADDRESS);
}
// TODO: WiFi and NTP Functions
/**
 * @brief Tries to get local time from NTP:
 * -> If it's successful, The year, month, date, hour, minutes, seconds is updated to global variables with time from NTP,
 * and the time is saved to RTC.
 * -> If it fails, the fuction is called rtc_get_time()
 */
void printLocalTime(void)
{
    solid_rgb_ring(YELLOW_COLOR);
    Serial.println("Updating NTP");
    update_oled("Updating", "NTP", "");
    delay(1000);
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        solid_rgb_ring(RED_COLOR);
        Serial.println("NTP Failed to Update");
        update_oled("NTP", "Update", "Failed");
        delay(1000);
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
    if (month <= 9)
        month = '0' + month;
    if (date <= 9)
        date = '0' + date;
    if (hour <= 9)
        hour = '0' + hour;
    if (minutes <= 9)
        minutes = '0' + minutes;
    if (seconds <= 9)
        seconds = '0' + seconds;

    solid_rgb_ring(GREEN_COLOR);
    Serial.println("NTP Update Successful");
    update_oled("NTP", "Update", "Success");
    delay(1000);
    String day = (String)year + "/" + (String)month + "/" + (String)date;
    String time = (String)hour + ":" + (String)minutes + ":" + (String)seconds;
    Serial.println("Time from NTP:\n" + day + ' ' + time);
    update_oled("NTP Time", day, time);
    delay(1000);

    solid_rgb_ring(YELLOW_COLOR);
    update_oled("RTC", "Update", "Progress");
    delay(1000);
    rtc.adjust(DateTime(year, month, date, hour, minutes, seconds));

    solid_rgb_ring(GREEN_COLOR);
    update_oled("RTC", "Update", "Successful");
    delay(1000);

    rtc_get_time();
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
    solid_rgb_ring(RED_COLOR);
    update_oled("Wi-Fi", "Details", "Missing");
    Serial.println("Wi-Fi Login Details Unavailable");
    delay(1000);

    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Connect", "To New", "Wi-Fi");
    Serial.println("Connecting to new Wi-Fi");
    delay(500);

    IPAddress local_ip(192, 168, 1, 1);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.mode(WIFI_AP);
    char temp_ssid[] = "Box-It Router";
    char temp_pwd[] = "123456789";
    WiFi.softAP(temp_ssid, temp_pwd);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);
    Serial.println("Connect to:");
    Serial.println("SSID: " + (String)temp_ssid);
    Serial.println("Password: " + (String)temp_pwd);
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
                    deleteFile(SD, SSID_FILE);
                    strcpy(ssid, inputMessage.c_str());
                    writeFile(SD, SSID_FILE, ssid);
                    Serial.println((String)ssid);
                    }
                    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
                else if (request->hasParam(PARAM_INPUT_2))
                {
                    inputMessage = request->getParam(PARAM_INPUT_2)->value();
                    inputParam = PARAM_INPUT_2;
                    deleteFile(SD, PASSWORD_FILE);
                    strcpy(password, inputMessage.c_str());
                    writeFile(SD, PASSWORD_FILE, password);
                    Serial.println((String)password);
                }
                else
                {
                    inputMessage = "No message sent";
                    inputParam = "none";
                }
                Serial.println(inputMessage);
                request->send(200, "text/html", "Saved " + inputParam + ": " + inputMessage + "<br><a href=\"/\">Enter Next Value</a>"); });
    server.onNotFound(notFound);
    server.begin();
    int i = 0;
    while (1)
    {
        if (digitalRead(outputS) == HIGH)
        {
            Serial.println("Restarting Device");
            update_oled("Restarting", "Device", "");
            delay(3000);
            ESP.restart();
            break;
        }
        if (i < 100)
            update_oled("SSID:", (String)temp_ssid, "");
        else if (i < 200)
            update_oled("Password:", (String)temp_pwd, "");
        else if (i < 500)
            update_oled("Connect @", "192.168.1.1", "");
        else
            i = 0;
        i++;
    }
}

// TODO: RTC Functions
/**
 * @brief Fetch Time from RTC and save it in global time variables
 *
 */
void rtc_get_time(void)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("RTC", "Update", "Progress");
    delay(1000);
    DateTime now = rtc.now();
    solid_rgb_ring(GREEN_COLOR);
    update_oled("RTC", "Fetch", "Successful");
    delay(1000);
    year = now.year();
    month = now.month();
    seconds = now.second();
    hour = now.hour();
    minutes = now.minute();
    seconds = now.second();
    String day = (String)year + "/" + (String)month + "/" + (String)date;
    String time = (String)hour + ":" + (String)minutes + ":" + (String)seconds;
    Serial.println("Time from RTC:\n" + day + ' ' + time);
    update_oled("RTC Time", day, time);
    delay(1000);
    solid_rgb_ring(GREEN_COLOR);
    update_oled("RTC", "Update", "Successful");
    delay(1000);
    return;
}

// TODO: RGB NeoPixel Functions
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
    int current_led_brightness = EEPROM.read(LED_RING_BRIGHTNESS_STORED_LOCATION);

    String item1 = "Brightness\nPercentage";
    String item2 = (String)current_led_brightness + '%';
    String item3 = "";
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
                current_led_brightness = 100; // Ensures that the LED Brightness is not above 100%
            else if (current_led_brightness <= 0)
                current_led_brightness = 0; // Ensures that the LED Brightness is not below 0%

            item2 = (String)current_led_brightness + '%';
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
                Serial.println("Saving LED Brightness to memory");
                update_oled("Saving", "To", "Memory");
                delay(1000);
                break;
            }
        }
    }
    Serial.println("Exit");
    return;
}

// TODO: MicroSD Card Functions3
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
                listDir(fs, file.name(), levels - 1);
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
    delay(1000);
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path))
    {
        Serial.println("Dir created");
        solid_rgb_ring(GREEN_COLOR);
        update_oled("Dir.", "Create", "Successful");
        delay(1000);
    }
    else
    {
        Serial.println("mkdir failed");
        solid_rgb_ring(RED_COLOR);
        update_oled("Dir.", "Create", "Failed");
        delay(1000);
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
        Serial.println("File deleted");
    else
        Serial.println("Delete failed");
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
        delay(1000);
        return;
    }
    if (file.print(message))
    {
        solid_rgb_ring(GREEN_COLOR);
        update_oled("File", "Write", "Successful");
        Serial.println("File written");
        delay(1000);
    }
    else
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("File", "Write", "Failed");
        Serial.println("Write failed");
        delay(1000);
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
        delay(1000);
        writeFile(SD, path, message);
        return;
    }
    if (file.print(message))
    {
        solid_rgb_ring(GREEN_COLOR);
        update_oled("Message", "Append", "Successful");
        Serial.println("Message appended");
        delay(1000);
    }
    else
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("Message", "Append", "Failed");
        Serial.println("Append failed");
        delay(1000);
    }
    file.close();
    return;
}

// TODO: Rotary Encoder Functions
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
    timerStop(timer);
    timerAlarmDisable(timer);
    delay(500);
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
                        menu_level = 0;
                        main_list_pos = 0;
                        sub_list_pos = 0;
                        menu_item = 0;
                    }
                }
                else
                {
                    counter = 0;
                    menu_item = 10 * main_list_pos;
                    menu_level++;
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
            counter = 0;
            switch (menu_item)
            {
            case 10: // Connected Boxes Data
                sub_list_pos = (int)counter;
                if (sub_list_pos != 0)
                    item1 = BOX_DETAILS[sub_list_pos][0];

                if ((int)counter < menu_length - 1)
                    item2 = BOX_DETAILS[sub_list_pos + 1][0];
                if ((int)counter < menu_length - 2)
                    item3 = BOX_DETAILS[sub_list_pos + 2][0];
                break;

            case 20: // Connect to New Wifi
                connect_to_new_wifi();
                menu_level = 0;
                main_list_pos = 0;
                sub_list_pos = 0;
                menu_item = 0;
                break;

            case 30: // Reciever Details
                item2 = "Rec. ID";
                item3 = RECIEVER_ID;
                break;

            case 40: // Change LED Ring Brightness
                change_led_ring_brightness();
                menu_level = 0;
                main_list_pos = 0;
                sub_list_pos = 0;
                menu_item = 0;
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
        oled_menu_update(item1, item2, item3);
    }
    print_company_logo();
    delay(500);
    timerStart(timer);
    timerAlarmEnable(timer);
    return;
}
// TODO: Menu Operation Functions

// TODO: Special Protocols
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
void add_new_box(String box_id, String str_box_address, String dt)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Adding", "New Box", box_id);
    delay(1000);

    int n = str_box_address.length();
    char char_box_address[n];
    strcpy(char_box_address, str_box_address.c_str());

    byte communication_address[COMMUNICATION_ID_LENGTH]; // Communication address to be sent for NRF Communication
    for (int i = 0; i < COMMUNICATION_ID_LENGTH; i++)
        communication_address[i] = char_box_address[i];
    Serial.println("Address: " + str_box_address);

    // Write the message in format "pair,<box_id>,<communication_address>"
    String message = box_id + ",pair" + "," + str_box_address + "," + dt;
    write_radio(BROADCAST_RECIEVER_ADDRESS, message);

    // Wait for the Box to return a message
    set_radio_in_read_mode(communication_address);
    int i = 0;
    String recieved_message = "";
    int max_attempts = 200; // TODO: Change this time as per the need
    while ((i < max_attempts))
    {
        if (radio.available())
        {
            recieved_message = read_radio();
            break;
        }
        delay(100);
        i++;
    }

    int connection_status = 0; // 0 -> Failure, 1 -> Success
    // Check if the recieved message is in format "pair,ok,<box_id>"
    String address = "";
    if (recieved_message == ("pair,ok," + box_id))
    { // TRUE
        connection_status = 1;

        // Creating char array where the data in format<box_id><box_communication_address> is saved
        String str_sd_message = box_id + "," + str_box_address + "," + dt + "\n";
        int n = str_sd_message.length();
        char sd_message[n];
        Serial.println("Writing to SD: " + str_sd_message);
        strcpy(sd_message, str_sd_message.c_str());
        appendFile(SD, BOX_ID_DETAILS_FILE, sd_message); // Appending the char array in SD Card
        read_box_details_from_sd_card();                 // Updating the box data and saving the new box in Box ID Array
        solid_rgb_ring(GREEN_COLOR);
        update_oled("Added", "New Box", box_id);
        delay(1000);
        address = str_box_address;
    }
    else // FALSE
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("Failed", "New Box", box_id);
        delay(1000);
    }
    if (!client.connected())
        checkWiFiThenMQTT();
    if (client.connected())
    {
        client.loop();
        send_Success_Data((String)box_id, "add", connection_status, address);
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
    Serial.println("Calibrating Box: " + box_id);
    update_oled("Calibrating", "Box", box_id);
    delay(1000);

    // Writing the NRF with message "cali,<box_id>"
    String message = box_id + ",cali";
    write_radio(ptr_box_address, message);

    // Check if the message recieved is in required format or not
    set_radio_in_read_mode(ptr_box_address);
    int i = 0;
    String recieved_message = "";
    int max_attempts = 100; // TODO: Change this as per the need
    while (i < max_attempts)
    {
        if (radio.available())
        {
            recieved_message = read_radio();
            break;
        }
        delay(100);
        i++;
    }

    int connection_status = 0; // 0 -> Failure, 1 -> Success
    // Check if the NRF connection was successful or not
    if (recieved_message == ("cali,ok"))
    {
        connection_status = 1;
        solid_rgb_ring(GREEN_COLOR);
        update_oled("Calibration", "Connect", "Successful");
        delay(1000);
    }
    else
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("Calibration", "Connect", "Failed");
        delay(1000);
    }

    // AWS is contacted and updated if calibration is successful or not
    if (!client.connected())
        checkWiFiThenMQTT();
    if (client.connected())
    {
        client.loop();
        send_Success_Data(box_id, "calibrate", connection_status, "");
    }

    if (connection_status == 1)
    {
        Serial.println("Waiting for 30 seconds");
        delay(30 * 1000);                               // Wait for calibration to complete
        write_radio(ptr_box_address, box_id + ",clup"); // Write to radio for updated values
        set_radio_in_read_mode(ptr_box_address);
        int i = 0;
        String update_message = "";
        int max_attempts = 50;
        while (i < max_attempts)
        {
            if (radio.available())
            {
                update_message = read_radio();
                break;
            }
            delay(100);
            i++;
        }

        if (update_message != "") // Check if the radio message is not empty
        {
            solid_rgb_ring(GREEN_COLOR);
            update_oled("Calibration", "Successful", box_id);
            delay(1000);

            // Creating File Path
            String str_file_path = "/" + (String)year + "-" + (String)month + "-" + (String)date + file_extension;
            int path_size = str_file_path.length();
            char file_path[path_size]; // Str to Char conversion
            strcpy(file_path, str_file_path.c_str());

            // Converting data into required format
            String str_sd_message = year + "/" + (String)month + "/" + (String)date + "," + (String)hour + ":" + (String)minutes + ":" + (String)seconds + "," + box_id + "," + update_message + "\n";
            int msg_size = str_sd_message.length(); // Calculating the message size
            char sd_message[msg_size];              // Str to Char conversion
            strcpy(sd_message, str_sd_message.c_str());
            appendFile(SD, file_path, sd_message);

            if (!client.connected())
                checkWiFiThenMQTT();
            if (client.connected())
            {
                client.loop();
                send_Success_Data((String)box_id, "calibration_update", connection_status, update_message);
            }
            else
                appendFile(SD, AWS_BACKLOG_FILE, sd_message);
        }
        else
        {
            connection_status = 0;
            solid_rgb_ring(RED_COLOR);
            update_oled("Calibration", "Failed", box_id);
            delay(1000);
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
void update_box_data(String box_id, byte *ptr_box_address, String required_params, int call_location)
{
    Serial.println("Updating Box");
    Serial.println("Required Params: " + required_params);
    // Write radio with the address recieved
    String write_message = box_id + ",updt," + required_params;
    write_radio(ptr_box_address, write_message);

    // Read the radio
    set_radio_in_read_mode(ptr_box_address);
    int i = 0;
    String message = "";
    int max_attempts = 100; // TODO: Change in final setting
    while (i < max_attempts)
    {
        if (radio.available())
        {
            message = read_radio();
            break;
        }
        delay(100);
        i++;
    }

    if (message != "")
    {

        // Creating File Path
        String str_file_path = "/" + (String)year + "-" + (String)month + "-" + (String)date + file_extension;
        int path_size = str_file_path.length();
        char file_path[path_size]; // Str to Char conversion
        strcpy(file_path, str_file_path.c_str());

        // Converting data into required format
        String str_sd_message = year + "/" + (String)month + "/" + (String)date + "," + (String)hour + ":" + (String)minutes + ":" + (String)seconds + "," + box_id + "," + message + "\n";
        int msg_size = str_sd_message.length(); // Calculating the message size
        char sd_message[msg_size];              // Str to Char conversion
        strcpy(sd_message, str_sd_message.c_str());
        appendFile(SD, file_path, sd_message);

        if (!client.connected())
            checkWiFiThenMQTT();
        if (client.connected())
        {
            client.loop();
            sendData(box_id, message, "na");
        }
        else
            appendFile(SD, AWS_BACKLOG_FILE, sd_message);
    }
    if (call_location == 1)
    {
        if (!client.connected())
            checkWiFiThenMQTT();
        if (client.connected())
        {
            client.loop();
            send_Success_Data(box_id, "update", 0, "");
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
void change_box_setting(String box_id, String box_address, String inventory_name, String dt, String bt, String st)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Change", "Setting", box_id);
    Serial.println("Changing Settings");
    delay(1000);

    byte communication_address[COMMUNICATION_ID_LENGTH];
    for (size_t i = 0; i < COMMUNICATION_ID_LENGTH; i++)
        communication_address[i] = box_address[i];

    // Write radio with the address recieved
    String message = box_id + ",chng";
    if (inventory_name != "null")
        message += (",in:" + inventory_name);
    if (dt != "null")
        message += (",dt:" + dt);
    if (bt != "null")
        message += (",bt:" + bt);
    if (st != "null")
        message += (",st:" + st);
    Serial.println("Changed Settings: " + message);
    write_radio(communication_address, message);
    set_radio_in_read_mode(communication_address);

    int i = 0;
    String recieved_message = "";
    int max_attempts = 50;
    while (i < max_attempts)
    {
        if (radio.available())
        {
            recieved_message = read_radio();
            break;
        }
        delay(100);
        i++;
    }

    int success_code = 0;
    if (recieved_message == "chng,ok")
    {
        success_code = 1;
        solid_rgb_ring(GREEN_COLOR);
        update_oled("Change", "Successful", box_id);
        delay(1000);
        if (dt != "")
        {
            deleteFile(SD, BOX_ID_DETAILS_FILE); // Deletes the file with connected boxes
            for (size_t i = 0; i < connected_boxes; i++)
            {
                if (box_id == BOX_DETAILS[i][0]) // Checks the connected boxes
                {
                    int n = dt.length();
                    char char_dt[n];
                    strcpy(char_dt, dt.c_str());
                    BOX_DT_TIME_DETAILS[i] = atoi(char_dt);
                }
                String file_message = BOX_DETAILS[i][0] + "," + BOX_DETAILS[i][1] + "," + (String)BOX_DT_TIME_DETAILS[i] + "\n";
                int n = file_message.length();
                char char_file_message[n];
                strcpy(char_file_message, file_message.c_str());
                writeFile(SD, BOX_ID_DETAILS_FILE, char_file_message); // Rewrite the new data in file
                read_box_details_from_sd_card();
            }
        }
    }
    else
    {
        solid_rgb_ring(RED_COLOR);
        update_oled("Change", "Failed", box_id);
        delay(1000);
    }

    // Update to AWS
    if (!client.connected())
        checkWiFiThenMQTT();
    if (client.connected())
    {
        client.loop();
        send_Success_Data(box_id, "change", success_code, "");
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
    delay(1000);
    String message = box_id + ",buzz";
    write_radio(ptr_box_address, message);
    solid_rgb_ring(GREEN_COLOR);
    update_oled("Buzz", "Box", "Successful");
    delay(1000);
}

void box_initiated_call()
{
    Serial.println("Box Initiated Communication on NRF");
    String msg = read_radio();
    Serial.println(msg);

    String box_id, param;
    String token = ",";
    int i;
    for (i = 0; i < msg.length(); i++)
    {
        if (msg.substring(i, i + 1) == token)
        {
            box_id = msg.substring(0, i);
            break;
        }
    }
    param = msg.substring(i + 1, msg.length());
    String str_box_address = fetch_box_address(box_id); // Checks the box id is in database for address
    if (str_box_address.length() == COMMUNICATION_ID_LENGTH)
    {
        byte box_address[COMMUNICATION_ID_LENGTH];
        for (int i = 0; i < COMMUNICATION_ID_LENGTH; i++)
            box_address[i] = str_box_address[i];
        Serial.println("Box ID: " + box_id);
        Serial.println("Address: " + str_box_address);
        Serial.println("param: " + param);

        if (param == "updt")
            update_box_data(box_id, box_address, PERIODIC_UPDATE_VALUES, 0);
        else if (param == "cali")
            calibrate_box(box_id, box_address);
    }
    set_radio_in_read_mode(BROADCAST_RECIEVER_ADDRESS);
    return;
}

// TODO: NRF24 Functions
/**
 * @brief Write a message to the NRF Radio
 *
 * @param transmission_address 5 byte Transmission Address
 * @param transmission_message 32 byte Transmission Message
 */
void write_radio(byte ptr_transmission_address[], String str_transmission_message)
{
    update_oled("Sending", "Message", "via NRF");

    // TODO: Delete this part later on
    char address[COMMUNICATION_ID_LENGTH];
    for (size_t i = 0; i < COMMUNICATION_ID_LENGTH; i++)
        address[i] = ptr_transmission_address[i];
    Serial.println("Writing Message at:" + (String)address);
    // TODO: Delete till here

    int n = str_transmission_message.length();
    char transmission_message[n];
    strcpy(transmission_message, str_transmission_message.c_str());
    Serial.println("Writing Message via NRF: " + (String)transmission_message);
    radio.openWritingPipe(ptr_transmission_address);
    radio.setPALevel(RF24_PA_MIN);
    radio.stopListening();
    delay(100);
    radio.write(&transmission_message, sizeof(transmission_message));
    delay(50);
    update_oled("NRF", "Message", "Sent");
    return;
}

void set_radio_in_read_mode(byte ptr_recieving_address[])
{
    radio.openReadingPipe(0, ptr_recieving_address);
    radio.setPALevel(RF24_PA_MIN);
    radio.startListening();

    // TODO: Delete this part later on
    char address[COMMUNICATION_ID_LENGTH];
    for (size_t i = 0; i < COMMUNICATION_ID_LENGTH; i++)
        address[i] = ptr_recieving_address[i];
    Serial.println("Listening Message at:" + (String)address);
    // TODO: Delete till here

    delay(100);
}

String read_radio(void)
{
    update_oled("Recieved", "Message", "via NRF");
    delay(1000);
    char recieved_message[32];
    radio.read(&recieved_message, sizeof(recieved_message));
    Serial.println("Recieved Message via NRF: " + (String)recieved_message);
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
    update_oled("Box", "Polling", "Start");
    delay(1000);
    rtc_get_time();
    for (int i = 0; i < connected_boxes; i++)
    {
        halt_rgb_ring(i);
        String box_id = BOX_DETAILS[i][0];
        String address = BOX_DETAILS[i][1];
        int dt = BOX_DT_TIME_DETAILS[i];
        Serial.println("Box ID: " + box_id + "\tAddress: " + address + "\tdt: " + (String)dt);

        if (counter % (dt * 60) == 0)
        {
            byte box_address[COMMUNICATION_ID_LENGTH];
            for (size_t i = 0; i < COMMUNICATION_ID_LENGTH; i++)
                box_address[i] = address[i];
            update_oled("Contacting", "Box", box_id);
            update_box_data(box_id, box_address, PERIODIC_UPDATE_VALUES, 0);
        }
    }

    solid_rgb_ring(GREEN_COLOR);
    update_oled("Box", "Polling", "Completed");
    delay(1000);
    timerStart(timer);
    timerAlarmEnable(timer);
    print_company_logo();
    set_radio_in_read_mode(BROADCAST_RECIEVER_ADDRESS);
}

/**
 * @brief Fetch Box Address from the Box ID Array matrix
 *
 * @param box_id 9 byte Box ID
 * @return String 5 byte Box Communication Address
 */
String fetch_box_address(String box_id)
{
    Serial.println("Fetching Address");
    Serial.println("Connected Boxes: " + (String)connected_boxes);
    String box;
    String address;
    for (int i = 0; i < connected_boxes; i++)
    {
        box = BOX_DETAILS[i][0];
        if (box == box_id)
        {
            address = BOX_DETAILS[i][1];
            break;
        }
    }
    return address;
}

// TODO: Auxillary Functions
/**
 * @brief Reading the SD Card File which stores the "Box ID and communication Address"
 */
void read_box_details_from_sd_card(void)
{
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Read", "Box Details", "From SD");
    Serial.println("Reading Box Details");
    delay(1000);
    RL.readLines(BOX_ID_DETAILS_FILE, &handleEachLine);
    solid_rgb_ring(GREEN_COLOR);
    update_oled("Box Details", "Read", "Done");
    delay(1000);
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
    String params[3]; // Box id, box address, box dt
    String token = ",";
    String sd_message = (String)line;
    Serial.println("Line: " + sd_message);
    int j = 0;
    int last_val = 0;
    int i;
    for (i = 0; i < sd_message.length(); i++)
    {
        if (sd_message.substring(i, i + 1) == token)
        {
            params[j] = sd_message.substring(last_val, i);
            j++;
            last_val = i + 1;
        }
    }
    params[j] = sd_message.substring(last_val, i);
    BOX_DETAILS[lineIndex][0] = params[0];
    BOX_DETAILS[lineIndex][1] = params[1];
    int n = params[2].length();
    char dt[n];
    strcpy(dt, params[2].c_str());
    BOX_DT_TIME_DETAILS[lineIndex] = atoi(dt);
    Serial.println("Box ID: " + BOX_DETAILS[lineIndex][0] + '\n' + "Box Address: " + BOX_DETAILS[lineIndex][1] + '\n' + "Delay Time: " + BOX_DT_TIME_DETAILS[lineIndex]);
    connected_boxes = lineIndex + 1;
    Serial.println("Connected Boxes: " + (String)connected_boxes);
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
    latest_error_log = (String)line;
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
    for (size_t i = 0; i < SSID_CHAR_LENGTH; i++)
        ssid[i] = line[i];
}
/**
 * @brief Read Line by Line for Password File
 *
 * @param line
 * @param lineIndex
 */
void handleEachPasswordLine(char line[], int lineIndex)
{
    for (size_t i = 0; i < PASSWORD_CHAR_LENGTH; i++)
        password[i] = line[i];
}

/**
 * @brief Write Error Log to SD Card in ERROR_LOG_FILE
 * Format: YYYY/MM/DD-HH:MM:SS-#<XXXX>
 * <XXXX> - Error Code
 * @param error_log_tag
 */
void write_error_log(String error_log_tag)
{
    // Writing Error
    String str_error_log = (String)hour + ':' + (String)minutes + ' ' + error_log_tag;
    latest_error_log = str_error_log;
    char error_log[ERROR_LOG_LENGTH];
    strcpy(error_log, latest_error_log.c_str());
    writeFile(SD, ERROR_LOG_FILE, error_log);
}

// TODO: AWS Functions

/**
 * @brief This function is activated when the message is recieved from AWS
 *
 * @param topic
 * @param payload
 */
void messageReceived(String &topic, String &payload)
{ // Reference (JSON to String Converter): https://github.com/bblanchon/ArduinoJson/blob/6.x/examples/JsonParserExample/JsonParserExample.ino
    timerStop(timer);
    timerAlarmDisable(timer);
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("Message", "Recieved", "from AWS");
    delay(1000);

    Serial.println("Recieved:\n" + payload);
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        Serial.println("deserializeJson() failed: " + (String)error.f_str());
        return;
    }

    String box_id = doc["boxid"];
    String cmd = doc["cmd"];
    update_oled(box_id, cmd, "");
    delay(1000);
    // Perform action as per the command from AWS
    if (cmd == "add") // Add new Box
    {
        String box_address = doc["boxaddress"];
        String dt = doc["dt"];
        add_new_box(box_id, box_address, dt);
    }
    else
    {
        String address = fetch_box_address(box_id);
        Serial.println("Address: " + address);
        byte box_address[COMMUNICATION_ID_LENGTH];
        for (size_t i = 0; i < COMMUNICATION_ID_LENGTH; i++)
            box_address[i] = address[i];

        if (cmd == "calibrate") // Calibrate box parametersśś
            calibrate_box(box_id, box_address);
        else if (cmd == "ask") // Ask box for updated values
        {
            String required_params = doc["identifier"];
            rtc_get_time();
            solid_rgb_ring(YELLOW_COLOR);
            update_oled("Update", "Box", "Progress");
            update_box_data(box_id, box_address, required_params, 1);
            solid_rgb_ring(GREEN_COLOR);
            update_oled("Update", "Box", "Successful");
        }
        else if (cmd == "change_setting") // Change box settings
        {
            String in = doc["in"];
            String dt = doc["dt"];
            String bt = doc["bt"];
            String st = doc["st"];
            change_box_setting(box_id, address, in, dt, bt, st);
        }
        else if (cmd == "buzz")
            sound_buzzer(box_id, box_address);
    }
    solid_rgb_ring(GREEN_COLOR);
    update_oled("AWS", "MSG Recieved", "Complete");
    timerStart(timer);
    timerAlarmEnable(timer);
}

void read_aws_backlog_file()
{
    Serial.println("Reading Backlog Files");
    RL.readLines(AWS_BACKLOG_FILE, &handleEachAWSBacklogLine);
    deleteFile(SD, AWS_BACKLOG_FILE);
}

void handleEachAWSBacklogLine(char line[], int lineIndex)
{
    String params[4]; // date, time, boxid, message
    String token = ",";
    String sd_message = (String)line;
    Serial.println("Line: " + sd_message);
    int j = 0;
    int last_val = 0;
    int i;
    for (i = 0; i < sd_message.length(); i++)
    {
        if (sd_message.substring(i, i + 1) == token)
        {
            params[j] = sd_message.substring(last_val, i);
            j++;
            last_val = i + 1;
            if (j == 3)
                break;
        }
    }
    params[j] = sd_message.substring(last_val, sd_message.length());

    if (!client.connected())
        checkWiFiThenMQTT();
    if (client.connected())
    {
        client.loop();
        sendData(params[2], params[3], params[0] + " " + params[1]);
    }
    return;
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
    int tries = 0;
    int max_tries = 50;
    while ((!client.connected()) && (tries < max_tries))
    {
        tries++;
        if (client.connect(THINGNAME))
        {
            Serial.println("connected!");
            if (!client.subscribe(MQTT_SUB_TOPIC))
                lwMQTTErr(client.lastError());
        }
        else
        {
            Serial.println("failed, reason -> ");
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

void checkWiFiThenMQTT(void)
{
    if (WiFi.status() != WL_CONNECTED)
        connect_to_wifi();
    if (WiFi.status() == WL_CONNECTED)
        connectToMqtt(false);
}

unsigned long previousMillis = 0;
const long interval = 5000;

void checkWiFiThenMQTTNonBlocking(void)
{
    connect_to_wifi();
    if (millis() - previousMillis >= interval && !client.connected())
    {
        previousMillis = millis();
        connectToMqtt(true);
    }
}

void checkWiFiThenReboot(void)
{
    connect_to_wifi();
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
void send_Success_Data(String box_id, String command, int success_status, String param1)
{
    // Reference (String to JSON Converter):https://github.com/bblanchon/ArduinoJson/blob/6.x/examples/JsonGeneratorExample/JsonGeneratorExample.ino
    solid_rgb_ring(YELLOW_COLOR);
    update_oled("AWS", "Upload", "InProgess");
    StaticJsonDocument<200> doc;
    doc["boxid"] = box_id;
    doc["cmd"] = command;
    doc["success"] = success_status;
    if (command == "add")
        doc["boxaddress"] = param1;
    else if (command == "calibration_update")
    {
        // Seperating the comma seperated values from message in format "<param_letter>:<param_value>"
        Serial.println("Calibration update Parameters: " + param1);
        String formatted_params[UPDATE_PARAMETERS];
        String token = ",";
        int string_index = 0;
        int last_val_pos = 0;
        int array_index = 0;
        for (string_index; string_index < param1.length(); string_index++)
        {
            if (param1.substring(string_index, string_index + 1) == token)
            {
                formatted_params[array_index] = param1.substring(last_val_pos, string_index);
                Serial.println(formatted_params[array_index]);
                array_index++;
                last_val_pos = string_index + 1;
            }
        }
        formatted_params[array_index] = param1.substring(last_val_pos, string_index);
        Serial.println(formatted_params[array_index]);
        array_index++;

        // Seperating the colon seperated value and identifier and storing it in different arrays
        String token_2 = ":";
        for (int i = 0; i < array_index; i++)
        {
            String param = formatted_params[i];
            for (int j = 0; j < param.length(); j++)
            {
                if (param.substring(j, j + 1) == token_2)
                {
                    String updated_params[2];
                    updated_params[0] = param.substring(0, j);
                    updated_params[1] = param.substring(j + 1);
                    doc[updated_params[0]] = updated_params[1]; // Converting the data into JSON Format
                    Serial.println(updated_params[0] + ":" + updated_params[1]);
                }
            }
        }
    }

    serializeJson(doc, Serial);
    serializeJsonPretty(doc, Serial);
    // The above line prints:
    // {
    //   "Box ID": "123456789",
    //   "cmd": add,
    //   "success": 1,
    //   "param1": "param1_data",
    // }
    char shadow[measureJson(doc) + 1];
    serializeJson(doc, shadow, sizeof(shadow));

    int tries = 0;
    int max_tries = 30;
    while ((!client.publish(MQTT_PUB_TOPIC, shadow, false, 0)) && (tries < max_tries))
    {
        lwMQTTErr(client.lastError());
        update_oled("AWS", "update", "Failure");
        delay(50);
        tries++;
    }
}

/**
 * @brief Upload data to AWS for periodic box updates
 *
 * @param box_id
 * @param message
 */
void sendData(String box_id, String message, String date_time)
{
    // Reference:https://github.com/bblanchon/ArduinoJson/blob/6.x/examples/JsonGeneratorExample/JsonGeneratorExample.ino
    Serial.println("Sending to AWS");
    StaticJsonDocument<300> doc;
    if (date_time == "na")
        doc["time"] = (String)year + '/' + (String)month + '/' + (String)date + ' ' + (String)hour + ':' + (String)minutes + ':' + (String)seconds;
    else
        doc["time"] = date_time;
    doc["boxid"] = box_id;
    doc["cmd"] = "update";

    // Seperating the comma seperated values from message in format "<param_letter>:<param_value>"
    String formatted_params[UPDATE_PARAMETERS];
    String token = ",";
    int i = 0;
    int last_val = 0;
    int j = 0;
    for (i = 0; i < message.length(); i++)
    {
        if (message.substring(i, i + 1) == token)
        {
            formatted_params[j] = message.substring(last_val, i);
            j++;
            last_val = i + 1;
        }
    }
    formatted_params[j] = message.substring(last_val, i);
    j++;

    // Seperating the colon seperated value and identifier and storing it in different arrays
    String token_2 = ":";
    for (int i = 0; i < j; i++)
    {
        String param = formatted_params[i];
        for (int j = 0; j < param.length(); j++)
        {
            if (param.substring(j, j + 1) == token_2)
            {
                String updated_params[2];
                updated_params[0] = param.substring(0, j);
                updated_params[1] = param.substring(j + 1);
                Serial.println(updated_params[0] + ":" + updated_params[1]);
                doc[updated_params[0]] = updated_params[1]; // Converting the data into JSON Format
            }
        }
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

    int tries = 0;
    int max_tries = 30;
    while ((!client.publish(MQTT_PUB_TOPIC, shadow, false, 0)) && (tries < max_tries))
    {
        lwMQTTErr(client.lastError());
        update_oled("AWS", "update", "Failure");
        delay(50);
        tries++;
    }
}

// TODO: Basic Setup, Code Operation Starts here
void setup()
{
    Serial.begin(115200);
    pin_setup();
    EEPROM.begin(EEPROM_SIZE);
    rgb_ring_setup(110);
    oled_setup();
    nrf24_setup();
    sd_setup();
    connect_to_wifi();
    connect_to_ntp();
    aws_setup();
    read_box_details_from_sd_card();
    RL.readLines(ERROR_LOG_FILE, &handleEachErrorLine);
    if (BOX_DETAILS[0][0] == NULL)
    {
        solid_rgb_ring(RED_COLOR);
        if (wifi_connection_status == 0)
        {
            update_oled("Wifi", "Not", "Connected");
            delay(1000);
            connect_to_new_wifi();
        }
        update_oled("No", "Box", "Linked");
        Serial.println("No Box Linked");
    }
    configure_timer();
    timerAlarmEnable(timer);
    print_company_logo();
    lastMillis = millis();
    set_radio_in_read_mode(BROADCAST_RECIEVER_ADDRESS);
}

// TODO: Loop for the code
void loop()
{
    // //? Detects when the rotary encoder button is pressed
    if (digitalRead(outputS) == HIGH)
    {
        delay(3 * DEBOUNCE_TIME); // Debounce time
        if (digitalRead(outputS) == HIGH)
            read_rotary_encoder(); // Open Main Menu
    }

    //? When Count variable overflows the Timer Value, the periodic box polling is done
    if ((count != 0) && ((count % DELAY_ONE_MINUTES) == 0))
        if (previous_count != count)
        {
            regular_box_update(count);
            if (count >= DELAY_SIXTY_MINUTES)
                count = 0;
            previous_count = count;
        }

    // //? Main Splash Screen Cycle
    if ((millis() - lastMillis) > MAIN_SCREEN_REFRESH_TIME)
        update_splash_screen();

    //? Polls NRF24 to collect live data from boxes
    if (radio.available())
    {
        box_initiated_call();
        delay(500);
    }

    //? Polling AWS for recieved data
    if (WiFi.status() != WL_CONNECTED)
        connect_to_new_wifi();
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!client.connected())
            checkWiFiThenMQTT();
        if (client.connected())
            client.loop();
    }
}