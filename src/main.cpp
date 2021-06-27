#include "HX711.h"
#define DOUT D1
#define CLK D8
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <MFRC522.h>
#include <PubSubClient.h>
#include <SPI.h>
typedef double gramm;

constexpr uint8_t RST_PIN = D3;
constexpr uint8_t SS_PIN = D4;
const char *ssid = "";
const char *password = "";
const char *MQTT_BROKER = "broker.hivemq.com";
double masse;
WiFiClient espClient;
PubSubClient client(espClient);
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
String rfidtag;
String tag;
String mac;
float calibration_factor = -339000;
// float calibration_factor = -495000;
// float calibration_factor = -575000;
// float calibration_factor = -550000;
// float calibration_factor = -15000000; // der is eigentlich shit --> Ergebnis
// geht gegen 0
boolean scaling = true;
HX711 scale;
void setup() {
    scale.begin(DOUT, CLK);
    Serial.begin(9600);
    Serial.println("Hallo");
    // WLAN Verbindung aufbauen
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println(WiFi.localIP());
    mac = WiFi.macAddress();
    client.setServer(MQTT_BROKER, 1883);
    // RFID lesen
    Serial.println("SPI Begin");
    SPI.begin();
    Serial.println("PCD _INIT");
    rfid.PCD_Init();

    // Waegezelle einrichten
    Serial.println("Set scale");
    scale.set_scale();
    Serial.println("Tare scale");
    scale.tare(); // Reset the scale to 0
    delay(5000);
    Serial.println("Finished Tare scale");
    long zero_factor = scale.read_average(); // Get a baseline reading
    Serial.print(
        "Zero factor: "); // This can be used to remove the need to tare the
                          // scale. Useful in permanent scale projects.
    Serial.println(zero_factor);
    /*
   if (!rfid.PCD_PerformSelfTest()) {
   Serial.println("Self test failed");
   } else {
   Serial.println("Self test sucessfully");
   }
   */
}
void loop() {
    // RFID
    // Serial.println("Enter Loop");
    /*
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
    */
    tag = "EGAL";
    // TODO comment in delay(1000);
    // Waegezelle
    if (scaling) {
        scale.set_scale(calibration_factor); // Adjust to this calibration
                                             // factor
        calibration_factor += 500;
    }
    Serial.print("Calibration Factor ");
    Serial.println(calibration_factor);
    Serial.print("Messe... ");
    Serial.print(scale.get_units(), 3);
    masse = scale.get_units();
    Serial.print(" kg");
    Serial.print(" calibration_factor: ");
    Serial.print(calibration_factor);
    Serial.println();
    // delay(1000);
    if (!client.connected()) {
        while (!client.connected()) {
            // Client-ID wird gesetzt
            client.connect("MQTT_BROKER");
            delay(100);
        }
    }
    masse = abs(masse);
    if (masse > 0.264) {
        Serial.print("scaling done");
        scaling = false;
    }
    String mqttMessage = mac + " , " + tag + " , " + String(masse);
    String masseAlsString = String(masse);
    char buf[64];
    mqttMessage.toCharArray(buf, 64);
    client.publish("massBeverage", buf);
    Serial.println("Versendet");
    rfidtag = "";
    tag = "";
}
