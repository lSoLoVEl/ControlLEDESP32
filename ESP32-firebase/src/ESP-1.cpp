#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#include <WebServer.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#elif defined(ESP8266)
#endif
#include <addons/RTDBHelper.h>
#include <FB_Const.h>
// WiFi credentials
const char* wifi_ssid = "OPPO";
const char* wifi_password = "12345678";

// Firebase credentials
#define API_KEY "AIzaSyC8LlbCVEfb2TSa6GaEthAqiSFfHt_olkI"
#define DATABASE_URL "https://controlesp32-4af2a-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "admin@gmail.com"
#define USER_PASSWORD "12345678"
// Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
unsigned long count = 0;
// ตัวแปรสำหรับเก็บ UID ที่ได้จากการ Authenticate
String currentUserUid = "";

// LED pin definition
const int LED_PIN = 5;

// LED control modes
enum LedMode {
  LED_OFF,
  LED_ON,
  LED_AUTO
};

// Current LED mode
LedMode currentMode = LED_OFF;

// Variables for auto mode
unsigned long previousMillis = 0;
long interval = 1000;  // Blink interval in milliseconds

// Create WebServer object
WebServer server(80);

// Function to get LED status
String getLedStatus() {
  return digitalRead(LED_PIN) ? "ON" : "OFF";
}

// Function to get current mode
String getMode() {
  switch (currentMode) {
    case LED_OFF: return "OFF";
    case LED_ON: return "ON";
    case LED_AUTO: return "AUTO";
    default: return "UNKNOWN";
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Connect to WiFi
  WiFi.begin(wifi_ssid, wifi_password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

   Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  // Initialize Firebase
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

 




  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  // Handle auto mode
  if (currentMode == LED_AUTO) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
  }

  // Firebase operations - combined update every 5 seconds
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    // Create JSON object for device data
    FirebaseJson json;
    json.setDoubleDigits(3);
    json.add("ledStatus", getLedStatus());
    json.add("mode", getMode());
    json.add("count", count);
    json.add("onlineStatus", true);

    // Set main device data
    String path = "/devices/";
    path += currentUserUid;
    Serial.printf("Set device data... %s\n", Firebase.RTDB.setJSON(&fbdo, path, &json) ? "ok" : fbdo.errorReason().c_str());

    // Create and set history array
    FirebaseJsonArray arr;
    arr.setFloatDigits(2);
    arr.setDoubleDigits(4);
    arr.add(getLedStatus(), getMode(), count);

    String arrayPath = path;
    arrayPath += "/history";
    Serial.printf("Set history array... %s\n", Firebase.RTDB.setArray(&fbdo, arrayPath, &arr) ? "ok" : fbdo.errorReason().c_str());

    // Push to logs
    String pushPath = path;
    pushPath += "/logs";
    Serial.printf("Push to logs... %s\n", Firebase.RTDB.pushJSON(&fbdo, pushPath, &json) ? "ok" : fbdo.errorReason().c_str());

    // Update the pushed data with incremented counter
    json.set("count", count + 0.29745);
    String updatePath = pushPath;
    updatePath += "/";
    updatePath += fbdo.pushName();
    Serial.printf("Update log entry... %s\n\n", Firebase.RTDB.updateNode(&fbdo, updatePath, &json) ? "ok" : fbdo.errorReason().c_str());

    count++;
  }
} 