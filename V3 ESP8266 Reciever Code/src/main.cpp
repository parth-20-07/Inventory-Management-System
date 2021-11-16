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
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

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
byte BROADCAST_RECIEVER_ADDRESS[6] = "boxit";
String periodic_data_update_request = ",updt,c,t,b,o,a"; // Parameter for requesting box data updates
String add_box = ",pair,";                               // Add new box
String calibrate_box = ",cali";                          // Calibrate request to box
String change_box_parameters = ",chng,";                 // Change box Parameters
String buzz_box = ",buzz";                               // Start buzzer on box

//! SD Card Setup
#include "FS.h"
#include "SD.h"
String AWS_BACKLOG_FILE = "AWS_Backlog_file.txt";
String file_extension = ".txt";

//! Encoder Setup
//Reference: https://howtomechatronics.com/tutorials/arduino/rotary-encoder-works-use-arduino/
#define outputA RE_DT
#define outputB RE_CLK
#define outputS RE_SW
float counter = 0;
int aState;
int aLastState;

//! Function Definition
void rtc_setup();
void update_oled(String text1, String text2, String text3);

//! Function Declaration

int printLocalTime()
{
    update_oled("Updating", "NTP", "");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return EXIT_FAILURE;
    }
    latest_time.year = timeinfo.tm_year;
    latest_time.month = timeinfo.tm_mon;
    latest_time.date = timeinfo.tm_mday;
    latest_time.hour = timeinfo.tm_hour;
    latest_time.minutes = timeinfo.tm_min;
    latest_time.seconds = timeinfo.tm_sec;
    Serial.println(latest_time.year + latest_time.month + latest_time.date + latest_time.hour + latest_time.minutes + latest_time.seconds);
    rtc_setup();
    return EXIT_SUCCESS;
}

void wifi_ntp_setup(void)
{
    //connect to WiFi
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
    //init and get the time
    update_oled("Connecting", "NTP", "");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    printLocalTime();
}

void rtc_setup()
{
    update_oled("RTC", "Connecting", "");
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        while (1)
            ;
        rtc.adjust(DateTime(latest_time.year, latest_time.month, latest_time.date, latest_time.hour, latest_time.minutes, latest_time.seconds));
    }
    update_oled("RTC", "Updated", "");
}

void rtc_get_time()
{
    update_oled("NTP", "Updating", "");
    DateTime now = rtc.now();
    latest_time.year = now.year();
    latest_time.month = now.month();
    latest_time.seconds = now.second();
    latest_time.hour = now.hour();
    latest_time.minutes = now.minute();
    latest_time.seconds = now.second();
}

void oled_setup()
{
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
    delay(500);
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

void write_radio(byte transmission_address[], String transmission_message)
{
    update_oled("Sending", "Message", "via NRF");
    byte address[6];
    for (int i = 0; i < 6; i++)
    {
        address[i] = transmission_address[i];
    }
    radio.openWritingPipe(address);
    radio.setPALevel(RF24_PA_MIN);
    radio.stopListening();
    radio.write(&transmission_message, sizeof(transmission_message));
    delay(100);
}

String read_radio(byte recieving_address[])
{
    update_oled("Recieving", "Message", "via NRF");
    byte address[6];
    for (size_t i = 0; i < 6; i++)
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
}

void readFile(fs::FS &fs, const char *path)
{
    update_oled("Reading", "File from", "MicroSD");
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while (file.available())
    {
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
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
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
    update_oled("Appending", "File to", "MicroSD");
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("Failed to open file for appending");
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
}

void sd_setup()
{
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
}

void read_rotary_encoder()
{
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
}

void pin_setup()
{
    pinMode(outputA, INPUT);
    pinMode(outputB, INPUT);
    pinMode(outputS, INPUT);
}

void setup()
{
    Serial.begin(115200);
    pin_setup();
    oled_setup();
    wifi_ntp_setup();
    rtc_setup();
    radio.begin();
    sd_setup();
}

void loop()
{
    read_rotary_encoder();
}
