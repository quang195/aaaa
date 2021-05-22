#include "PCF8574.h"
#include <LiquidCrystal_I2C.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
//#include "ArduinoJson.h"
#include <PubSubClient.h>
#include <SimpleTimer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#define mqtt_server "broker.hivemq.com"
#define topic0  "home/quang195/sensors0"
#define topic1  "home/quang195/sensors1"
#define topic2  "home/quang195/sensors2"
#define topic3  "home/quang195/sensors3"
#define topic4  "home/quang195/sensors4"
#define topic5  "home/quang195/sensors5"
#define topic6  "home/quang195/sensors6"
#define topic7  "home/quang195/sensors7"
#define topic8  "home/quang195/sensors8"
#define topic9  "home/quang195/sensors9"
#define topic10  "home/quang195/sensors10"
#define topic11  "home/quang195/sensors11"
#define topic12  "home/quang195/sensors12"
#define topic13  "home/quang195/sensors13"
#define topic14  "home/quang195/sensors14"
#define topic15  "home/quang195/sensors15"

void Display(unsigned char mode_t);
void pcf_read();
void check_location();
void setup_wifi();

WiFiClient espClient;
PubSubClient client(espClient);
PCF8574 pcf8574(0x21);
PCF8574 pcf8573(0x20);
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiManager wm;
WiFiUDP u;
NTPClient n(u, "2.vn.pool.ntp.org", 7 * 3600);
SimpleTimer timer;

uint8_t pcf[16];
uint8_t pcf1[16];
uint8_t zone[3];
const uint16_t mqtt_port = 1883; //Port của MQT
long timeStart = 0;
byte degree1[8] = {
  0B00011,
  0B00111,
  0B01111,
  0B11111,
  0B11111,
  0B01111,
  0B00111,
  0B00011
};
byte degree2[8] = {
  0B00100,
  0B01110,
  0B11111,
  0B00100,
  0B00100,
  0B00100,
  0B00100,
  0B00100
};
byte degree3[8] = {
  0B11000,
  0B11100,
  0B11110,
  0B11111,
  0B11111,
  0B11110,
  0B11100,
  0B11000
};

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.print("Connect Wifi");
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port); // cài đặt server và lắng nghe client ở port 1883
  client.setCallback(callback); // gọi hàm callback để thực hiện các chức năng publish/subcribe
  if (!client.connected()) { // Kiểm tra kết nối
    reconnect();
  }
  lcd.clear();
  n.begin();
  pcf8574.pinMode(0, INPUT);
  pcf8574.pinMode(1, INPUT);
  pcf8574.pinMode(2, INPUT);
  pcf8574.pinMode(3, INPUT);
  pcf8574.pinMode(7, INPUT);
  pcf8574.pinMode(6, INPUT);
  pcf8574.pinMode(5, INPUT);
  pcf8574.pinMode(4, INPUT);
  pcf8573.pinMode(0, INPUT);
  pcf8573.pinMode(1, INPUT);
  pcf8573.pinMode(2, INPUT);
  pcf8573.pinMode(3, INPUT);
  pcf8573.pinMode(7, INPUT);
  pcf8573.pinMode(6, INPUT);
  pcf8573.pinMode(5, INPUT);
  pcf8573.pinMode(4, INPUT);
  pcf8574.begin();
  pcf8573.begin();
  lcd.createChar(1, degree1);
  lcd.createChar(2, degree2);
  lcd.createChar(3, degree3);
  timer.setInterval(2000, publish_mqtt);
  timeStart = millis();
}
void loop() {
  timer.run();
  pcf_read();
  check_location();
  client.loop();
  if ((millis() - timeStart < 5000) && (millis() - timeStart > 100)) {
    Display(1);
  }
  if ((millis() - timeStart < 10000) && (millis() - timeStart > 5000)) {
    Display(2);
  }
  if ((millis() - timeStart > 10000)) {
    n.update();
    timeStart = millis();
  }
  delay(10);
}

void publish_mqtt() {
  //  StaticJsonDocument<20> doc0;
  //  StaticJsonDocument<20> doc1;
  //  StaticJsonDocument<20> doc2;
  //  StaticJsonDocument<20> doc3;
  //  StaticJsonDocument<20> doc4;
  //  StaticJsonDocument<20> doc5;
  //  StaticJsonDocument<20> doc6;
  //  StaticJsonDocument<20> doc7;
  //  StaticJsonDocument<20> doc8;
  //  StaticJsonDocument<20> doc9;
  //  StaticJsonDocument<20> doc10;
  //  StaticJsonDocument<20> doc11;
  //  StaticJsonDocument<20> doc12;
  //  StaticJsonDocument<20> doc13;
  //  StaticJsonDocument<20> doc14;
  //  StaticJsonDocument<20> doc15;

  //  doc0["pfc[0]"] = pcf[0];
  //  doc1["pfc[1]"] = pcf[1];
  //  doc2["pfc[2]"] = pcf[2];
  //  doc3["pfc[3]"] = pcf[3];
  //  doc4["pfc[4]"] = pcf[4];
  //  doc5["pfc[5]"] = pcf[5];
  //  doc6["pfc[6]"] = pcf[6];
  //  doc7["pfc[7]"] = pcf[7];
  //  doc8["pfc[8]"] = pcf[8];
  //  doc9["pfc[9]"] = pcf[9];
  //  doc10["pfc[10]"] = pcf[10];
  //  doc11["pfc[11]"] = pcf[11];
  //  doc12["pfc[12]"] = pcf[12];
  //  doc13["pfc[13]"] = pcf[13];
  //  doc14["pfc[14]"] = pcf[14];
  //  doc15["pfc[15]"] = pcf[15];
  //
  char buffer0[1];
  char buffer1[1];
  char buffer2[1];
  char buffer3[1];
  char buffer4[1];
  char buffer5[1];
  char buffer6[1];
  char buffer7[1];
  char buffer8[1];
  char buffer9[1];
  char buffer10[1];
  char buffer11[1];
  char buffer12[1];
  char buffer13[1];
  char buffer14[1];
  char buffer15[1];
  sprintf(buffer0, "%d", pcf1[0]);
  sprintf(buffer1, "%d", pcf1[1]);
  sprintf(buffer2, "%d", pcf1[2]);
  sprintf(buffer3, "%d", pcf1[3]);
  sprintf(buffer4, "%d", pcf1[4]);
  sprintf(buffer5, "%d", pcf1[5]);
  sprintf(buffer6, "%d", pcf1[6]);
  sprintf(buffer7, "%d", pcf1[7]);
  sprintf(buffer8, "%d", pcf1[8]);
  sprintf(buffer9, "%d", pcf1[9]);
  sprintf(buffer10, "%d", pcf1[10]);
  sprintf(buffer11, "%d", pcf1[11]);
  sprintf(buffer12, "%d", pcf1[12]);
  sprintf(buffer13, "%d", pcf1[13]);
  sprintf(buffer14, "%d", pcf1[14]);
  sprintf(buffer15, "%d", pcf1[15]);


  //  serializeJson(doc0, buffer0);
  //  serializeJson(doc1, buffer1);
  //  serializeJson(doc2, buffer2);
  //  serializeJson(doc3, buffer3);
  //  serializeJson(doc4, buffer4);
  //  serializeJson(doc5, buffer5);
  //  serializeJson(doc6, buffer6);
  //  serializeJson(doc7, buffer7);
  //  serializeJson(doc8, buffer8);
  //  serializeJson(doc9, buffer9);
  //  serializeJson(doc10, buffer10);
  //  serializeJson(doc11, buffer11);
  //  serializeJson(doc12, buffer12);
  //  serializeJson(doc13, buffer13);
  //  serializeJson(doc14, buffer14);

  if (pcf[0] != pcf1[0]) {
    delay(10);
    if (pcf[0] != pcf1[0])client.publish(topic0, buffer0);
  }
  if (pcf[1] != pcf1[1]) {
    delay(10);
    if (pcf[1] != pcf1[1]) client.publish(topic1, buffer1);
  }
  if (pcf[2] != pcf1[2]) {
    delay(10);
    if (pcf[2] != pcf1[2]) client.publish(topic2, buffer2);
  }
  if (pcf[3] != pcf1[3]) {
    delay(10);
    if (pcf[3] != pcf1[3]) client.publish(topic3, buffer3);
  }
  if (pcf[4] != pcf1[4]) {
    delay(10);
    if (pcf[4] != pcf1[4]) client.publish(topic4, buffer4);
  }
  if (pcf[5] != pcf1[5]) {
    delay(10);
    if (pcf[5] != pcf1[5]) client.publish(topic5, buffer5);
  }
  if (pcf[6] != pcf1[6]) {
    delay(10);
    if (pcf[6] != pcf1[6]) client.publish(topic6, buffer6);
  }
  if (pcf[7] != pcf1[7]) {
    delay(10);
    if (pcf[7] != pcf1[7]) client.publish(topic7, buffer7);
  }
  if (pcf[8] != pcf1[8]) {
    delay(10);
    if (pcf[8] != pcf1[8]) client.publish(topic8, buffer8);
  }
  if (pcf[9] != pcf1[9]) {
    delay(10);
    if (pcf[9] != pcf1[9]) client.publish(topic9, buffer9);
  }
  if (pcf[10] != pcf1[10]) {
    delay(10);
    if (pcf[10] != pcf1[10]) client.publish(topic10, buffer10);
  }
  if (pcf[11] != pcf1[11]) {
    delay(10);
    if (pcf[11] != pcf1[11]) client.publish(topic11, buffer11);
  }
  if (pcf[12] != pcf1[12]) {
    delay(10);
    if (pcf[12] != pcf1[12]) client.publish(topic12, buffer12);
  }
  if (pcf[13] != pcf1[13]) {
    delay(10);
    if (pcf[13] != pcf1[13]) client.publish(topic13, buffer13);
  }
  if (pcf[14] != pcf1[14]) {
    delay(10);
    if (pcf[14] != pcf1[14]) client.publish(topic14, buffer14);
  }
  if (pcf[15] != pcf1[15]) {
    delay(10);
    if (pcf[15] != pcf1[15]) client.publish(topic15, buffer15);
  }
  for (int i = 0; i < 16; i++) pcf1[i] = pcf[i];
}
void pcf_read() {
  pcf[0] = pcf8574.digitalRead(0);
  pcf[1] = pcf8574.digitalRead(1);
  pcf[2] = pcf8574.digitalRead(2);
  pcf[3] = pcf8574.digitalRead(3);
  pcf[4] = pcf8574.digitalRead(4);
  pcf[5] = pcf8574.digitalRead(5);
  pcf[6] = pcf8574.digitalRead(6);
  pcf[7] = pcf8574.digitalRead(7);
  pcf[8] = pcf8573.digitalRead(0);
  pcf[9] = pcf8573.digitalRead(1);
  pcf[10] = pcf8573.digitalRead(2);
  pcf[11] = pcf8573.digitalRead(3);
  pcf[12] = pcf8573.digitalRead(4);
  pcf[13] = pcf8573.digitalRead(5);
  pcf[14] = pcf8573.digitalRead(6);
  pcf[15] = pcf8573.digitalRead(7);
  for (int i = 0; i < 16; i = i + 1) {
    Serial.print(pcf[i]);
  }
  Serial.println();
}

void Display(unsigned char mode_t) {
  if (mode_t == 1) {
    lcd.setCursor(0, 0);
    lcd.print("- Parking lot - ");
    lcd.setCursor(0, 1);
    lcd.print("    ");
    lcd.setCursor(4, 1);
    lcd.print(n.getFormattedTime());
    lcd.print("      ");
  }
  else if (mode_t == 2) {
    lcd.setCursor(0, 0);
    lcd.print("A: ");
    lcd.print(zone[0]);
    lcd.print("    ");
    lcd.setCursor(8, 0);
    lcd.print("B: ");
    lcd.print(zone[1]);
    lcd.print("    ");
    lcd.setCursor(0, 1);
    lcd.print("C: ");
    lcd.print(zone[2]);
    lcd.print("    ");
    lcd.setCursor(8, 1);
    lcd.print("D: ");
    lcd.print(zone[3]);
    lcd.print("    ");
  } else if (mode_t == 3) {
    uint8_t a = zone[0] + zone[2];
    uint8_t b = zone[2] + zone[3];
    uint8_t c = zone[1] + zone[3];
    lcd.setCursor(8, 0);
    lcd.write(byte(2));
    lcd.setCursor(0, 1);
    lcd.write(byte(1));
    lcd.print(a);
    lcd.setCursor(8, 1);
    lcd.print(b);
    lcd.setCursor(14, 1);
    lcd.print(c);
    lcd.write(byte(3));
  } else {
  }
}

void setup_wifi()
{
  printf("====================> 1\r\n");
  WiFi.mode(WIFI_STA);
  printf("====================> 1\r\n");
  bool res = wm.autoConnect("ESP32", "12345678"); // password protected ap
  if (!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
  delay(1000);
}

// Hàm reconnect thực hiện kết nối lại khi mất kết nối với MQTT Broker
void reconnect()
{
  while (!client.connected()) // Chờ tới khi kết nối
  {
    if (client.connect("Truong-ESP8266-195"))  //kết nối vào broker
    {
      Serial.println("Đã kết nối:");
      //đăng kí nhận dữ liệu từ topic
    }
    else
    {
      // in ra trạng thái của client khi không kết nối được với broker
      Serial.print("Lỗi:, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Đợi 5s
      delay(5000);
    }
  }
}
void check_location() {
  zone[0] = pcf[0] + pcf[0] + pcf[0] + pcf[0];
  zone[1] = pcf[0] + pcf[0] + pcf[0] + pcf[0];
  zone[2] = pcf[0] + pcf[0] + pcf[0] + pcf[0];
  zone[3] = pcf[0] + pcf[0] + pcf[0] + pcf[0];
}
void callback(char* topic, byte* payload, unsigned int length)
{
  //-----------------------------------------------------------------
  //in ra tên của topic và nội dung nhận được
  Serial.print("Co tin nhan moi tu topic: ");
  Serial.println(topic);
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);
}
