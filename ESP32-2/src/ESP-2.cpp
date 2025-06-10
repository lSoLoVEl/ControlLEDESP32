#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

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

// Add these global variables for ESP-1 status
String lastESP1Mode = "UNKNOWN";
bool lastESP1State = false;
bool lastESP1Online = false;

// Create WebServer object on port 80
WebServer server(80);

// HTML content for the web interface
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-2 LED Control</title>
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
    <h1>ESP32-2 LED Control</h1>
    
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
            <button class="button" onclick="controlESP1('on')">ON</button>
            <button class="button" onclick="controlESP1('off')">OFF</button>
            <button class="button" onclick="controlESP1('auto')">AUTO</button>
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
            <span id="esp2-connection" class="connection-status online">Online</span>
        </div>
        <div class="status-details">
            <span class="status-label">Last Update:</span>
            <span id="esp2-last-update">Never</span>
        </div>
        <div>
            <button class="button" onclick="controlLED('on')">ON</button>
            <button class="button" onclick="controlLED('off')">OFF</button>
            <button class="button" onclick="controlLED('auto')">AUTO</button>
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
            fetch('/status/esp1')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('esp1-mode').textContent = data.mode;
                    const ledElement = document.getElementById('esp1-led');
                    ledElement.className = 'led-status ' + (data.actualState ? 'led-on' : 'led-off');
                    const connectionElement = document.getElementById('esp1-connection');
                    connectionElement.textContent = data.online ? 'Online' : 'Offline';
                    connectionElement.className = 'connection-status ' + (data.online ? 'online' : 'offline');
                    document.getElementById('esp1-last-update').textContent = formatTime();
                });

            // Update ESP-2 status
            fetch('/status/2')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('esp2-mode').textContent = data.mode;
                    const ledElement = document.getElementById('esp2-led');
                    ledElement.className = 'led-status ' + (data.actualState ? 'led-on' : 'led-off');
                    document.getElementById('esp2-last-update').textContent = formatTime();
                });
        }

        function controlLED(mode) {
            fetch('/led/' + mode)
                .then(response => response.json())
                .then(data => {
                    console.log('LED control response:', data);
                    // Update status immediately after control
                    updateStatus();
                })
                .catch(error => {
                    console.error('Error controlling LED:', error);
                });
        }

        function controlESP1(mode) {
            fetch('/control/esp1/' + mode)
                .then(response => response.json())
                .then(data => {
                    console.log('ESP1 control response:', data);
                    // Update status immediately after control
                    updateStatus();
                })
                .catch(error => {
                    console.error('Error controlling ESP1:', error);
                });
        }

        function setInterval() {
            const value = document.getElementById('interval').value;
            Promise.all([
                fetch('/interval?value=' + value),
                fetch('/control/esp1/interval?value=' + value)
            ]).then(() => {
                console.log('Interval updated for both ESPs');
                // Update status after interval change
                updateStatus();
            })
            .catch(error => {
                console.error('Error setting interval:', error);
            });
        }

        // Update status every 1 second for more responsive UI
        setInterval(updateStatus, 1000);
        // Initial status update
        updateStatus();
    </script>
</body>
</html>
)rawliteral";

// Function to control ESP-1
void controlESP1(String command) {
  HTTPClient http;
  String url = "http://" + String(esp1_ip) + "/led/" + command + "/1";
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("ESP-1 Response: " + payload);
  } else {
    Serial.println("Error on HTTP request to ESP-1");
  }
  http.end();
}

// Function to set interval on ESP-1
void setESP1Interval(long newInterval) {
  HTTPClient http;
  String url = "http://" + String(esp1_ip) + "/interval/1?value=" + String(newInterval);
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("ESP-1 Interval Response: " + payload);
  } else {
    Serial.println("Error setting interval on ESP-1");
  }
  http.end();
}

// Function to send status to ESP-1
void sendStatusToESP1() {
  HTTPClient http;
  String url = "http://" + String(esp1_ip) + "/status/2";
  http.begin(url);
  
  // Create JSON response with mode, actualState, and online status
  String jsonResponse = "{\"mode\":\"";
  if (currentMode == LED_OFF) jsonResponse += "OFF";
  else if (currentMode == LED_ON) jsonResponse += "ON";
  else jsonResponse += "AUTO";
  jsonResponse += "\",\"actualState\":" + String(!digitalRead(LED_PIN)) + 
                 ",\"online\":true}";
  
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

  // Route for root / web page
  server.on("/", HTTP_GET, []() {
    Serial.println("Received request for root page");
    server.send(200, "text/html", index_html);
  });

  // Route for LED control
  server.on("/led/off", HTTP_GET, []() {
    Serial.println("Turning LED OFF");
    currentMode = LED_OFF;
    digitalWrite(LED_PIN, HIGH);  // LED is active low
    ledState = false;  // Reset auto mode state
    server.send(200, "application/json", "{\"mode\":\"OFF\"}");
    sendStatusToESP1();
    // Send immediate status update
    String response = "{\"mode\":\"OFF\",\"actualState\":false,\"online\":true}";
    server.send(200, "application/json", response);
  });

  server.on("/led/on", HTTP_GET, []() {
    Serial.println("Turning LED ON");
    currentMode = LED_ON;
    digitalWrite(LED_PIN, LOW);  // LED is active low
    ledState = true;  // Reset auto mode state
    server.send(200, "application/json", "{\"mode\":\"ON\"}");
    sendStatusToESP1();
    // Send immediate status update
    String response = "{\"mode\":\"ON\",\"actualState\":true,\"online\":true}";
    server.send(200, "application/json", response);
  });

  server.on("/led/auto", HTTP_GET, []() {
    Serial.println("Setting LED to AUTO mode");
    currentMode = LED_AUTO;
    server.send(200, "application/json", "{\"mode\":\"AUTO\"}");
    sendStatusToESP1();
    // Send immediate status update
    String response = "{\"mode\":\"AUTO\",\"actualState\":" + String(!digitalRead(LED_PIN)) + ",\"online\":true}";
    server.send(200, "application/json", response);
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

  // Route for controlling ESP-1
  server.on("/control/esp1/off", HTTP_GET, []() {
    controlESP1("off");
    server.send(200, "application/json", "{\"status\":\"sent\"}");
  });

  server.on("/control/esp1/on", HTTP_GET, []() {
    controlESP1("on");
    server.send(200, "application/json", "{\"status\":\"sent\"}");
  });

  server.on("/control/esp1/auto", HTTP_GET, []() {
    controlESP1("auto");
    server.send(200, "application/json", "{\"status\":\"sent\"}");
  });

  server.on("/control/esp1/interval", HTTP_GET, []() {
    if (server.hasArg("value")) {
      long newInterval = server.arg("value").toInt();
      if (newInterval > 0) {
        setESP1Interval(newInterval);
        server.send(200, "application/json", "{\"status\":\"sent\"}");
      } else {
        server.send(400, "application/json", "{\"error\":\"Invalid interval value\"}");
      }
    } else {
      server.send(400, "application/json", "{\"error\":\"No interval value provided\"}");
    }
  });

  // Route for checking ESP-1 status
  server.on("/status/esp1", HTTP_GET, []() {
    static unsigned long lastCheck = 0;
    static String lastResponse = "{\"mode\":\"UNKNOWN\",\"actualState\":false,\"online\":false}";
    
    // Only check every 5 seconds
    if (millis() - lastCheck >= 5000) {
      HTTPClient http;
      String url = "http://" + String(esp1_ip) + "/status/1";
      http.begin(url);
      http.setTimeout(1000); // Set timeout to 1 second
      int httpCode = http.GET();
      
      if (httpCode > 0) {
        lastResponse = http.getString();
        // Parse and store the last known values
        if (lastResponse.indexOf("\"mode\":\"OFF\"") != -1) {
          lastESP1Mode = "OFF";
          lastESP1Online = true;
        }
        else if (lastResponse.indexOf("\"mode\":\"ON\"") != -1) {
          lastESP1Mode = "ON";
          lastESP1Online = true;
        }
        else if (lastResponse.indexOf("\"mode\":\"AUTO\"") != -1) {
          lastESP1Mode = "AUTO";
          lastESP1Online = true;
        }
        
        if (lastResponse.indexOf("\"actualState\":true") != -1) {
          lastESP1State = true;
        } else if (lastResponse.indexOf("\"actualState\":false") != -1) {
          lastESP1State = false;
        }
      } else {
        lastESP1Online = false;
      }
      http.end();
      lastCheck = millis();
    }
    
    // Create response with last known values
    String response = "{\"mode\":\"" + lastESP1Mode + 
                     "\",\"actualState\":" + String(lastESP1State) + 
                     ",\"online\":" + String(lastESP1Online) + "}";
    server.send(200, "application/json", response);
  });

  // Route for checking ESP-2 status
  server.on("/status/2", HTTP_GET, []() {
    String response = "{\"mode\":\"";
    if (currentMode == LED_OFF) response += "OFF";
    else if (currentMode == LED_ON) response += "ON";
    else response += "AUTO";
    response += "\",\"actualState\":" + String(!digitalRead(LED_PIN)) + 
               ",\"online\":true}";
    server.send(200, "application/json", response);
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