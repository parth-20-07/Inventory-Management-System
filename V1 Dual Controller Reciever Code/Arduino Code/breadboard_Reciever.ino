#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

byte reciever_address[5]={'b','o','x','i','t'};
byte box_address[5]={'b','o','x','i','t'};


RF24 radio(6, 7); // CE CSN

String radio_msg;

void setup() {
  Serial.begin(115200);
  pinMode(3,OUTPUT);
  digitalWrite(3,HIGH);
 radio.begin();
     
    radio.openReadingPipe(0, box_address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
    delay(500);

}

void loop() {

radio_msg=read_radio();
if (radio_msg!="")
{
  Serial.println(radio_msg);
  radio_msg="";
  digitalWrite(3,LOW);
  delay(10);
  digitalWrite(3,HIGH);
  }
}

String read_radio()
{
    if (radio.available()) {
    char text[32] = "";
    radio.read(&text, sizeof(text));
    radio.flush_rx();
    return String(text);
  }
  else{return "";}
  }

boolean send_radio(String cmd)
{ char sendm[32];
  cmd.toCharArray(sendm,32);
  radio.openWritingPipe(reciever_address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  delay(500);
  radio.write(&sendm, sizeof(sendm));
  radio.openReadingPipe(0, box_address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
    delay(500);
  }

