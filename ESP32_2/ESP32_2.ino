#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <WiFi.h>
 #define DEVICE "ESP32"
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <Wire.h>   
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
// uart
#define rx_Pin 26
#define tx_Pin 25
SoftwareSerial mySerial(rx_Pin, tx_Pin);
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x3F, lcdColumns, lcdRows);
// thong tin wifi

#define WIFI_SSID "Galaxy"                                                                                        //Network Name
#define WIFI_PASSWORD "12345678"  
#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
  #define INFLUXDB_TOKEN "T8uSu5hD-P5mTiQzd_fG6oZA0gQr8yM6LIfjIbsK4nFmv3MX2OY7ZD9mBGIDqGbS-U-aJnTLzvXA6WhLW75IFg=="
  #define INFLUXDB_ORG "1ddafcc0e36e6530"
  #define INFLUXDB_BUCKET "test2"
  #define rain_sensor 33
  // Time zone info
  #define TZ_INFO "UTC7"   
float temp = 0;                                                       //Variables to store sensor readings
int humid = 0;
int pressure = 0;
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);                 //InfluxDB client instance with preconfigured InfluxCloud certificate

Point sensor("weather");                                            //Data point

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  lcd.init();
  lcd.backlight();
  // kết nối wifi
    WiFi.mode(WIFI_STA);                                              //Setup wifi connection
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");                               //Connect to WiFi
  while (wifiMulti.run() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Đang kết nối WiFi...");
  }
  Serial.println("WiFi đã kết nối!");
timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");                 //Accurate time is necessary for certificate validation and writing in batches

  if (client.validateConnection())                                   //Check server connection
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } 
  else 
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void loop() {
  // Nhận dữ liệu truyền từ ESP32(1) 
  if (mySerial.available() > 0) {
    String data = mySerial.readString();
    Serial.println("Nhận dữ liệu: " + data);
    delay(20);  

    // Phân tích chuỗi dữ liệu để lấy riêng các giá trị
    int tempIndex = data.indexOf("Temperature:");
    int humIndex = data.indexOf("Humidity:");
    int rainIndex = data.indexOf("Rain:");
    int preIndex = data.indexOf("Pressure:");
    int predictIndex = data.indexOf("Predict:");

    // Khai báo biến ở đầu loop để sử dụng toàn bộ trong hàm
    
    int r = 0, pre = 0, p = 0, h = 0,t = 0;

  if (tempIndex != -1 && humIndex != -1 && rainIndex != -1 && preIndex != -1 && predictIndex != -1) {
    t = data.substring(tempIndex + 12, humIndex).toFloat();
    h = data.substring(humIndex + 9, rainIndex).toFloat();
    r = data.substring(rainIndex + 5, preIndex).toInt();  
    p = data.substring(preIndex + 9,predictIndex).toInt();
    pre = data.substring(predictIndex +8).toInt(); 
    } else {
      Serial.println("Lỗi: Dữ liệu không hợp lệ!");
      return;
    }
    sensor.clearFields();                                              //Clear fields for reusing the point. Tags will remain untouched

  sensor.addField("temperature", t);                              // Store measured value into point
  sensor.addField("humidity", h);                                // Store measured value into point
  sensor.addField("pressure", p);
  sensor.addField("rain", r);  
  if (wifiMulti.run() != WL_CONNECTED)                               //Check WiFi connection and reconnect if needed
    Serial.println("Wifi connection lost");

  if (!client.writePoint(sensor))                                    //Write data point
  {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
    // Hiển thị dữ liệu lên LCD
    lcd.setCursor(0, 0);
    lcd.print("T:"); lcd.print(t);
    lcd.setCursor(0, 1);
    lcd.print("H:"); lcd.print(h);
    lcd.setCursor(8, 0);
    lcd.print("P:"); lcd.print(p);
    lcd.setCursor(8, 1);
    lcd.print("R:"); lcd.print(r);
    Serial.println(t);
    Serial.println(h);
    Serial.println(p);
    Serial.println(r);
    Serial.println(pre);
 
  }
  delay(60000); 
}

