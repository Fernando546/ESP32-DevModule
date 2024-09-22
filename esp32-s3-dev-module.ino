#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <esp_sleep.h>

#define DHTPIN 16
#define DHTTYPE DHT22

const char* ssid = "SSID";
const char* password = "PASSWORD";
const char* serverName = "https://esp32-frontend.vercel.app/api/data"; 

DHT dht(DHTPIN, DHTTYPE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  timeClient.begin();
  timeClient.update(); 

  Serial.print("Current time: ");
  Serial.println(timeClient.getFormattedTime());

  int currentMinute = timeClient.getMinutes();
  int currentSecond = timeClient.getSeconds();

  if (currentMinute % 30 == 0 && currentSecond < 30) {
    Serial.println("Attempting to take a measurement...");
    takeMeasurement();
  } else {
    Serial.println("Not the right time for measurement");
  }

  unsigned long sleepTime = calculateSleepTime();

  esp_sleep_enable_timer_wakeup(sleepTime * 1000000); 
  Serial.println("Going to sleep...");
  esp_deep_sleep_start();
}

void takeMeasurement() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + "}";
  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response Code: " + String(httpResponseCode));
    Serial.println("Response: " + response);
  } else {
    Serial.println("Error on sending POST request");
    Serial.println("Response Code: " + String(httpResponseCode));
  }

  http.end();
}

unsigned long calculateSleepTime() {
  timeClient.update();
  int currentMinute = timeClient.getMinutes();
  int currentSecond = timeClient.getSeconds();

  int minutesToNextHalfHour = 30 - (currentMinute % 30); 
  int secondsToNextMinute = 60 - currentSecond; 

  unsigned long totalSleepTime = (minutesToNextHalfHour * 60) + secondsToNextMinute - 30;

  Serial.printf("Going to sleep for: %d min, %d s\n", totalSleepTime / 60, totalSleepTime % 60);
  
  timeClient.update();
  String predictedWakeTime = timeClient.getFormattedTime();
  int predictedHour = (predictedWakeTime.substring(0, 2).toInt() + (currentMinute + minutesToNextHalfHour) / 60) % 24;
  int predictedMinute = (currentMinute + minutesToNextHalfHour) % 60;
  predictedWakeTime = String(predictedHour) + ":" + String(predictedMinute < 10 ? "0" : "") + String(predictedMinute) + ":30";

  Serial.println("Predicted wake-up time: " + predictedWakeTime);
  
  return totalSleepTime;
}

void loop() {
  // Nic się tutaj nie dzieje
}
