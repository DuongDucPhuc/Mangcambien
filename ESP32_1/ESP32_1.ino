#include <Adafruit_BMP085.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Wire.h> 

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>


// uart
#define rx_Pin 17
#define tx_Pin 16
SoftwareSerial mySerial(rx_Pin,tx_Pin);   
#define DHTPIN 32   
#define DHTTYPE DHT11 
#define rain_sensor 33
Adafruit_BMP085 bmp;
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Galaxy";
const char* password = "12345678";
String openWeatherMapApiKey = "267f23d527e7e455ccb5ba259761e07f";
String jsonBuffer;

String lat = "10.84";
String lon = "106.78";

unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

float mean[] = {0.00269901, 0.00032764, 0.01377685, 0.01672216, -0.01443705};
float scale[] = {1.00427983, 1.00391359, 0.99704573, 0.99346408, 0.99871994};

// Hàm chuẩn hóa
float normalize(float value, int feature_index) {
    return (value - mean[feature_index]) / scale[feature_index];
}


int predict_rain(float temperature, float humidity, float wind_speed, float cloud_cover, float pressure) {
    // Chuẩn hóa đầu vào
    float temp_norm = normalize(temperature, 0);
    float humidity_norm = normalize(humidity, 1);
    float wind_speed_norm = normalize(wind_speed, 2);
    float cloud_cover_norm = normalize(cloud_cover, 3);
    float pressure_norm = normalize(pressure, 4);

    // Hệ số của mô hình
    float coefficients[] = {-0.28033064, 0.13625921, 0.00357166, 0.08477338, -0.00160099};
    float intercept = -10.476772143809645;

    // Tính toán giá trị tuyến tính
    float linear_combination = (temp_norm * coefficients[0]) +
                               (humidity_norm * coefficients[1]) +
                               (wind_speed_norm * coefficients[2]) +
                               (cloud_cover_norm * coefficients[3]) +
                               (pressure_norm * coefficients[4]) + intercept;

    // Hàm sigmoid
    float probability = 1.0 / (1.0 + exp(-linear_combination));

    // Phân loại
    if (probability >= 0.5) {
        return 1; // Rain
    } else {
        return 0; // No Rain
    }
}
int Wind_Speed = 0;
int Cloud_Cover = 0;

void setup() {
  pinMode(rain_sensor, INPUT);
  dht.begin();
  Serial.begin(115200);
  mySerial.begin(9600);
  if(!bmp.begin())
  Serial.println("bmp280 init error!");

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

}

void loop() 
{
  // đọc cảm biến
  int adc_val2 = digitalRead(rain_sensor);
  float pressure = bmp.readPressure()/100;
  float h = dht.readHumidity();
  float t = dht.readTemperature();


  // truyền dữ liệu qua esp32(2)
  Serial.print("Temperature:");
  Serial.print(t);

  mySerial.print("Temperature:");
  mySerial.print(t);


  Serial.print("Humidity:");
  Serial.print(h);

  mySerial.print("Humidity:");
  mySerial.print(h);


  Serial.print("Rain:");
  Serial.print(adc_val2);

  mySerial.print("Rain:");
  mySerial.print(adc_val2);


  Serial.print("Pressure:");
  Serial.print(pressure);

  mySerial.print("Pressure:");
  mySerial.print(pressure);

if ((millis() - lastTime) > timerDelay) {
    // Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?lat=" + lat + "&lon=" + lon + "&appid=" + openWeatherMapApiKey;
      
      jsonBuffer = httpGETRequest(serverPath.c_str());
      Serial.println(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
      Wind_Speed = myObject["wind"]["speed"];
      Cloud_Cover = myObject["clouds"]["all"];

    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }

  int rain_pre = predict_rain(t, h,  Wind_Speed, Cloud_Cover, pressure);
  if (rain_pre == 1) 
  {
    Serial.println("Predict:1");
    mySerial.println("Predict:1");
  } 
  else 
  {
    Serial.println("Predict:0");
    mySerial.println("Predict:0");
  }

  delay(10000);
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}
