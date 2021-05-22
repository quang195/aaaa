#include <TridentTD_EasyFreeRTOS32.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h> 
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>


#define RST_PIN             15
#define SS_PIN              5 
#define SDA_PIN             21 
#define SCL_PIN             22 
#define Servo_pin_1         13
#define Servo_pin_2         14
#define buzzer_pin          2 
/*cảm biến pin */
#define entrance_sensor     25
#define exit_sensor         26
/*nút nhấn pin */
#define open_entrance_pin   33
#define close_entrance_pin  34
#define open_exit_pin       35
#define close_exit_pin      36
/*độ mở của barie*/
#define max_angle_1         60//entrance             
#define max_angle_2         60//exit
/*Thông tin về wifi*/
#define ssid "abc"
#define password "12345678"
#define mqtt_server "broker.hivemq.com"
const uint16_t mqtt_port = 1883; //Port của MQTT
#define topic1  "in"
#define topic2  "out"

LiquidCrystal_I2C lcd_1(0x25, 16, 2);// lcd lối vào
LiquidCrystal_I2C lcd_2(0x27, 16, 2);// lcd lối ra
MFRC522 mfrc522(SS_PIN, RST_PIN);
RTC_DS1307 rtc;
Servo myservo_1, myservo_2;
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi();
void setup_rtc();
void setup_rc522();
void setup_lcd();
void check_connect();//ktra ket noi wifi, mqtt, tu ket noi lai
void getTime();// hàm lấy và in ra thời gian từ RTC,trả lại vào mảng buffTime
void convert(unsigned long currentMillis);//hàm tính tổng time đỗ, giá tiền vé ,trả lại vào mảng totalTime, cost
void getUid();// hàm lấy mã của thẻ
void handleUid();// hàm xử lý mã thẻ khi xe ra/vào bãi
void buzzer();// còi báo
void button_entrance(void * parameter);// đóng mở barie = nút nhấn
void button_exit(void * parameter);
void open_entrance();// hàm mở barie lối vào
void auto_close_entrance(void * parameter);// đóng barie khi xe qua cảm biến
void close_entrance();//hàm đóng barie lối vào
void open_exit();// hàm mở barie lối ra
void auto_close_exit(void * parameter);// đóng lối ra khi xe qua cảm biến
void close_exit();// hàm đóng barie lối ra
void display1();

char buffer[200], buffTime[12], buffDay[12], totalTime[12], cost[12];
unsigned long uidDec, uidDecTemp;
unsigned long timer2, timer3, timer4 = 0;// biến hàm check_connect
unsigned long startTime[15] = {};// xe vào thì bấm startTime = current millis, 16 thẻ = 16 biến tính time
unsigned long endTime[15] = {};// time xe ra = currentMillis() - startTime
// uint8_t checktime[15] = {}; //checktime[x] = 0 xe vào, checktime[x] = 1 xe ra
int pos1, pos2 = 0;// position of servo

void setup() 
{
  Serial.begin(115200);
  EEPROM.begin(15);
  pinMode(buzzer_pin, OUTPUT);
  pinMode(entrance_sensor, INPUT);
  pinMode(exit_sensor, INPUT);
  pinMode(open_entrance_pin, INPUT);
  pinMode(close_entrance_pin, INPUT);
  pinMode(open_exit_pin, INPUT);
  pinMode(close_exit_pin, INPUT);
  myservo_1.attach(Servo_pin_1);
  myservo_2.attach(Servo_pin_2);
  setup_lcd();
  setup_wifi();
  setup_rtc();
  setup_rc522();
  open_entrance();
  open_exit();
  close_entrance();
  close_exit();
  buzzer();
  lcd_1.clear();lcd_2.clear();
  lcd_1.setCursor(1,0);
  lcd_1.print("Entrance Gate");
  lcd_2.setCursor(4,0);
  lcd_2.print("Exit Gate");
  
  xTaskCreate(
    auto_close_entrance,    // Function that should be called
    "entrance",   // Name of the task (for debugging)
    5000,            // Stack size (bytes)
    NULL,            // Parameter to pass
    1,               // Task priority
    NULL             // Task handle
  );
  xTaskCreate(
    auto_close_exit,    // Function that should be called
    "exit",   // Name of the task (for debugging)
    5000,            // Stack size (bytes)
    NULL,            // Parameter to pass
    1,               // Task priority
    NULL             // Task handle
  );
  xTaskCreate(
    button_entrance,    // Function that should be called
    "Button1",   // Name of the task (for debugging)
    5000,            // Stack size (bytes)
    NULL,            // Parameter to pass
    1,               // Task priority
    NULL           // Task handle
  );
  xTaskCreate(
    button_exit,    // Function that should be called
    "Button2",   // Name of the task (for debugging)
    5000,            // Stack size (bytes)
    NULL,            // Parameter to pass
    1,               // Task priority
    NULL           // Task handle
  );
}

void loop() 
{
  client.loop();    
  check_connect();
  getTime(); 
  getUid();// lấy đc mã thẻ Uid vào biến uidDec
  handleUid();
  display1();
}

void setup_rc522(){
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.print("Khởi tạo thành công RC522, đang chờ đọc thẻ…");
  Serial.println();
}

void setup_rtc(){
  Wire.begin(SDA_PIN, SCL_PIN);
  rtc.begin();
  if (! rtc.begin()){
   Serial.print("Couldn't find RTC");
   Serial.println();
  }
  if (! rtc.isrunning()){
   Serial.print("RTC is NOT running!");
   Serial.println();
   
  }
}

void setup_lcd(){
  lcd_1.init();       
  lcd_1.begin(16, 2);
  lcd_1.backlight();
  lcd_1.clear();
  
  lcd_2.init();       
  lcd_2.begin(16, 2);
  lcd_2.backlight();
  lcd_2.clear();
}

void setup_wifi(){
  client.setServer(mqtt_server, mqtt_port); 
  delay(10);
  Serial.println();
  Serial.print("Connecting to Wifi ");
  Serial.println(ssid);
  lcd_1.clear();
  lcd_2.clear();
  lcd_1.setCursor(1,0);
  lcd_1.print("Connecting Wifi");
  lcd_2.setCursor(1,0);
  lcd_2.print("Connecting Wifi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  lcd_1.clear();
  lcd_2.clear();
  lcd_1.setCursor(1,0);
  lcd_1.print("Wifi connected");
  lcd_2.setCursor(1,0);
  lcd_2.print("Wifi connected");
  
  // ket noi den mqtt
  Serial.println("Connecting to MQTT server");
  while (!client.connected()) // Chờ tới khi kết nối
  {
    if (client.connect("ESP32")){ //kết nối vào broker
      Serial.println("");
      Serial.println("MQTT connected");
    }
    else{
      Serial.print(".");
    }
  }
}

void check_connect(){
  //reconnect wifi
  unsigned long presentTime = millis();
  if(WiFi.status() != WL_CONNECTED && (presentTime - timer2 > 5000)){
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    delay(10);
    WiFi.reconnect();
    timer2 = presentTime;  
  }
  // reconnect MQTT
  if(!client.connected() && (presentTime - timer3 > 5000)){
    Serial.println("Reconnecting to MQTT Broker.... ");
    if(client.connect("ESP32")){ //kết nối vào broker
      Serial.println("MQTT connected");
    }
    timer3 = presentTime;  
  }
}

void getTime(){
  DateTime now = rtc.now();
  sprintf(buffDay, "%02d/%02d/%02d", now.day(), now.month(), now.year());
  sprintf(buffTime, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
}

void convert(unsigned long currentMillis){// s = seconds, m = minutes, h = hours, d = day 
  uint32_t money = 0;
  unsigned long s = currentMillis / 1000;
  unsigned long m = s / 60;
  unsigned long h = m / 60;
  unsigned long d = h / 24;
  currentMillis %= 1000;
  s %= 60;
  m %= 60;
  h %= 24;
  money = s*60+m*3600+h*216000;
  sprintf(totalTime, "%02lu:%02lu:%02lu", h, m, s);
  sprintf(cost, "%d", money);
}

void buzzer(){
  digitalWrite(buzzer_pin, HIGH);
  delay(200);
  digitalWrite(buzzer_pin, LOW);
}

void button_entrance(void * parameter){
  while(1){
    if(digitalRead(open_entrance_pin)==1){
      while(digitalRead(open_entrance_pin)==1){// nếu ko thả nút nhấn, dothing
        vTaskDelay(1);
      }
      if(pos1 < max_angle_1){ //nếu cổng đang đóng thì mới đc mở
        for (pos1 = 0; pos1 <= max_angle_1; pos1 += 1){ 
          myservo_1.write(pos1);
          delay(15);
        }
      }
    }
    if(digitalRead(close_entrance_pin)==1){
      while(digitalRead(close_entrance_pin)==1){
        vTaskDelay(1);
      }
      if(pos1 >= max_angle_1){//nếu cổng ra đang mở thì mới đóng
        for (pos1 = max_angle_1; pos1 >= 0; pos1 -= 1){ 
          myservo_1.write(pos1);
          delay(15);
        }
      }
    }
    vTaskDelay(1);
  }
}
void button_exit(void * parameter){
  while(1){
    if(digitalRead(open_exit_pin)==1){
      while(digitalRead(open_exit_pin)==1){
        vTaskDelay(1);
      }
      if(pos2 < max_angle_2){// nếu cổng đang đóng thì mới đc mở
        for (pos2 = 0; pos2 <= max_angle_2; pos2 += 1){ 
          myservo_2.write(pos2);
          delay(15);
        } 
      }
    }
    if(digitalRead(close_exit_pin)==1){
      while(digitalRead(close_exit_pin)==1){
        vTaskDelay(1);
      }
      if(pos2 >= max_angle_2){// nếu cổng ra đang mở thì mới đóng
        for (pos2 = max_angle_2; pos2 >= 0; pos2 -= 1){ 
          myservo_2.write(pos2);
          delay(15);
        }
      }
    }
    vTaskDelay(1);
  }
}

void open_entrance(){
  if(pos1 < max_angle_1){//nếu cổng đóng thì mới đc mở
    for (pos1 = 0; pos1 <= max_angle_1; pos1 += 1){ 
      myservo_1.write(pos1);
      delay(15);
    }
  }
}

void close_entrance(){
  if(pos1 >= max_angle_1){//nếu cổng mở thì mới đc đóng
    for (pos1 = max_angle_1; pos1 >= 0; pos1 -= 1){ 
      myservo_1.write(pos1);
      delay(15);
    }
  }
}

void open_exit(){
  if(pos2 < max_angle_2){
    for (pos2 = 0; pos2 <= max_angle_2; pos2 += 1){ 
      myservo_2.write(pos2);
      delay(15);
    }
  }
}

void close_exit(){
  if(pos2 >= max_angle_2){
    for (pos2 = max_angle_2; pos2 >= 0; pos2 -= 1){ 
      myservo_2.write(pos2);
      delay(15);
    }
  }
}

void auto_close_entrance(void * parameter){// nếu xe đi qua cảm biến sau 3s tự động đóng barrie
  for(;;){
    if(digitalRead(entrance_sensor) == LOW){// sensor bị chặn thì = 0  
      while(digitalRead(entrance_sensor) == LOW){// nếu xe vẫn chưa đi qua
        vTaskDelay(1);
      }          
      delay(2000);
      if(pos1 >= max_angle_1){//nếu cổng vào đang mở thì mới đóng     
        for (pos1 = max_angle_1; pos1 >= 0; pos1 -= 1){ 
          myservo_1.write(pos1);
          delay(15);
        }
      }        
    }
    vTaskDelay(1);
  }
}

void auto_close_exit(void * parameter){  // nếu xe đi qua cảm biến sau 3s tự động đóng barrie
  for(;;){
    if(digitalRead(exit_sensor) == LOW){  
      while(digitalRead(exit_sensor) == LOW){
        vTaskDelay(1);
      }          
      delay(2000);
      if(pos2 >= max_angle_2){// nếu cổng ra đang mở thì mới đóng     
        for (pos2 = max_angle_2; pos2 >= 0; pos2 -= 1){ 
          myservo_2.write(pos2);
          delay(15);
        }
      }        
    }
    vTaskDelay(1);
  }
}

void display1(){
  if(millis() - timer4 > 3500){
    timer4 = millis();
    if(pos1 < 5){
      lcd_1.clear();
      lcd_1.setCursor(1,0);
      lcd_1.print("Entrance Gate");
    }
    if(pos2 < 5){
      lcd_2.clear();
      lcd_2.setCursor(4,0);
      lcd_2.print("Exit Gate");
    }
  }
}

void getUid(){
  if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial()) 
  {  
    return;
  }
  uidDec = 0;
  Serial.println();
  Serial.print("Uid_Hex: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "); //mã thẻ UID = x byte HEX, nếu 1 byte có giá trị < 0x10 (=16dec) thì thêm số 0 đằng trước.
    Serial.print(mfrc522.uid.uidByte[i], HEX);// mfrc522.uid.uidByte[i] trả về 1 byte 0xXX
    uidDecTemp = mfrc522.uid.uidByte[i];
    uidDec = uidDec*256 + uidDecTemp;// gán biến uidDec = mã UID của thẻ
  }
  Serial.println();
  Serial.print("Uid_Dec: ");
  Serial.print(uidDec);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void handleUid(){
  /* THẺ 1*/
  if(uidDec == 2597410477 && EEPROM.read(0) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[0] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(0, 1);//  1 gán trạng thái thẻ đã có xe vào  
      EEPROM.commit();  
  }
  if(uidDec == 2597410477 && EEPROM.read(0) == 1){ // xe ra
      endTime[0] = millis() - startTime[0];
      convert(endTime[0]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(0, 0);// 0 trả lại trạng thái free cho thẻ
      EEPROM.commit();
  }
  /* THẺ 2*/
  if(uidDec == 437994932 && EEPROM.read(1) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[1] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(1, 1);//  1 gán trạng thái thẻ đã có xe vào  
      EEPROM.commit();  
  }
  if(uidDec == 437994932 && EEPROM.read(1) == 1){ // xe ra
      endTime[1] = millis() - startTime[1];
      convert(endTime[1]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(1, 0);// 0 trả lại trạng thái free cho thẻ
      EEPROM.commit();
  }
  /* THẺ 3*/
  if(uidDec == 173351604 && EEPROM.read(2) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[2] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(2, 1);//  1 gán trạng thái thẻ đã có xe vào  
      EEPROM.commit();  
  }
  if(uidDec == 173351604 && EEPROM.read(2) == 1){ // xe ra
      endTime[2] = millis() - startTime[2];
      convert(endTime[2]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(2, 0); 
      EEPROM.commit();
  }
  /* THẺ 4*/
  if(uidDec == 169959347 && EEPROM.read(3)== 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[3] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(3, 1); 
      EEPROM.commit();//  1 gán trạng thái thẻ đã có xe vào    
  }
  if(uidDec == 169959347 && EEPROM.read(3) == 1){ // xe ra
      endTime[3] = millis() - startTime[3];
      convert(endTime[3]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(3, 0); 
      EEPROM.commit();
  }
  /* THẺ 5*/
  if(uidDec == 1523457715 && EEPROM.read(4) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[4] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(4, 1); 
      EEPROM.commit();  
  }
  if(uidDec == 1523457715 && EEPROM.read(4) == 1){ // xe ra
      endTime[4] = millis() - startTime[4];
      convert(endTime[4]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(4, 0); 
      EEPROM.commit();// 0 trả lại trạng thái free cho thẻ
  }
    /* THẺ 6*/
  if(uidDec == 709308339 && EEPROM.read(5) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[5] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(5, 1); 
      EEPROM.commit();//  1 gán trạng thái thẻ đã có xe vào    
  }
  if(uidDec == 709308339 && EEPROM.read(5) == 1){ // xe ra
      endTime[5] = millis() - startTime[5];
      convert(endTime[5]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(5, 0); 
      EEPROM.commit();// 0 trả lại trạng thái free cho thẻ
  }
    /* THẺ 7*/
  if(uidDec == 1253160109 && EEPROM.read(6) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[6] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(6, 1); 
      EEPROM.commit();//  1 gán trạng thái thẻ đã có xe vào    
  }
  if(uidDec == 1253160109 && EEPROM.read(6) == 1){ // xe ra
      endTime[6] = millis() - startTime[6];
      convert(endTime[6]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(6, 0); 
      EEPROM.commit();// 0 trả lại trạng thái free cho thẻ
  }
    /* THẺ 8*/
  if(uidDec == 1793178798 && EEPROM.read(7) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[7] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(7, 1); 
      EEPROM.commit();;//  1 gán trạng thái thẻ đã có xe vào    
  }
  if(uidDec == 1793178798 && EEPROM.read(7) == 1){ // xe ra
      endTime[7] = millis() - startTime[7];
      convert(endTime[7]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(7, 0); 
      EEPROM.commit();// 0 trả lại trạng thái free cho thẻ
  }
    /* THẺ 9*/
  if(uidDec == 2862040756 && EEPROM.read(8) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[8] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(8, 1); 
      EEPROM.commit();//  1 gán trạng thái thẻ đã có xe vào    
  }
  if(uidDec == 2862040756 && EEPROM.read(8) == 1){ // xe ra
      endTime[8] = millis() - startTime[8];
      convert(endTime[8]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(8, 0); 
      EEPROM.commit();// 0 trả lại trạng thái free cho thẻ
  }
    /* THẺ 10*/
  if(uidDec == 708590772 && EEPROM.read(9) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[9] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(9, 1); 
      EEPROM.commit();//  1 gán trạng thái thẻ đã có xe vào    
  }
  if(uidDec == 708590772 && EEPROM.read(9) == 1){ // xe ra
      endTime[9] = millis() - startTime[9];
      convert(endTime[9]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(9, 0); 
      EEPROM.commit();// 0 trả lại trạng thái free cho thẻ
  }
    /* THẺ 11*/
  if(uidDec == 974857140 && EEPROM.read(10) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[10] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(10, 1); 
      EEPROM.commit();//  1 gán trạng thái thẻ đã có xe vào    
  }
  if(uidDec == 974857140 && EEPROM.read(10) == 1){ // xe ra
      endTime[10] = millis() - startTime[10];
      convert(endTime[10]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(10, 0); 
      EEPROM.commit();// 0 trả lại trạng thái free cho thẻ
  }
    /* THẺ 12*/
  if(uidDec == 179273645 && EEPROM.read(11) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[11] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(11, 1); 
      EEPROM.commit();//  1 gán trạng thái thẻ đã có xe vào    
  }
  if(uidDec == 179273645 && EEPROM.read(11) == 1){ // xe ra
      endTime[11] = millis() - startTime[11];
      convert(endTime[11]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(11, 0); 
      EEPROM.commit();// 0 trả lại trạng thái free cho thẻ
  }
    /* THẺ 13*/
  if(uidDec == 181764013 && EEPROM.read(12) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[12] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(12, 1); 
      EEPROM.commit();//  1 gán trạng thái thẻ đã có xe vào    
  }
  if(uidDec == 181764013 && EEPROM.read(12) == 1){ // xe ra
      endTime[12] = millis() - startTime[12];
      convert(endTime[12]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(12, 0); 
      EEPROM.commit();// 0 trả lại trạng thái free cho thẻ
  }
    /* THẺ 14*/
  if(uidDec == 3938217902 && EEPROM.read(13) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[13] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(13, 1); 
      EEPROM.commit();//  1 gán trạng thái thẻ đã có xe vào    
  }
  if(uidDec == 3938217902 && EEPROM.read(13) == 1){ // xe ra
      endTime[13] = millis() - startTime[13];
      convert(endTime[13]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(13, 0); 
      EEPROM.commit();// 0 trả lại trạng thái free cho thẻ
  }
    /* THẺ 15*/
  if(uidDec == 1793031853 && EEPROM.read(14) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[14] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(14, 1); 
      EEPROM.commit();//  1 gán trạng thái thẻ đã có xe vào    
  }
  if(uidDec == 1793031853 &&EEPROM.read(14) == 1){ // xe ra
      endTime[14] = millis() - startTime[14];
      convert(endTime[14]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(14, 0); 
      EEPROM.commit();// 0 trả lại trạng thái free cho thẻ
  }
    /* THẺ 16*/
  if(uidDec == 3938407853 && EEPROM.read(15) == 0){//xe vào
      lcd_1.clear();
      lcd_1.setCursor(4,0);
      lcd_1.print("Time In:");
      lcd_1.setCursor(4,1);
      lcd_1.print(buffTime);
      startTime[15] = millis();// bấm giờ
      buzzer();
      delay(500);
      open_entrance();
      
      StaticJsonDocument<100> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_in"] = buffTime;
      doc["Day_in"] = buffDay;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_in" = x:x:x ; "Day_in" = x/x/x}
      client.publish(topic1, buffer);
      
      uidDec = 0;
      EEPROM.write(15, 1); 
      EEPROM.commit();//  1 gán trạng thái thẻ đã có xe vào    
  }
  if(uidDec == 3938407853 && EEPROM.read(15) == 1){ // xe ra
      endTime[15] = millis() - startTime[15];
      convert(endTime[15]);//return totalTime and cost
      lcd_2.clear();
      lcd_2.setCursor(0,0);
      lcd_2.print("Total:");
      lcd_2.setCursor(7,0);
      lcd_2.print(totalTime);
      lcd_2.setCursor(0,1);
      lcd_2.print("Cost: ");
      lcd_2.setCursor(6,1);
      lcd_2.print(cost);
      buzzer();
      delay(500);
      open_exit();

      StaticJsonDocument<200> doc;
      doc["Tag_ID"] = uidDec;
      doc["Time_out"] = buffTime;
      doc["Day_out"] = buffDay;
      doc["Total_time"] = totalTime;
      doc["Cost"] = cost;
      serializeJson(doc, buffer);//{"Tag_ID" = x; "Time_out" = x:x:x ; "Day_out" =x/x/x; "Total_time" = x; "Cost" = x}
      client.publish(topic2, buffer);
      
      uidDec = 0;
      EEPROM.write(15, 0); 
      EEPROM.commit();// 0 trả lại trạng thái free cho thẻ
  } 
}
