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
const char* esp2_ip = "192.168.4.1";  // IP ของ ESP-2 ในเครือข่าย AP

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
  String url = "http://" + String(esp2_ip) + "/interval/" + String(newInterval);
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

// HTML content
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP-1 Control Panel</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin: 0px auto;
      padding: 20px;
    }
    .container {
      display: flex;
      justify-content: space-around;
      flex-wrap: wrap;
    }
    .device {
      margin: 20px;
      padding: 20px;
      border: 1px solid #ccc;
      border-radius: 8px;
      width: 300px;
    }
    .button {
      background-color: #4CAF50;
      border: none;
      color: white;
      padding: 15px 32px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 16px;
      margin: 4px 2px;
      cursor: pointer;
      border-radius: 4px;
    }
    .button-off {
      background-color: #f44336;
    }
    .button-auto {
      background-color: #2196F3;
    }
    .slider {
      width: 300px;
      margin: 20px;
    }
    .status {
      margin: 20px;
      padding: 10px;
      border-radius: 4px;
      background-color: #f0f0f0;
    }
    .wifi-info {
      background-color: #e3f2fd;
      padding: 10px;
      border-radius: 4px;
      margin: 20px 0;
    }
  </style>
</head>
<body>
  <h1>ESP Control Panel</h1>
  
  <div class="wifi-info">
    <h3>WiFi Information</h3>
    <p>SSID: ESP32-yabai</p>
    <p>Password: 12345678</p>
  </div>

  <div class="container">
    <!-- ESP-1 Control -->
    <div class="device">
      <h2>ESP-1 Control</h2>
      <div class="status">
        <p>Board Status: <span id="boardStatus1" style="font-weight: bold;">ONLINE</span></p>
        <p>Current Mode: <span id="mode1">OFF</span></p>
        <p>LED Status: <span id="ledStatus1">OFF</span></p>
        <p>Interval: <span id="interval1">1000</span>ms</p>
      </div>
      <p>
        <button class="button button-off" onclick="controlLED('off', 1)">OFF</button>
        <button class="button" onclick="controlLED('on', 1)">ON</button>
        <button class="button button-auto" onclick="controlLED('auto', 1)">AUTO</button>
      </p>
      <p>
        <label for="interval1">Blink Interval (ms):</label><br>
        <input type="range" min="100" max="5000" value="1000" class="slider" id="interval1" onchange="updateIntervalValue(this.value, 1)">
        <input type="number" min="100" max="5000" value="1000" id="intervalInput1" style="width: 80px; margin: 0 10px;">
        <button class="button" onclick="updateInterval(1)">Confirm</button>
        <span id="intervalValue1">1000</span>ms
      </p>
    </div>

    <!-- ESP-2 Control -->
    <div class="device">
      <h2>ESP-2 Control</h2>
      <div class="status">
        <p>Board Status: <span id="boardStatus2" style="font-weight: bold;">ONLINE</span></p>
        <p>Current Mode: <span id="mode2">OFF</span></p>
        <p>LED Status: <span id="ledStatus2">OFF</span></p>
        <p>Interval: <span id="interval2">1000</span>ms</p>
      </div>
      <p>
        <button class="button button-off" onclick="controlLED('off', 2)">OFF</button>
        <button class="button" onclick="controlLED('on', 2)">ON</button>
        <button class="button button-auto" onclick="controlLED('auto', 2)">AUTO</button>
      </p>
      <p>
        <label for="interval2">Blink Interval (ms):</label><br>
        <input type="range" min="100" max="5000" value="1000" class="slider" id="interval2" onchange="updateIntervalValue(this.value, 2)">
        <input type="number" min="100" max="5000" value="1000" id="intervalInput2" style="width: 80px; margin: 0 10px;">
        <button class="button" onclick="updateInterval(2)">Confirm</button>
        <span id="intervalValue2">1000</span>ms
      </p>
    </div>
  </div>

  <script>
    // Function to check board status
    function checkBoardStatus(device) {
      fetch('/status/' + device)
        .then(response => {
          if (response.ok) {
            return response.json();
          }
          throw new Error('Network response was not ok');
        })
        .then(data => {
          if (device === 2) {
            // For ESP-2, only update the online status
            document.getElementById('boardStatus' + device).innerHTML = data.online ? 'ONLINE' : 'OFFLINE';
            document.getElementById('boardStatus' + device).style.color = data.online ? '#4CAF50' : '#f44336';
            
            // Only update other values if we got a successful response
            if (data.online) {
              document.getElementById('mode' + device).innerHTML = data.mode;
              document.getElementById('ledStatus' + device).innerHTML = data.actualState ? 'ON' : 'OFF';
            }
          } else {
            // For ESP-1, update everything as before
            document.getElementById('boardStatus' + device).innerHTML = 'ONLINE';
            document.getElementById('boardStatus' + device).style.color = '#4CAF50';
            document.getElementById('mode' + device).innerHTML = data.mode;
            document.getElementById('ledStatus' + device).innerHTML = data.actualState ? 'ON' : 'OFF';
          }
          
          // Check if LED state matches the mode
          const statusDiv = document.querySelector('.device:nth-child(' + device + ') .status');
          if ((data.mode === 'ON' && !data.actualState) || 
              (data.mode === 'OFF' && data.actualState)) {
            statusDiv.style.backgroundColor = '#ffebee';
            if (!statusDiv.querySelector('.warning')) {
              statusDiv.innerHTML += '<p class="warning" style="color: red;">Warning: LED may be malfunctioning!</p>';
            }
          } else {
            statusDiv.style.backgroundColor = '#f0f0f0';
            const warning = statusDiv.querySelector('.warning');
            if (warning) {
              warning.remove();
            }
          }
        })
        .catch(error => {
          if (device === 1) {  // Only update ESP-1 status on error
            document.getElementById('boardStatus' + device).innerHTML = 'OFFLINE';
            document.getElementById('boardStatus' + device).style.color = '#f44336';
            document.getElementById('mode' + device).innerHTML = 'UNKNOWN';
            document.getElementById('ledStatus' + device).innerHTML = 'UNKNOWN';
          }
        });
    }

    // Check status for both boards every 5 seconds instead of 2
    setInterval(() => {
      checkBoardStatus(1);
      checkBoardStatus(2);
    }, 5000);

    // Initial check
    checkBoardStatus(1);
    checkBoardStatus(2);

    function controlLED(mode, device) {
      fetch('/led/' + mode + '/' + device)
        .then(response => response.json())
        .then(data => {
          document.getElementById('mode' + device).innerHTML = data.mode;
          if (mode === 'auto') {
            document.getElementById('interval' + device).value = 1000;
            document.getElementById('intervalInput' + device).value = 1000;
            document.getElementById('intervalValue' + device).innerHTML = 1000;
            fetch('/interval/' + device + '?value=1000')
              .then(response => response.json())
              .then(data => {
                document.getElementById('interval' + device).innerHTML = data.interval;
              });
          }
        });
    }

    function updateIntervalValue(value, device) {
      document.getElementById('intervalValue' + device).innerHTML = value;
      document.getElementById('intervalInput' + device).value = value;
    }

    function updateInterval(device) {
      const value = document.getElementById('intervalInput' + device).value;
      if (value >= 100 && value <= 5000) {
        document.getElementById('interval' + device).value = value;
        document.getElementById('intervalValue' + device).innerHTML = value;
        fetch('/interval/' + device + '?value=' + value)
          .then(response => response.json())
          .then(data => {
            document.getElementById('interval' + device).innerHTML = data.interval;
          });
      } else {
        alert('Please enter a value between 100 and 5000');
      }
    }
  </script>
</body>
</html>
)rawliteral";

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
      
      // Simple string parsing instead of JSON
      if (payload.indexOf("\"mode\":\"OFF\"") != -1) currentModeESP2 = LED_OFF;
      else if (payload.indexOf("\"mode\":\"ON\"") != -1) currentModeESP2 = LED_ON;
      else if (payload.indexOf("\"mode\":\"AUTO\"") != -1) currentModeESP2 = LED_AUTO;
      
      Serial.println("Received status from ESP-2: " + payload);
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