#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson (use v6.xx)
#include <time.h>
#define emptyString String()
#include "secrets.h"
///////////////////////////////////////////////////////////////////////////////
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
#include <RF24.h> // Library for NRF24L01 Module communication
RF24 radio(5,4); //CNS,CE
#define NO_OF_BOXES 1
const byte reciever_address[6] = "boxit"; //Address of the reciever node
/* NRF24L01 Connection
NRF24L01 VCC  ------------------ 3.3V of ESP32
NRF24L01 CSN  ------------------ D5 of ESP32
NRF24L01 MOSI ------------------ D23 of ESP32
NRF24L01 GND  ------------------ GND of ESP32
NRF24L01 CE   ------------------ D4 of ESP32
NRF24L01 SCK  ------------------ D18 of ESP32
NRF24L01 MISO ------------------ D19 of ESP32
*/

////////////////////////////////////////////////////////////////////////////////
#if !(ARDUINOJSON_VERSION_MAJOR == 6 and ARDUINOJSON_VERSION_MINOR >= 7)
#error "Install ArduinoJson v6.7.0-beta or higher"
#endif

const int MQTT_PORT = 8883;
const char MQTT_SUB_TOPIC[] = "aws/command22/123456789";
const char MQTT_PUB_TOPIC[] = "$aws/rules/123456789/aws/tel/123456789";

WiFiClientSecure net;

MQTTClient client;

unsigned long lastMillis = 0;
time_t now;
time_t nowish = 1510592825;

void NTPConnect(void)
{
  Serial.print("Setting time using SNTP");
  configTime(19800, 0 , "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < nowish)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
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

void connectToMqtt(bool nonBlocking = false)
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
  connectToMqtt();
}

unsigned long previousMillis = 0;
const long interval = 5000;

void checkWiFiThenMQTTNonBlocking(void)
{
  connectToWiFi(emptyString);
  if (millis() - previousMillis >= interval && !client.connected()) {
    previousMillis = millis();
    connectToMqtt(true);
  }
}

String get_time_format()
{
    struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "" ;
  }
 const char * f = "%Y-%m-%d %H:%M:%S";
    char buf[64];
    strftime(buf, 64, f, &timeinfo);
    return String(buf);
  }

void OLED_sprint(String s1)
{ 
  
  display.clearDisplay();
  display.setTextSize(1.7);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println(s1);
  display.display(); 
  delay(10);
  }

void sendData(String cmd)
{
  DynamicJsonDocument jsonBuffer(JSON_OBJECT_SIZE(15));
  JsonObject root = jsonBuffer.to<JsonObject>();
  //JsonObject state = root.createNestedObject("state");
  //JsonObject state_reported = state.createNestedObject("reported");
  //state_reported["value"] = random(100);
  root["boxid"]=987654321;
  root["dtime"]=get_time_format();
  root["t"]=cmd.substring(cmd.indexOf('t')+2,cmd.indexOf('b')-1).toFloat();
  root["b"]=cmd.substring(cmd.indexOf('b')+2).toFloat();
  root["c"]=cmd.substring(cmd.indexOf('l')+2,cmd.indexOf(',')).toInt();
  Serial.printf("Sending  [%s]: ", MQTT_PUB_TOPIC);
  //OLED_sprint(get_time_format());
  serializeJson(root, Serial);
  Serial.println();
  char shadow[measureJson(root) + 1];
  serializeJson(root, shadow, sizeof(shadow));
  if (!client.publish(MQTT_PUB_TOPIC, shadow, false, 0)){
    lwMQTTErr(client.lastError());
  }
}

void OLED_print(String s1,String s2,String s3)
{ 
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println(s1);
  display.setCursor(0, 30);
  display.println(s2);
  display.setCursor(0, 50);
  display.println(s3);
  display.display(); 
  delay(10);
  }

void messageReceived(String &topic, String &payload)
{
  Serial.println("Recieved [" + topic + "]: " + payload);
}

void connect_to_NRF_slave(){
  //Connects to various slave NRF Boxes
  radio.openWritingPipe(reciever_address); // Address of the receiver node for connection
  radio.setPALevel(RF24_PA_MIN); //Setting power amplifier level
}

char* collect_data_from_NRF_slave(char *data_requests){
  // Asks NRF Module for data2w2wwss
  radio.stopListening(); // Putting Radio in request mode
  radio.write(&data_requests, sizeof(data_requests));
  delay(5);

  radio.openReadingPipe(1,reciever_address); // Address of the NRF Slave for connection
  radio.startListening(); //Putting Radio in listening mode
  while(!radio.available());
  char text[32]="";//Variable to store data recieved from slaves
  radio.read(&text, sizeof(text));
  return text;
}

void setup()
{
  Serial.begin(115200);
  delay(5000);
  Serial.println();
  Serial.println();
  radio.begin();

  WiFi.setHostname(THINGNAME);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  connectToWiFi(String("Attempting to connect to SSID: ") + String(ssid));

  NTPConnect();


  net.setCACert(cacert);
  net.setCertificate(client_cert);
  net.setPrivateKey(privkey);


  client.begin(MQTT_HOST, MQTT_PORT, net);
  client.onMessage(messageReceived);

  connectToMqtt();

/////////////////////////////////////////////////////
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  OLED_print("WELCOME ","TO","BOXIT");
}
String serial_msg;
void loop()
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
    client.loop();
    connect_to_NRF_slave(); // Connect to NRF Slave
    char data_request[] = "c,t";
    serial_msg = collect_data_from_NRF_slave(data_request);
    Serial.println(serial_msg);//Checking output on terminal
    OLED_sprint(serial_msg+"dtime:"+get_time_format());
    sendData(serial_msg);
    delay(2000);
    lastMillis = millis();
    
    OLED_print("MSG Sent",String((millis()-lastMillis)/1000)+"sec","AGO");
  }
}
