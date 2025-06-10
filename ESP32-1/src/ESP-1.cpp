#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi Access Point credentials
const char* ap_ssid = "ESP32-yabai";  // ชื่อ WiFi ที่จะปล่อย
const char* ap_password = "12345678";      // รหัสผ่าน WiFi (ต้องมีอย่างน้อย 8 ตัวอักษร)

// ESP-2 IP Address (จะใช้ IP แบบ Static)
const char* esp2_ip = "192.168.4.2";  // IP ของ ESP-2 ในเครือข่าย AP

// LED pin definition
const int LED_PIN = 5;  // Using GPIO2 for LED

// LED control modes
enum LedMode {
  LED_OFF,
  LED_ON,
  LED_AUTO
};

// Current LED mode for both ESPs
LedMode currentModeESP1 = LED_OFF;
LedMode currentModeESP2 = LED_OFF;

// Variables for auto mode
unsigned long previousMillis = 0;
long interval = 1000;  // Blink interval in milliseconds
bool ledState = false;

// Add these global variables at the top with other global variables
String lastESP2Mode = "UNKNOWN";
bool lastESP2State = false;
bool lastESP2Online = false;

// Create WebServer object on port 80
WebServer server(80);

// HTML content for the web interface
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 LED Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; text-align: center; margin: 20px; }
        .button { padding: 10px 20px; margin: 5px; font-size: 16px; }
        .status-box { 
            border: 1px solid #ccc; 
            padding: 10px; 
            margin: 10px; 
            border-radius: 5px;
            display: inline-block;
            min-width: 200px;
        }
        .led-status {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            display: inline-block;
            margin-left: 10px;
        }
        .led-on { background-color: #00ff00; }
        .led-off { background-color: #ff0000; }
        .connection-status {
            font-size: 14px;
            margin-top: 5px;
            padding: 5px;
            border-radius: 3px;
        }
        .online { 
            color: #ffffff; 
            background-color: #00aa00;
        }
        .offline { 
            color: #ffffff; 
            background-color: #aa0000;
        }
        .status-details {
            margin: 10px 0;
            padding: 5px;
            background-color: #f5f5f5;
            border-radius: 3px;
        }
        .status-label {
            font-weight: bold;
            margin-right: 5px;
        }
    </style>
</head>
<body>
    <h1>ESP32 LED Control</h1>
    
    <div class="status-box">
        <h2>ESP-1</h2>
        <div class="status-details">
            <span class="status-label">Mode:</span>
            <span id="esp1-mode">OFF</span>
            <span id="esp1-led" class="led-status led-off"></span>
        </div>
        <div class="status-details">
            <span class="status-label">Connection:</span>
            <span id="esp1-connection" class="connection-status online">Online</span>
        </div>
        <div class="status-details">
            <span class="status-label">Last Update:</span>
            <span id="esp1-last-update">Never</span>
        </div>
        <div>
            <button class="button" onclick="controlLED(1, 'on')">ON</button>
            <button class="button" onclick="controlLED(1, 'off')">OFF</button>
            <button class="button" onclick="controlLED(1, 'auto')">AUTO</button>
        </div>
    </div>

    <div class="status-box">
        <h2>ESP-2</h2>
        <div class="status-details">
            <span class="status-label">Mode:</span>
            <span id="esp2-mode">OFF</span>
            <span id="esp2-led" class="led-status led-off"></span>
        </div>
        <div class="status-details">
            <span class="status-label">Connection:</span>
            <span id="esp2-connection" class="connection-status offline">Offline</span>
        </div>
        <div class="status-details">
            <span class="status-label">Last Update:</span>
            <span id="esp2-last-update">Never</span>
        </div>
        <div>
            <button class="button" onclick="controlLED(2, 'on')">ON</button>
            <button class="button" onclick="controlLED(2, 'off')">OFF</button>
            <button class="button" onclick="controlLED(2, 'auto')">AUTO</button>
        </div>
    </div>

    <div>
        <h3>Interval (ms)</h3>
        <input type="number" id="interval" value="1000" min="100">
        <button class="button" onclick="setInterval()">Set Interval</button>
    </div>

    <script>
        function formatTime() {
            const now = new Date();
            return now.toLocaleTimeString();
        }

        function updateStatus() {
            // Update ESP-1 status
            fetch('/status/1')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('esp1-mode').textContent = data.mode;
                    const ledElement = document.getElementById('esp1-led');
                    ledElement.className = 'led-status ' + (data.actualState ? 'led-on' : 'led-off');
                    document.getElementById('esp1-last-update').textContent = formatTime();
                });

            // Update ESP-2 status
            fetch('/status/2')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('esp2-mode').textContent = data.mode;
                    const ledElement = document.getElementById('esp2-led');
                    ledElement.className = 'led-status ' + (data.actualState ? 'led-on' : 'led-off');
                    
                    const connectionElement = document.getElementById('esp2-connection');
                    connectionElement.textContent = data.online ? 'Online' : 'Offline';
                    connectionElement.className = 'connection-status ' + (data.online ? 'online' : 'offline');
                    
                    document.getElementById('esp2-last-update').textContent = formatTime();
                });
        }

        function controlLED(esp, mode) {
            fetch('/led/' + mode + '/' + esp)
                .then(response => response.json())
                .then(data => {
                    console.log(data);
                    updateStatus(); // Update status immediately after control
                });
        }

        function setInterval() {
            const value = document.getElementById('interval').value;
            Promise.all([
                fetch('/interval/1?value=' + value),
                fetch('/interval/2?value=' + value)
            ]).then(() => {
                console.log('Interval updated');
            });
        }

        // Update status every 2 seconds
        setInterval(updateStatus, 2000);
        // Initial status update
        updateStatus();
    </script>
</body>
</html>
)rawliteral";

// Function to control ESP-2
void controlESP2(String command) {
  HTTPClient http;
  String url = "http://" + String(esp2_ip) + "/led/" + command;
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("ESP-2 Response: " + payload);
  } else {
    Serial.println("Error on HTTP request to ESP-2");
  }
  http.end();
}

// Function to set interval on ESP-2
void setESP2Interval(long newInterval) {
  HTTPClient http;
  String url = "http://" + String(esp2_ip) + "/interval?value=" + String(newInterval);
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("ESP-2 Interval Response: " + payload);
  } else {
    Serial.println("Error setting interval on ESP-2");
  }
  http.end();
}

void setup() {
  // Initialize LED pin as output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Ensure LED starts in OFF state
  
  // Initialize Serial
  Serial.begin(115200);
  Serial.println("\n\nStarting ESP-1...");
  
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");

  // Set up Access Point
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println("Access Point Started");
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Serial.print("SSID: ");
  Serial.println(ap_ssid);
  Serial.print("Password: ");
  Serial.println(ap_password);

  // Route for root / web page
  server.on("/", HTTP_GET, []() {
    Serial.println("Received request for root page");
    server.send(200, "text/html", index_html);
  });

  // Route for LED control
  server.on("/led/off/1", HTTP_GET, []() {
    Serial.println("Turning LED OFF");
    currentModeESP1 = LED_OFF;
    digitalWrite(LED_PIN, HIGH);  // LED is active low
    server.send(200, "application/json", "{\"mode\":\"OFF\"}");
  });

  server.on("/led/on/1", HTTP_GET, []() {
    Serial.println("Turning LED ON");
    currentModeESP1 = LED_ON;
    digitalWrite(LED_PIN, LOW);  // LED is active low
    server.send(200, "application/json", "{\"mode\":\"ON\"}");
  });

  server.on("/led/auto/1", HTTP_GET, []() {
    Serial.println("Setting LED to AUTO mode");
    currentModeESP1 = LED_AUTO;
    server.send(200, "application/json", "{\"mode\":\"AUTO\"}");
  });

  server.on("/led/off/2", HTTP_GET, []() {
    controlESP2("off");
    server.send(200, "application/json", "{\"mode\":\"OFF\"}");
  });

  server.on("/led/on/2", HTTP_GET, []() {
    controlESP2("on");
    server.send(200, "application/json", "{\"mode\":\"ON\"}");
  });

  server.on("/led/auto/2", HTTP_GET, []() {
    controlESP2("auto");
    server.send(200, "application/json", "{\"mode\":\"AUTO\"}");
  });

  // Route for interval control
  server.on("/interval/1", HTTP_GET, []() {
    if (server.hasArg("value")) {
      long newInterval = server.arg("value").toInt();
      if (newInterval > 0) {
        interval = newInterval;
        server.send(200, "application/json", "{\"interval\":" + String(newInterval) + "}");
      }
    }
  });

  server.on("/interval/2", HTTP_GET, []() {
    if (server.hasArg("value")) {
      long newInterval = server.arg("value").toInt();
      if (newInterval > 0) {
        setESP2Interval(newInterval);
        server.send(200, "application/json", "{\"interval\":" + String(newInterval) + "}");
      }
    }
  });

  // Route for checking LED status
  server.on("/status/1", HTTP_GET, []() {
    bool actualState = !digitalRead(LED_PIN);  // LED is active low, so we invert the reading
    String status = actualState ? "ON" : "OFF";
    if (currentModeESP1 == LED_AUTO) {
      status = "AUTO";
    }
    server.send(200, "application/json", "{\"mode\":\"" + status + "\",\"actualState\":" + String(actualState) + "}");
  });

  // Route for checking ESP-2 status
  server.on("/status/2", HTTP_GET, []() {
    static unsigned long lastCheck = 0;
    static String lastResponse = "{\"mode\":\"UNKNOWN\",\"actualState\":false,\"online\":false}";
    
    // Only check every 5 seconds
    if (millis() - lastCheck >= 5000) {
      HTTPClient http;
      String url = "http://" + String(esp2_ip) + "/status/2";
      http.begin(url);
      http.setTimeout(1000); // Set timeout to 1 second
      int httpCode = http.GET();
      
      if (httpCode > 0) {
        lastResponse = http.getString();
        // Parse and store the last known values
        if (lastResponse.indexOf("\"mode\":\"OFF\"") != -1) {
          lastESP2Mode = "OFF";
          lastESP2Online = true;
        }
        else if (lastResponse.indexOf("\"mode\":\"ON\"") != -1) {
          lastESP2Mode = "ON";
          lastESP2Online = true;
        }
        else if (lastResponse.indexOf("\"mode\":\"AUTO\"") != -1) {
          lastESP2Mode = "AUTO";
          lastESP2Online = true;
        }
        
        if (lastResponse.indexOf("\"actualState\":true") != -1) {
          lastESP2State = true;
        } else if (lastResponse.indexOf("\"actualState\":false") != -1) {
          lastESP2State = false;
        }
      } else {
        lastESP2Online = false;
      }
      http.end();
      lastCheck = millis();
    }
    
    // Create response with last known values
    String response = "{\"mode\":\"" + lastESP2Mode + 
                     "\",\"actualState\":" + String(lastESP2State) + 
                     ",\"online\":" + String(lastESP2Online) + "}";
    server.send(200, "application/json", response);
  });

  // Route to receive status updates from ESP-2
  server.on("/status/2", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      String payload = server.arg("plain");
      server.send(200, "application/json", "{\"status\":\"received\"}");
      
      // Parse JSON response
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        // Update last known values
        String mode = doc["mode"].as<String>();
        if (mode == "OFF") {
          lastESP2Mode = "OFF";
          currentModeESP2 = LED_OFF;
        }
        else if (mode == "ON") {
          lastESP2Mode = "ON";
          currentModeESP2 = LED_ON;
        }
        else if (mode == "AUTO") {
          lastESP2Mode = "AUTO";
          currentModeESP2 = LED_AUTO;
        }
        
        lastESP2State = doc["actualState"].as<bool>();
        lastESP2Online = doc["online"].as<bool>();
        
        Serial.println("Received status from ESP-2: " + payload);
      } else {
        Serial.println("Failed to parse JSON from ESP-2");
      }
    } else {
      server.send(400, "application/json", "{\"error\":\"No data received\"}");
    }
  });

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();  // Handle client requests
  
  // Handle auto mode for ESP-1
  if (currentModeESP1 == LED_AUTO) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_PIN, !ledState);  // LED is active low
      Serial.println(ledState ? "LED ON (Auto)" : "LED OFF (Auto)");
    }
  }
} 