#include "HX711.h"
#define DOUT  D1
#define CLK  D8
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>
typedef double gramm;

constexpr uint8_t RST_PIN = D3;
constexpr uint8_t SS_PIN = D4;

const char* ssid = "";
const char* password =  "";
const char* MQTT_BROKER = "192.168.0.137";
double masse;

WiFiClient espClient;
PubSubClient client(espClient);

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

String rfidtag;
String tag;
float calibration_factor = -495000;

HX711 scale(DOUT, CLK);

void setup() {
  Serial.begin(9600);
  Serial.println("Hallo");
  // WLAN Verbindung aufbauen
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println(WiFi.localIP());
  client.setServer(MQTT_BROKER, 1883);
  // RFID lesen
  
  SPI.begin();
  rfid.PCD_Init();
  

  // Waegezelle einrichten
  scale.set_scale();
  scale.tare(); //Reset the scale to 0

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);

}

void loop() {
  // RFID
  if (!rfid.PICC_IsNewCardPresent())
    return;
  if (rfid.PICC_ReadCardSerial()) {
  for (byte i = 0; i < 4; i++) {
      tag += rfid.uid.uidByte[i];
    }
  }
  Serial.println(tag);
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  
  delay(4000);
  // Waegezelle
  scale.set_scale(calibration_factor); //Adjust to this calibration factor

  Serial.print("Messe... ");
  Serial.print(scale.get_units(), 3);
  masse = scale.get_units();
  Serial.print(" kg");
  Serial.print(" calibration_factor: ");
  Serial.print(calibration_factor);
  Serial.println();

  delay(1000);

  if (!client.connected()) {
  while (!client.connected()) {
      //Client-ID wird gesetzt
      client.connect("MQTT_BROKER");
      
      delay(100);
    }
  }
  if (masse <0){
    masse= masse * (-1);
  }
  String mqttMessage= tag + " , "+ String(masse);
  String masseAlsString= String(masse);
  char buf[64];
  mqttMessage.toCharArray(buf, 64);
  
  client.publish("massBeverage",buf);
  Serial.println("Versendet"); 
  rfidtag= ""; 
  tag= "";

}
