#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials for connecting to ESP-1
const char* ssid = "ESP32-yabai";  // ESP-1's WiFi name
const char* password = "12345678";  // ESP-1's WiFi password

// ESP-1's IP address
const char* esp1_ip = "192.168.4.1";  // ESP-1's IP address in AP mode

// LED pin definition
const int LED_PIN = 5;  // Built-in LED on GPIO5

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
bool ledState = false;

// Create WebServer object on port 80
WebServer server(80);

// Function to check actual LED status
bool getActualLEDStatus() {
  return !digitalRead(LED_PIN);  // LED is active low
}

// Function to check board status
String getBoardStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    return "DISCONNECTED";
  }
  return "CONNECTED";
}

// Function to get current mode as string
String getCurrentModeString() {
  switch (currentMode) {
    case LED_OFF: return "OFF";
    case LED_ON: return "ON";
    case LED_AUTO: return "AUTO";
    default: return "UNKNOWN";
  }
}

// Function to send status to ESP-1
void sendStatusToESP1() {
  HTTPClient http;
  String url = "http://" + String(esp1_ip) + "/status/2";
  http.begin(url);
  
  // Create JSON response with actual device status
  String jsonResponse = "{";
  jsonResponse += "\"boardStatus\":\"" + getBoardStatus() + "\",";
  jsonResponse += "\"mode\":\"" + getCurrentModeString() + "\",";
  jsonResponse += "\"actualState\":" + String(getActualLEDStatus());
  jsonResponse += "}";
  
  int httpCode = http.POST(jsonResponse);
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("ESP-1 Response: " + payload);
  } else {
    Serial.println("Error sending status to ESP-1");
  }
  http.end();
}

void setup() {
  // Initialize LED pin as output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // Start with LED OFF (active low)
  
  // Initialize Serial
  Serial.begin(115200);
  Serial.println("\n\nStarting ESP-2...");

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");

  // Connect to ESP-1's Access Point
  WiFi.begin(ssid, password);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Route for LED control
  server.on("/led/off", HTTP_GET, []() {
    Serial.println("Turning LED OFF");
    currentMode = LED_OFF;
    digitalWrite(LED_PIN, HIGH);  // LED is active low
    server.send(200, "application/json", "{\"mode\":\"OFF\"}");
    sendStatusToESP1();
  });

  server.on("/led/on", HTTP_GET, []() {
    Serial.println("Turning LED ON");
    currentMode = LED_ON;
    digitalWrite(LED_PIN, LOW);  // LED is active low
    server.send(200, "application/json", "{\"mode\":\"ON\"}");
    sendStatusToESP1();
  });

  server.on("/led/auto", HTTP_GET, []() {
    Serial.println("Setting LED to AUTO mode");
    currentMode = LED_AUTO;
    server.send(200, "application/json", "{\"mode\":\"AUTO\"}");
    sendStatusToESP1();
  });

  // Route for interval control
  server.on("/interval", HTTP_GET, []() {
    if (server.hasArg("value")) {
      long newInterval = server.arg("value").toInt();
      if (newInterval > 0) {
        interval = newInterval;
        Serial.print("New interval set: ");
        Serial.println(newInterval);
        server.send(200, "application/json", "{\"interval\":" + String(newInterval) + "}");
        sendStatusToESP1();
      } else {
        server.send(400, "application/json", "{\"error\":\"Invalid interval value\"}");
      }
    } else {
      server.send(400, "application/json", "{\"error\":\"No interval value provided\"}");
    }
  });

  // Route for getting current status
  server.on("/status", HTTP_GET, []() {
    String jsonResponse = "{";
    jsonResponse += "\"boardStatus\":\"" + getBoardStatus() + "\",";
    jsonResponse += "\"mode\":\"" + getCurrentModeString() + "\",";
    jsonResponse += "\"actualState\":" + String(getActualLEDStatus());
    jsonResponse += "}";
    server.send(200, "application/json", jsonResponse);
  });

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();  // Handle client requests
  
  // Handle auto mode
  if (currentMode == LED_AUTO) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_PIN, !ledState);  // LED is active low
      Serial.println(ledState ? "LED ON (Auto)" : "LED OFF (Auto)");
      sendStatusToESP1();
    }
  }
}