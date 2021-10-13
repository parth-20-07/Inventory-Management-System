#ifndef AWS_Setup_h
#define AWS_Setup_h

#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson (use v6.xx)
#define emptyString String()

WiFiClientSecure net;

MQTTClient client;

void messageReceived(String &topic, String &payload)
{
    Serial.println("Recieved [" + topic + "]: " + payload);
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

    root["boxid"] = 987654321;
    root["dtime"] = get_time_format();
    root["t"] = cmd.substring(cmd.indexOf('t') + 2, cmd.indexOf('b') - 1).toFloat();
    root["b"] = cmd.substring(cmd.indexOf('b') + 2).toFloat();
    root["c"] = cmd.substring(cmd.indexOf('l') + 2, cmd.indexOf(',')).toInt();
    Serial.printf("Sending  [%s]: ", MQTT_PUB_TOPIC);
    //OLED_sprint(get_time_format());
    serializeJson(root, Serial);
    Serial.println();
    char shadow[measureJson(root) + 1];
    serializeJson(root, shadow, sizeof(shadow));
    if (!client.publish(MQTT_PUB_TOPIC, shadow, false, 0))
        lwMQTTErr(client.lastError());
}

void OLED_print(String s1, String s2, String s3)
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

#endif