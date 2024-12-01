#define BLYNK_TEMPLATE_ID "TMPL6MZ1q0c6-"
#define BLYNK_TEMPLATE_NAME "ESP32"
#define BLYNK_AUTH_TOKEN "8gY2B3mcqDpaX6D4Mxu7Ym7EymXIz5MD"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Firebase_ESP_Client.h>
#include <HTTPClient.h> // For LINE Notify

// Firebase credentials
#define DATABASE_URL "https://finalprojectembeded-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define API_KEY "AIzaSyCc8blsqjS2buXnO0-oSIF-ihculPD51Nw"

// LINE Notify token
const String lineNotifyToken = "5csNQjR898Iev5G5ENGXP3q7QmNr6l5ZDZBvjHhwLhf";

// WiFi credentials
const char* ssid = "Hello";
const char* pass = "12345678";

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;
unsigned long sendDataPrevMillis = 0;

// Define Blynk virtual pins
#define V0 0
#define V1 1
#define V2 2
#define V3 3
#define V4 4
#define V5 5

// Structure to hold sensor data
struct CombinedData {
  int soilMoisture;         // Soil moisture value
  float lightLevel;         // Light level value
  float temperature;        // Temperature value
  float humidity;           // Humidity value
  float co2Concentration;   // CO2 concentration
  bool relayState;          // Relay state
};

CombinedData receivedData;

// Function to send a LINE notification
void sendLineNotify(const String& message) {
  HTTPClient http;
  http.begin("https://notify-api.line.me/api/notify");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Authorization", "Bearer " + lineNotifyToken);

  String payload = "message=" + message;
  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    Serial.print("LINE Notify Response: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Failed to send LINE Notify. Error: ");
    Serial.println(http.errorToString(httpResponseCode).c_str());
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // TX2 and RX2 for communication with Device 2

  // Connect to WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // Connect to Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Firebase setup
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase connected!");
    signupOK = true;
  } else {
    Serial.printf("Firebase Signup Error: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Device ready!");
}

void loop() {
  Blynk.run();

  // Check if data is available on Serial2
  if (Serial2.available() >= sizeof(receivedData)) {
    Serial2.readBytes((uint8_t *)&receivedData, sizeof(receivedData));

    // Send data to Firebase every 5 seconds
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
      sendDataPrevMillis = millis();

      // Batch write all data to Firebase
      FirebaseJson json;
      json.set("soilMoisture", receivedData.soilMoisture);
      json.set("lightLevel", receivedData.lightLevel);
      json.set("temperature", receivedData.temperature);
      json.set("humidity", receivedData.humidity);
      json.set("co2Concentration", receivedData.co2Concentration);
      json.set("relayState", receivedData.relayState);

      if (Firebase.RTDB.updateNode(&fbdo, "data", &json)) {
        Serial.println("Data sent to Firebase.");
      } else {
        Serial.printf("Firebase Error: %s\n", fbdo.errorReason().c_str());
      }
    }

    // Update Blynk virtual pins
    Blynk.virtualWrite(V1, receivedData.soilMoisture);      // Soil Moisture
    Blynk.virtualWrite(V0, receivedData.lightLevel);        // Light Level
    Blynk.virtualWrite(V2, receivedData.temperature);       // Temperature
    Blynk.virtualWrite(V3, receivedData.humidity);          // Humidity
    Blynk.virtualWrite(V4, receivedData.co2Concentration);  // CO2 Concentration
    Blynk.virtualWrite(V5, receivedData.relayState);        // Relay State

    if (receivedData.soilMoisture > 170) {

      if (receivedData.soilMoisture > 170) {
        sendLineNotify("\n💡Light Level: " + String(receivedData.lightLevel) + " lux" + 
                      "\n🌱Soil Moisture: " + String(receivedData.soilMoisture) +
                       "\n💧Humidity: " + String(receivedData.humidity) + " percent" +
                       "\n🌡️Temperature: " + String(receivedData.temperature) + " °C" + 
                      "\n🍃CO2 : " + String(receivedData.co2Concentration) + " ppm" + 
                      "\n🔌Light State: " + String(receivedData.relayState));
      }
    }
  }
}