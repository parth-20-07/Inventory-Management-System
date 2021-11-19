#include <Arduino.h>

//! Include Header Files
#include "PIN_CONNECTION.h"
#include "CONFIGURATION.h"

//! WiFi and NTP Setup
//Reference: https://lastminuteengineers.com/esp32-ntp-server-date-time-tutorial/
#include <WiFi.h>
#include "time.h"
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
const char *ntpServer = "asia.pool.ntp.org";
const long gmtOffset_sec = 19800; // +5:30 = (5*60*60) + (30*60) = 19800
const int daylightOffset_sec = 0; // India doesn't observe DayLight Saving
struct time_struct
{
    uint16_t year;
    uint8_t month;
    uint8_t date;
    uint8_t hour;
    uint8_t minutes;
    uint8_t seconds;
};
struct time_struct latest_time;

//! RTC DS3231 Setup
//Reference: https://how2electronics.com/esp32-ds3231-based-real-time-clock/
#include <SPI.h>
#include <Wire.h>
#include "RTClib.h" //https://adafruit.github.io/RTClib/html/index.html
RTC_DS3231 rtc;

//! OLED Setup
//Reference: https://randomnerdtutorials.com/esp32-ssd1306-oled-display-arduino-ide/
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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
//Reference: https://howtomechatronics.com/tutorials/arduino/arduino-wireless-communication-nrf24l01-tutorial/
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
//Reference: https://randomnerdtutorials.com/esp32-microsd-card-arduino/
#include "FS.h"
#include "SD.h"
#include <ReadLines.h> //https://github.com/mykeels/ReadLines/blob/master/examples/print-line-and-index/print-line-only.ino
const char AWS_BACKLOG_FILE[] = "AWS_Backlog_file.txt";
const char NRF_COMMUNICATION_ADDRESS_FILE[] = "NRF_Communication_file.txt";
const char BOX_ID_DETAILS_FILE[] = "Box_ID_Details.txt";
const char file_extension[] = ".txt";

//! Encoder Setup
//Reference: https://howtomechatronics.com/tutorials/arduino/rotary-encoder-works-use-arduino/
#define outputA RE_DT
#define outputB RE_CLK
#define outputS RE_SW
float counter = 0;
int aState;
int aLastState;

//! WS2812 Neopixel Ring
//Reference: https://create.arduino.cc/projecthub/robocircuits/neopixel-tutorial-1ccfb9
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

//! Function Definition
void rtc_setup();
void update_oled(String text1, String text2, String text3);
void halt_rgb_ring(int led_number);
void solid_rgb_ring(uint32_t color);
void send_Success_Data(String box_id, String command, int success_status, String param1, String param2);
void sendData(String message);
void connectToMqtt(bool nonBlocking);
void messageReceived(String &topic, String &payload);

//! Function Declaration

int printLocalTime()
{
    solid_rgb_ring(RED_COLOR);
    update_oled("Updating", "NTP", "");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        update_oled("NTP", "Update", "Failed");
        solid_rgb_ring(GREEN_COLOR);
        return EXIT_FAILURE;
    }
    update_oled("NTP", "Update", "Success");
    latest_time.year = timeinfo.tm_year - 100;
    latest_time.month = timeinfo.tm_mon + 1;
    latest_time.date = timeinfo.tm_mday;
    update_oled((String)latest_time.year, (String)latest_time.month, (String)latest_time.date);
    Serial.println((String)latest_time.year + '/' + (String)latest_time.month + '/' + (String)latest_time.date);
    latest_time.hour = timeinfo.tm_hour;
    latest_time.minutes = timeinfo.tm_min;
    latest_time.seconds = timeinfo.tm_sec;
    update_oled((String)latest_time.hour, (String)latest_time.minutes, (String)latest_time.seconds);
    Serial.println((String)latest_time.hour + ':' + (String)latest_time.minutes + ':' + (String)latest_time.seconds);
    rtc_setup();
    return EXIT_SUCCESS;
}

void wifi_ntp_setup(void)
{
    //connect to WiFi
    solid_rgb_ring(RED_COLOR);
    Serial.printf("Connecting to %s ", ssid);
    update_oled("Wifi", "Connecting", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(50);
        Serial.print(".");
    }
    Serial.println(" CONNECTED");
    update_oled("Wifi", "Connected", ssid);
    update_oled("Connecting", "NTP", ""); //init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    solid_rgb_ring(GREEN_COLOR);
    printLocalTime();
}

void rtc_setup()
{
    solid_rgb_ring(RED_COLOR);
    update_oled("RTC", "Connecting", "");
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        while (1)
            ;
        rtc.adjust(DateTime(latest_time.year, latest_time.month, latest_time.date, latest_time.hour, latest_time.minutes, latest_time.seconds));
    }
    update_oled("RTC", "Updated", "");
    solid_rgb_ring(GREEN_COLOR);
}

void rtc_get_time()
{
    solid_rgb_ring(RED_COLOR);
    update_oled("NTP", "Updating", "");
    DateTime now = rtc.now();
    latest_time.year = now.year();
    latest_time.month = now.month();
    latest_time.seconds = now.second();
    latest_time.hour = now.hour();
    latest_time.minutes = now.minute();
    latest_time.seconds = now.second();
    solid_rgb_ring(GREEN_COLOR);
}

void oled_setup()
{
    solid_rgb_ring(RED_COLOR);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
    display.clearDisplay();
    display.setTextColor(WHITE);
    //display.startscrollright(0x00, 0x0F);
    display.setTextSize(OLED_MENU_TEXT_SIZE);
    display.setCursor(0, 5);
    display.print("Initializing...");
    display.display();
    delay(1000);
    solid_rgb_ring(GREEN_COLOR);
}

void update_oled(String text1, String text2, String text3)
{
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
}

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
}

void write_radio(char transmission_address[], String transmission_message)
{
    update_oled("Sending", "Message", "via NRF");
    byte address[COMMUNICATION_ID_LENGTH];
    for (int i = 0; i < COMMUNICATION_ID_LENGTH; i++)
    {
        address[i] = transmission_address[i];
    }
    radio.openWritingPipe(address);
    radio.setPALevel(RF24_PA_MIN);
    radio.stopListening();
    radio.write(&transmission_message, sizeof(transmission_message));
    delay(100);
}

String read_radio(char recieving_address[])
{
    update_oled("Recieving", "Message", "via NRF");
    byte address[COMMUNICATION_ID_LENGTH];
    for (size_t i = 0; i < COMMUNICATION_ID_LENGTH; i++)
    {
        address[i] = recieving_address[i];
    }
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
    return recieved_message;
}

void createDir(fs::FS &fs, const char *path)
{
    solid_rgb_ring(RED_COLOR);
    update_oled("Creating", "Dir. in", "MicroSD");
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path))
    {
        Serial.println("Dir created");
    }
    else
    {
        Serial.println("mkdir failed");
    }
    solid_rgb_ring(GREEN_COLOR);
}

int readFile(fs::FS &fs, const char *path)
{
    solid_rgb_ring(RED_COLOR);
    update_oled("Reading", "File from", "MicroSD");
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return EXIT_FAILURE;
    }

    Serial.print("Read from file: ");
    while (file.available())
    {
        Serial.write(file.read());
    }
    file.close();
    solid_rgb_ring(GREEN_COLOR);
    return EXIT_SUCCESS;
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
    solid_rgb_ring(RED_COLOR);
    update_oled("Writinging", "File to", "MicroSD");
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("File written");
    }
    else
    {
        Serial.println("Write failed");
    }
    file.close();
    solid_rgb_ring(GREEN_COLOR);
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
    solid_rgb_ring(RED_COLOR);
    update_oled("Appending", "File to", "MicroSD");
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("Failed to open file for appending");
        writeFile(SD, path, message);
        return;
    }
    if (file.print(message))
    {
        Serial.println("Message appended");
    }
    else
    {
        Serial.println("Append failed");
    }
    file.close();
    solid_rgb_ring(GREEN_COLOR);
}

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

void removeDir(fs::FS &fs, const char *path)
{
    Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path))
    {
        Serial.println("Dir removed");
    }
    else
    {
        Serial.println("rmdir failed");
    }
}

void sd_setup()
{
    solid_rgb_ring(RED_COLOR);
    update_oled("Setting", "up", "MicroSD");
    if (!SD.begin(SD_CS))
    {
        Serial.println("Card Mount Failed");
        update_oled("MicroSD", "Mount", "Failed");
        return;
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    createDir(SD, "/mydir");
    writeFile(SD, "/hello.txt", "Hello ");
    appendFile(SD, "/hello.txt", "World!\n");
    readFile(SD, "/hello.txt");
    readFile(SD, "/foo.txt");
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
    solid_rgb_ring(GREEN_COLOR);
}

void read_rotary_encoder()
{
    solid_rgb_ring(BLUE_COLOR);
    //TODO Pause all timers
    update_oled("Starting", "to read", "Rotary Enc.");
    String item1, item2, item3;
    int main_list_pos = 0;
    int sub_list_pos = 0;
    int menu_level = 1;
    int menu_item = 0;
    int menu_length = MAIN_MENU_LIST;
    oled_menu_update(main_menu[0][0][0], main_menu[1][0][0], main_menu[2][0][0]);
    aLastState = digitalRead(outputA);
    while (1)
    {
        item1 = "";
        item2 = "";
        item3 = "";
        aState = digitalRead(outputA); // Reads the "current" state of the outputA
        if (aState != aLastState)      // If the previous and the current state of the outputA are different, that means a Pulse has occured
        {
            if (digitalRead(outputB) != aState) // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
            {
                counter += 0.5;
            }
            else
            {
                counter -= 0.5;
            }

            if ((int)counter >= menu_length - 1)
            {
                counter = menu_length - 1;
            }
            else if ((int)counter < 0)
            {
                counter = 0;
            }

            if (menu_level == 1)
            {
                menu_length = MAIN_MENU_LIST;
                main_list_pos = (int)counter;
                item1 = main_menu[main_list_pos][0][0];

                if (main_list_pos < (menu_length - 1))
                {
                    item2 = main_menu[main_list_pos + 1][0][0];
                }

                if (main_list_pos < menu_length - 2)
                {
                    item3 = main_menu[main_list_pos + 2][0][0];
                }
                menu_item = 10 * (int)counter;
            }

            else if (menu_level == 2)
            {
                sub_list_pos = (int)counter;
                item1 = main_menu[main_list_pos][1][sub_list_pos];

                if (sub_list_pos < (menu_length - 1))
                {
                    item2 = main_menu[main_list_pos][1][sub_list_pos + 1];
                }

                if (sub_list_pos < (menu_length - 2))
                {
                    item3 = main_menu[main_list_pos][1][sub_list_pos + 2];
                }
                menu_item += (int)counter;
            }
            oled_menu_update(item1, item2, item3);
            aLastState = aState; // Updates the previous state of the outputA with the current state
        }
        if (digitalRead(outputS) == HIGH)
        {
            if ((int)counter == 0)
            {
                main_list_pos = 0;
                oled_menu_update(main_menu[0][0][0], main_menu[1][0][0], main_menu[2][0][0]);
                counter = 0;
                if (menu_level == 1)
                {
                    break;
                }
                else if (menu_level == 2)
                {
                    menu_level = 1;
                }
            }
            else
            {
                menu_length = SUB_MENU_LIST;
                oled_menu_update(main_menu[(int)counter][1][0], main_menu[(int)counter][1][1], main_menu[(int)counter][1][2]);
                counter = 0;
                menu_level++;
                Serial.println(menu_level);
            }
            delay(500);
        }
    }
    solid_rgb_ring(GREEN_COLOR);
}

void solid_rgb_ring(uint32_t color)
{
    pixels.clear();
    for (size_t i = 0; i < NUMPIXELS; i++)
    {
        pixels.setPixelColor(i, color);
    }
    pixels.show();
    delay(1000);
}

void halt_rgb_ring(int led_number)
{
    pixels.clear();
    led_number = led_number % NUMPIXELS;
    pixels.setPixelColor(led_number, YELLOW_COLOR);
    pixels.show();
}

void pin_setup()
{
    pinMode(outputA, INPUT);
    pinMode(outputB, INPUT);
    pinMode(outputS, INPUT);
}

void rgb_ring_setup()
{

    pixels.begin();
    pixels.setBrightness(25);
}

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
}

// void read_box_details_from_sd_card()
// {
//     char line1[RL_MAX_CHARS];
//     RL.readLines(BOX_ID_DETAILS_FILE, &handleEachLine);
// }

void add_new_box(String box_id_details)
{

    srand(time(0));

    char box_id[BOX_ID_LENGTH];
    for (int i = 0; i < BOX_ID_LENGTH; i++)
    {
        box_id[i] = box_id_details[i];
    }

    char address[COMMUNICATION_ID_LENGTH];
    for (int i = 0; i < COMMUNICATION_ID_LENGTH; i++)
    {
        address[i] = alphabet[rand() % (AVAILABLE_CHARACTERS - 1)];
    }

    String message = "pair," + (String)box_id + "," + (String)address;
    write_radio(BROADCAST_RECIEVER_ADDRESS, message);

    String recieved_message = read_radio(address);
    int connection_status = 0; // 0 -> Failure, 1 -> Success

    if (recieved_message = ("pair,ok," + (String)box_id))
    {
        connection_status = 1;
        char sd_message[BOX_ID_LENGTH + COMMUNICATION_ID_LENGTH];
        for (int i = 0; i < (BOX_ID_LENGTH + COMMUNICATION_ID_LENGTH); i++)
        {
            if (i < BOX_ID_LENGTH)
            {
                sd_message[i] = box_id[i];
            }
            else
            {
                sd_message[i] = address[i - BOX_ID_LENGTH];
            }
        }
        appendFile(SD, BOX_ID_DETAILS_FILE, sd_message);
    }
    send_Success_Data((String)box_id, "add", connection_status, (String)address, "");
}

void calibrate_box(String box_id_details)
{
    char box_id[BOX_ID_LENGTH];
    for (int i = 0; i < BOX_ID_LENGTH; i++)
    {
        box_id[i] = box_id_details[i];
    }

    char address[COMMUNICATION_ID_LENGTH];
    for (int i = 0; i < COMMUNICATION_ID_LENGTH; i++)
    {
        // TODO: Get communication address from saved data table
    }

    String message = "cali";
    write_radio(address, message);

    String recieved_message = read_radio(address);
    int connection_status = 0; // 0 -> Failure, 1 -> Success

    if (recieved_message = ("cali,ok"))
    {
        connection_status = 1;
    }

    send_Success_Data((String)box_id, "calibrate", connection_status, "", "");
    delay(30 * 1000);

    write_radio(address, "calibration_data_update");
    String update_message = read_radio(address);
    send_Success_Data((String)box_id, "calibration_update", connection_status, update_message, "");
}

void update_box_data(String box_id, String nrf_communication_address = "")
{
    if (nrf_communication_address == NULL)
    {
        //TODO: Search in table
    }
    char address[COMMUNICATION_ID_LENGTH];
    for (int i = 0; i < COMMUNICATION_ID_LENGTH; i++)
    {
        address[i] = nrf_communication_address[i];
    }

    String message = "updt";
    write_radio(address, message);

    String recieved_message = box_id + read_radio(address);
    sendData(recieved_message);
}

void change_box_setting(String box_id_details, String dt, String st, String bt)
{
    //TODO: Search in table for communicaiton id

    char address[COMMUNICATION_ID_LENGTH];

    String message = "chng," + dt + st + bt;
    write_radio(address, message);
    int success_code = 0;
    String recieved_message = read_radio(address);
    if (recieved_message == "chng,ok")
    {
        success_code = 1;
    }
    send_Success_Data(box_id_details, "change", success_code, st, bt);
}

void sound_buzzer()
{
    //TODO: Search in table for communicaiton id
    char address[COMMUNICATION_ID_LENGTH];
    String message = "buzz";
    write_radio(address, message);
}

void aws_setup()
{
    update_oled("AWS", "Setup", "In Progress");
    net.setCACert(cacert);
    net.setCertificate(client_cert);
    net.setPrivateKey(privkey);
    client.begin(MQTT_HOST, MQTT_PORT, net);
    client.onMessage(messageReceived);
    connectToMqtt(false);
    update_oled("AWS", "Setup", "Complete");
}

void messageReceived(String &topic, String &payload)
{
    //Reference: https://github.com/bblanchon/ArduinoJson/blob/6.x/examples/JsonParserExample/JsonParserExample.ino
    solid_rgb_ring(YELLOW_COLOR);
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
    {
        add_new_box(box_id);
    }
    else if (cmd == "calibrate") //Calibrate box parameters
    {
        calibrate_box(box_id);
    }
    else if (cmd == "ask") //Ask box for updated values
    {
        update_box_data(box_id);
    }
    else if (cmd == "change_setting") // Change box settings
    {
        String dt = doc["dt"];
        String st = doc["st"];
        String bt = doc["bt"];
        change_box_setting(box_id, dt, st, bt);
    }
    else if (cmd == "buzz")
    {
        sound_buzzer();
    }

    solid_rgb_ring(GREEN_COLOR);
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

void send_Success_Data(String box_id, String command, int success_status, String param1, String param2)
{
    // Reference:https://github.com/bblanchon/ArduinoJson/blob/6.x/examples/JsonGeneratorExample/JsonGeneratorExample.ino
    StaticJsonDocument<50> doc;
    doc["Box ID"] = box_id;
    doc["cmd"] = command;
    doc["success"] = success_status;
    doc["param1"] = param1;
    doc["param2"] = param2;
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
    {
        update_oled("AWS", command, "Success");
    }
}

void sendData(String message)
{
    // Reference:https://github.com/bblanchon/ArduinoJson/blob/6.x/examples/JsonGeneratorExample/JsonGeneratorExample.ino
    char token = ',';
    char params[UPDATE_PARAMETERS][10] = {{}, {}, {}, {}, {}};
    char box_id[BOX_ID_LENGTH];
    int j = 0;
    for (int i = 0; i < (sizeof(message) / message[0]); i++)
    {
        if (i < BOX_ID_LENGTH)
        {
            box_id[i] = message[i];
        }

        else
        {
            if (message[i] != token)
            {
                params[j][i % 10] = message[i];
            }
            else
            {
                j++;
            }
        }
    }
    String c = (String)params[0]; //Count
    String t = (String)params[1]; //Temperature
    String b = (String)params[2]; //Battery
    String o = (String)params[3]; //Offset
    String a = (String)params[4]; //Average Weight

    StaticJsonDocument<50> doc;
    doc["time"] = latest_time.hour + '/' + latest_time.minutes + '/' + latest_time.seconds;
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
    {
        update_oled("AWS", "update", "Success");
    }
}

void setup()
{
    Serial.begin(115200);
    pin_setup();
    rgb_ring_setup();
    aws_setup();
    oled_setup();
    wifi_ntp_setup();
    rtc_setup();
    radio.begin();
    sd_setup();
}

void loop()
{
    printLocalTime();

    //    read_rotary_encoder();
    /*
    if (radio_msg != "no")
    {
        now = time(nullptr);
        if (!client.connected())
        {
            checkWiFiThenMQTT();
            //checkWiFiThenMQTTNonBlocking();
            //checkWiFiThenReboot();
        }
        else
        {
            OLED_print("AWS", "Write");
            client.loop();
            sendData(radio_msg);
            delay(1000);
            lastMillis = millis();
            String data = get_time_format() + " : " + radio_msg + "\n";
            Serial.println("SD Message: " + data);
            appendFile(SD, "/data_file_1.txt", data);
        }
    }
    char timer[6];
    int timer_delay = 1;
    for (size_t i = 0; i < timer_delay; i++)
    {
        itoa(i, timer, 10);
        OLED_print("delay", timer);
        delay(1000);
    }

    */
}
