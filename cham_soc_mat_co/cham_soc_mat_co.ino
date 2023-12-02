#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <ESP32AnalogRead.h>

const String Name = "T01";
const String pathH = "/IOT/" + Name + "/Humidity";
const String pathT = "/IOT/" + Name + "/Temperature";
const String pathS = "/IOT/" + Name + "/SoilMS";
const String pathU = "/IOT/" + Name + "/Ugly_Grass";
const String pathLED = "/IOT/" + Name + "/DataLED";
const String pathDC = "/IOT/" + Name + "/DataDC";


const int led_wifi = 13;             // led xanh
const int led_camera = 2;            //led đỏ
const int A2 = 33;                   // A2 Cảm biến nhiệt độ - độ ẩm môi trường DHT11
const int MOISTURE_SENSOR_PIN = 32;  // A3 Cảm biến độ ẩm đất
const int button = 14;               // Nút nhấn ReadData
const int Relay1 = 23;
const int Relay2 = 5;
int control;
int count = 0;
int control2;

int percent;                   // Biến giá trị phần trăm độ ẩm đất
int btnState;                  // trạng thái hiện tại của button
int lastButtonState = 0;       // trạng thái trước đó của button
float h, t;                    // biến giá trị của Độ ẩm - Nhiệt độ
int value, esp32_Serial_data;  // biến đọc giá trị ADC Độ ẩm đất - % màu cỏ úa vàng

ESP32AnalogRead adc;
DHT dht(A2, DHT11);
LiquidCrystal_I2C lcd(0x27, 20, 4);

#define WIFI_SSID "tienthanh123"
#define WIFI_PASSWORD "15170203"
#define API_KEY "AIzaSyBtrvvn-FzknQ9KbL3mMHLLE-YyMLNHdcs"
#define DATABASE_URL "https://iot-team-2-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "nhom2@gmail.com"
#define USER_PASSWORD "112233"


unsigned long sendDataPrevMillis = 0;
unsigned long previousMillis = 0;
unsigned long interval = 30000;
FirebaseData fbdo;
volatile bool dataChanged = false;
FirebaseData stream;
FirebaseData stream1;

FirebaseAuth auth;
FirebaseConfig config;
FirebaseData firebaseData;
String uid;
//----- setup connect camera-----
void connect_camera() {
  while (!Serial.available()) {
    digitalWrite(led_camera, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unusual Camera !!!");
    delay(1500);
    lcd.setCursor(0, 1);
    lcd.print("Please try Again");
    lcd.setCursor(0, 2);
    lcd.print("Your Device will");
    lcd.setCursor(0, 3);
    lcd.print("Automatically reboot");
    delay(2000);
  }
  digitalWrite(led_camera, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected to Camera");
  delay(1500);
}

//------------Kết nối WiFi--------------
void accessWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lcd.clear();
  lcd.setCursor(2, 1);
  lcd.print("Access to Wi-Fi");
  lcd.setCursor(2, 2);
  lcd.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(led_wifi, LOW);
  }
  digitalWrite(led_wifi, HIGH);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Connected to Wifi!!!");
  lcd.setCursor(4, 2);
  lcd.print(WiFi.localIP());
  delay(1500);
}
//----- Đăng nhập với tài khoản được tạo trên Firebase-------
void signIn(const char *email, const char *password) {
  auth.user.email = email;
  auth.user.password = password;
  Firebase.reset(&config);
  Firebase.begin(&config, &auth);
  while ((auth.token.uid) == "") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Authenticating ...");
    delay(1500);
  }
  uid = auth.token.uid.c_str();
  String path0 = "/IOT/ListModule/" + String(uid) + "/" + Name;
  String path1 = "/IOT/" + Name + "/uid";
  Firebase.RTDB.setString(&firebaseData, path1.c_str(), uid.c_str());
  Firebase.RTDB.setInt(&firebaseData, path0.c_str(), 2);
  Firebase.RTDB.setInt(&firebaseData, pathLED.c_str(), 0);
  Firebase.RTDB.setInt(&firebaseData, pathDC.c_str(), 0);
  lcd.clear();
  lcd.setCursor(2, 1);
  lcd.print("Successful");
  lcd.setCursor(2, 2);
  lcd.print("Authencation !!!");
  delay(1500);
}
//-----Hàm luồng chờ thời gian thực realtime cho OUTPUT1----
void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("stream timed out, resuming...");
  }
  if (!stream.httpConnected()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("error code:, reason:");
    lcd.setCursor(0, 1);
    lcd.print(stream.httpCode());
    lcd.print(stream.errorReason().c_str());
  }
}
//-----Hàm luồng chờ thời gian thực realtime cho OUTPUT2------
void streamTimeoutCallback1(bool timeout) {
  if (timeout) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("stream timed out, resuming...");
  }
  if (!stream.httpConnected()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("error code:, reason:");
    lcd.setCursor(0, 1);
    lcd.print(stream.httpCode());
    lcd.print(stream.errorReason().c_str());
  }
}
//---Đọc dữ liệu realtime - Điều khiển OUTPUT1---
void streamCallback(FirebaseStream data) {
  Serial.printf("sream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str(),
                data.eventType().c_str());
  printResult(data);  // Xem addons/RTDBHelper.h
  String value = data.stringData();
  if (value == "1") {
    digitalWrite(Relay1, HIGH);
  } else if (value == "0") {
    digitalWrite(Relay1, LOW);
  }
  dataChanged = true;
}
//---Đọc dữ liệu realtime - Điều khiển OUTPUT2----
void streamCallback1(FirebaseStream data) {
  Serial.printf("sream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str(),
                data.eventType().c_str());
  printResult(data);  // Xem addons/RTDBHelper.h
  String value = data.stringData();
  if (value == "1") {
    digitalWrite(Relay2, HIGH);
  } else if (value == "0") {
    digitalWrite(Relay2, LOW);
  }
  dataChanged = true;
}
//---------SET UP---------
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(1);
  pinMode(button, INPUT);       // Nút nhấn ReadData
  pinMode(led_wifi, OUTPUT);    // Led đỏ _ WiFi disconnect
  pinMode(led_camera, OUTPUT);  // Led xanh _ WiFi connected
  pinMode(Relay1, OUTPUT);
  pinMode(Relay2, OUTPUT);
  digitalWrite(led_wifi, LOW);
  digitalWrite(led_camera, LOW);
  adc.attach(MOISTURE_SENSOR_PIN);
  dht.begin();  // Khởi động cảm biến DHT11
  lcd.init();
  lcd.backlight();
  connect_camera();  // Truy cập vào camera
  accessWiFi();      // Truy cập WiFi
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback;
  signIn(USER_EMAIL, USER_PASSWORD);
  stream.keepAlive(5, 5, 1);   //đặt thời gian chờ và tần suất gửi lại yêu cầu giữ kết nối luồng Firebase
  if (!Firebase.RTDB.beginStream(&stream, pathLED)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(stream.errorReason().c_str());
  }
  Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);
  if (!Firebase.RTDB.beginStream(&stream1, pathDC)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(stream.errorReason().c_str());
  }
  Firebase.RTDB.setStreamCallback(&stream1, streamCallback1, streamTimeoutCallback1);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("KC326-KC225 - Team 2");
  lcd.setCursor(0, 1);
  lcd.print("Caring Surface Grass");
  lcd.setCursor(1, 2);
  lcd.print("Press the button");
  lcd.setCursor(0, 3);
  lcd.print("ioteam-2-ctu.web.app");
}
//-----get data từ Firebase lên LCD----
void get_data_lcd(float do_am, float nhiet_do, int phan_tram, int phan_tram_co) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Humidity: ");
  lcd.print(do_am);
  lcd.print(" %");

  lcd.setCursor(0, 1);
  lcd.print("Temperature: ");
  lcd.print(nhiet_do);
  lcd.print(" C");

  lcd.setCursor(0, 2);
  lcd.print("Soil Moisture: ");
  lcd.print(phan_tram);
  lcd.print(" %");

  lcd.setCursor(0, 3);
  lcd.print("Ugly Grass: ");
  lcd.print(phan_tram_co);
  lcd.print(" %");
}
//------push to firebase------
void push_firebase(float h, float t, int percent, int grass_percent) {
  Firebase.RTDB.setFloat(&firebaseData, pathH.c_str(), h);
  Firebase.RTDB.setFloat(&firebaseData, pathT.c_str(), t);
  Firebase.RTDB.setInt(&firebaseData, pathS.c_str(), percent);
  Firebase.RTDB.setInt(&firebaseData, pathU.c_str(), grass_percent);
}
//--check camera in loop-----
void reconnect_camera() {
  while (!Serial.available()) {
    digitalWrite(led_camera, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Please reconnect to");
    lcd.setCursor(5, 1);
    lcd.print("the Camera");
    delay(500);
  }
  digitalWrite(led_camera, HIGH);
}
//---check wifi in loop-----
void reconnect_wifi() {
  unsigned long currentMillis = millis();
  while (WiFi.status() != WL_CONNECTED  && (currentMillis - previousMillis >=interval)) {
    WiFi.disconnect();
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Reconnecting to WiFi");
    delay(2000);
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
}
//---------LOOP-----------
void loop() {
  value = adc.readRaw();                             // Đọc giá trị ADC của cảm biến độ ẩm đất tại chân GPIO32
  percent = (100 - map(value, 1600, 3100, 0, 100));  // Đổi giá trị ADC sang tỉ lệ phần trăm tương ứng
  h = dht.readHumidity();                            // Đọc giá trị độ ẩm từ cảm biến DHT11
  t = dht.readTemperature();                         // Đọc giá trị nhiệt độ từ cảm biến DHT11
  esp32_Serial_data = Serial.readString().toInt();   // Đọc giá trị phần trăm cỏ úa vàng từ giao tiếp Serial
  btnState = digitalRead(button);
  if (btnState != lastButtonState) {
    if (btnState == 1) {
      if ((millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();
        push_firebase(h, t, percent, esp32_Serial_data);  // Gửi dữ liệu lên Firebase
      }
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Loading . . .");
      delay(2000);
      get_data_lcd(h, t, percent, esp32_Serial_data);  // Gửi dữ liệu lên màn hình LCD
    }
  }
  lastButtonState = btnState;
  while (!btnState) {
    btnState = digitalRead(button);
  }
  delay(100);
  reconnect_camera();
  reconnect_wifi();
}
