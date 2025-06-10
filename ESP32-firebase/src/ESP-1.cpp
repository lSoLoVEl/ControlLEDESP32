#include <WiFi.h>
#include <WebServer.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <FB_Const.h>
// WiFi credentials
const char* wifi_ssid = "OPPO";
const char* wifi_password = "12345678";

// Firebase credentials
#define API_KEY "AIzaSyC8LlbCVEfb2TSa6GaEthAqiSFfHt_olkI"
#define DATABASE_URL "controlesp32-4af2a-default-rtdb.asia-southeast1.firebasedatabase.app"

// Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

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

  // Initialize Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Signing in anonymously...");
  if (Firebase.auth.loginAnonymous(&config, &auth)) {
    Serial.println("Successfully signed in anonymously.");
    currentUserUid = auth.token.uid.c_str();
    Serial.print("Current user UID: ");
    Serial.println(currentUserUid);
  } else {
    Serial.print("Anonymous sign in failed: ");
    Serial.println(fbdo.errorReason());
  }

  // Set up server routes
  server.on("/", HTTP_GET, []() {
    server.send(200, "application/json", "{\"status\":\"online\"}");
  });

  server.on("/led/on", HTTP_GET, []() {
    currentMode = LED_ON;
    digitalWrite(LED_PIN, HIGH);
    server.send(200, "application/json", "{\"status\":\"LED turned ON\"}");
  });

  server.on("/led/off", HTTP_GET, []() {
    currentMode = LED_OFF;
    digitalWrite(LED_PIN, LOW);
    server.send(200, "application/json", "{\"status\":\"LED turned OFF\"}");
  });

  server.on("/led/auto", HTTP_GET, []() {
    currentMode = LED_AUTO;
    server.send(200, "application/json", "{\"status\":\"LED set to AUTO mode\"}");
  });

  server.on("/interval", HTTP_GET, []() {
    if (server.hasArg("value")) {
      long newInterval = server.arg("value").toInt();
      if (newInterval > 0) {
        interval = newInterval;
        String response;
        response.concat("{\"interval\":");
        response.concat(String(newInterval));
        response.concat("}");
        server.send(200, "application/json", response);
      }
    }
  });

  server.on("/status", HTTP_GET, []() {
    String status = getMode();
    String response;
    response.concat("{\"mode\":\"");
    response.concat(status);
    response.concat("\",\"actualState\":");
    response.concat(String(digitalRead(LED_PIN)));
    response.concat("}");
    server.send(200, "application/json", response);
  });

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

  // Update Firebase every 5 seconds
  static unsigned long lastFirebaseUpdate = 0;
  if (millis() - lastFirebaseUpdate >= 5000) {
    lastFirebaseUpdate = millis();
    
    String path;
    path.concat("/devices/");
    path.concat(currentUserUid);
    
    // Send LED status
    String ledPath;
    ledPath.concat(path);
    ledPath.concat("/led");
    if (Firebase.RTDB.setString(&fbdo, ledPath, getLedStatus())) {
      Serial.println("LED status sent to Firebase");
    } else {
      String errorMsg;
      errorMsg.concat("Failed to send LED status\nREASON: ");
      errorMsg.concat(fbdo.errorReason());
      Serial.println(errorMsg);
    }
    
    // Send mode
    String modePath;
    modePath.concat(path);
    modePath.concat("/mode");
    if (Firebase.RTDB.setString(&fbdo, modePath, getMode())) {
      Serial.println("Mode sent to Firebase");
    }
    
    // Send online status
    String onlinePath;
    onlinePath.concat(path);
    onlinePath.concat("/onlineStatus");
    if (Firebase.RTDB.setBool(&fbdo, onlinePath, true)) {
      Serial.println("Online status sent to Firebase");
    }
  }
} 