#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi credentials
const char *ssid = "ESP32_NO_1";   // WiFi network name
const char *password = "12345678"; // WiFi password

// Authentication credentials
const char* www_username = "admin";
const char* www_password = "esp32";

// LED pin
const int ledPin = 5;

// Web server on port 80
WebServer server(80);

// Variables for LED control
bool autoMode = false;
unsigned long previousMillis = 0;
long interval = 1000; // Default 1 second interval

// Function to handle WiFi events
void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case SYSTEM_EVENT_AP_STACONNECTED:
        Serial.println("\n=== New Device Connected ===");
        Serial.print("Total connected devices: ");
        Serial.println(WiFi.softAPgetStationNum());
        Serial.println("===========================\n");
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        Serial.println("\n=== Device Disconnected ===");
        Serial.print("Remaining connected devices: ");
        Serial.println(WiFi.softAPgetStationNum());
        Serial.println("===========================\n");
        break;
    }
}

// Login page HTML
const char loginHtml[] = "<!DOCTYPE html><html><head><title>ESP32 Login</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style>body{font-family:Arial,sans-serif;text-align:center;margin:0;padding:10px;background-color:#000000;color:#00BFFF;min-height:100vh;display:flex;flex-direction:column;align-items:center;justify-content:center}.login-form{background-color:#000000;padding:20px;border-radius:10px;margin:20px auto;max-width:400px;width:90%;box-shadow:0 2px 4px rgba(0,0,0,0.1);border:1px solid #00BFFF}.input-group{margin:15px 0}input[type=\"text\"],input[type=\"password\"]{width:100%;padding:10px;margin:5px 0;border:1px solid #00BFFF;border-radius:4px;background-color:#000000;color:#00BFFF;box-sizing:border-box}.button{background-color:#2196F3;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;width:100%;font-size:16px;margin-top:10px}.button:hover{opacity:0.9}.error-message{color:#f44336;margin-top:10px}</style></head><body><div class=\"login-form\"><h2>ESP32 Login</h2><form action=\"/login\" method=\"POST\"><div class=\"input-group\"><input type=\"text\" name=\"username\" placeholder=\"Username\" required></div><div class=\"input-group\"><input type=\"password\" name=\"password\" placeholder=\"Password\" required></div><button type=\"submit\" class=\"button\">Login</button></form></div></body></html>";

// Control page HTML with responsive design
const char htmlContent[] = "<!DOCTYPE html><html><head><title>ESP32 LED Control</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style>body{font-family:Arial,sans-serif;text-align:center;margin:0;padding:10px;background-color:#000000;color:#00BFFF;min-height:100vh;display:flex;flex-direction:column;align-items:center;justify-content:center}.button{border:none;color:white;padding:15px 32px;text-align:center;text-decoration:none;display:inline-block;font-size:16px;margin:4px 2px;cursor:pointer;border-radius:4px;width:200px;transition:all 0.3s ease}.button:hover{transform:scale(1.05);opacity:0.9}.button-red{background-color:#f44336}.button-green{background-color:#4CAF50}.button-auto{background-color:#2196F3}.button-save{background-color:#FF9800}.button-disabled{background-color:#cccccc;cursor:not-allowed}.input-group{margin:20px 0;display:flex;flex-direction:column;align-items:center;gap:10px}.input-group h3{margin:10px 0}input[type=\"number\"]{padding:10px;font-size:16px;width:80px;text-align:center;margin:0 5px;background-color:#000000;color:#00BFFF;border:1px solid #00BFFF;border-radius:4px}label{font-size:16px;color:#00BFFF;margin:5px 0}.status-indicator{width:20px;height:20px;border-radius:50%;display:inline-block;margin-left:10px;vertical-align:middle}.status-on{background-color:#808080}.status-off{background-color:#4CAF50}.status-auto{background-color:#FFD700}.header-frame{background-color:#000000;padding:20px;border-radius:10px;margin:20px auto;max-width:600px;width:90%;box-shadow:0 2px 4px rgba(0,0,0,0.1);border:1px solid #00BFFF}.logout-button{position:absolute;top:10px;right:10px;background-color:#f44336;color:white;padding:8px 16px;border-radius:4px;text-decoration:none;font-size:14px}@media (max-width: 480px){.button{width:90%;max-width:300px;padding:12px 24px;font-size:14px}.input-group{margin:15px 0}.input-group h3{font-size:18px}input[type=\"number\"]{width:60px;padding:8px;font-size:14px}label{font-size:14px}.header-frame{padding:15px;margin:10px auto}.logout-button{top:5px;right:5px;padding:6px 12px;font-size:12px}}</style></head><body><a href=\"/logout\" class=\"logout-button\">Logout</a><div class=\"header-frame\"><h1>ESP32 LED Control</h1><div style=\"margin:20px 0\"><h2>LED Status: <span id=\"ledStatus\">OFF</span><span id=\"statusIndicator\" class=\"status-indicator status-off\"></span></h2></div></div><p><a href=\"/off\"><button class=\"button button-green\">ON</button></a></p><p><a href=\"/on\"><button class=\"button button-red\">OFF</button></a></p><div class=\"input-group\"><label for=\"interval\">Blink Interval (ms):</label><input type=\"number\" id=\"interval\" name=\"interval\" min=\"100\" max=\"10000\" step=\"100\" oninput=\"checkInput()\"><button id=\"saveButton\" onclick=\"saveInterval()\" class=\"button button-save button-disabled\" disabled>Save Interval</button></div><p><a href=\"/auto\"><button class=\"button button-auto\">AUTO</button></a></p><script>function checkInput(){var input=document.getElementById('interval');var saveButton=document.getElementById('saveButton');if(input.value===''){saveButton.disabled=true;saveButton.classList.add('button-disabled');}else{saveButton.disabled=false;saveButton.classList.remove('button-disabled');}}function saveInterval(){var interval=document.getElementById('interval').value;window.location.href='/saveInterval?interval='+interval;}function updateStatus(){fetch('/status').then(response=>response.json()).then(data=>{var displayStatus = data.status === 'ON' ? 'OFF' : data.status === 'OFF' ? 'ON' : data.status;document.getElementById('ledStatus').textContent=displayStatus;document.getElementById('statusIndicator').className='status-indicator status-'+data.status.toLowerCase();});}setInterval(updateStatus,1000);updateStatus();</script></body></html>";

void setup()
{
    Serial.begin(115200);
    delay(1000); // Wait for serial to initialize

    Serial.println("\n\n");
    Serial.println("==========================================");
    Serial.println("ESP32 WiFi LED Control System Starting...");
    Serial.println("==========================================");

    // Configure LED pin
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW); // Start with LED off

    // Register WiFi event handler
    WiFi.onEvent(WiFiEvent);

    // Create WiFi access point
    WiFi.softAP(ssid, password);

    Serial.println("\n=== ESP32 WiFi Information ===");
    Serial.println("WiFi Access Point Started");
    Serial.print("Network Name (SSID): ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("=============================\n");

    Serial.println("Waiting for devices to connect...");
    Serial.println("(Connect to the WiFi network to control the LED)");
    Serial.println("==========================================\n");

    // Define server routes
    server.on("/", HTTP_GET, []() {
        if (!server.authenticate(www_username, www_password)) {
            return server.requestAuthentication();
        }
        server.send(200, "text/html", htmlContent);
    });

    server.on("/logout", HTTP_GET, []() {
        server.sendHeader("Location", "/login");
        server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server.sendHeader("Pragma", "no-cache");
        server.sendHeader("Expires", "-1");
        server.send(303);
    });

    server.on("/login", HTTP_GET, []() {
        server.send(200, "text/html", loginHtml);
    });

    server.on("/login", HTTP_POST, []() {
        if (server.hasArg("username") && server.hasArg("password")) {
            if (server.arg("username") == www_username && server.arg("password") == www_password) {
                server.sendHeader("Location", "/");
                server.send(303);
            } else {
                server.send(200, "text/html", loginHtml);
            }
        } else {
            server.send(200, "text/html", loginHtml);
        }
    });

    server.on("/status", HTTP_GET, []() {
        if (!server.authenticate(www_username, www_password)) {
            return server.requestAuthentication();
        }
        String status;
        if (autoMode) {
            status = "AUTO (" + String(interval) + "ms)";
        } else {
            status = digitalRead(ledPin) ? "ON" : "OFF";
        }
        String jsonResponse = "{\"status\":\"" + status + "\"}";
        server.send(200, "application/json", jsonResponse);
    });

    server.on("/on", HTTP_GET, []() {
        if (!server.authenticate(www_username, www_password)) {
            return server.requestAuthentication();
        }
        autoMode = false;
        digitalWrite(ledPin, HIGH);  // Turn LED on
        server.sendHeader("Location", "/");
        server.send(303);
    });

    server.on("/off", HTTP_GET, []() {
        if (!server.authenticate(www_username, www_password)) {
            return server.requestAuthentication();
        }
        autoMode = false;
        digitalWrite(ledPin, LOW);   // Turn LED off
        server.sendHeader("Location", "/");
        server.send(303);
    });

    server.on("/saveInterval", HTTP_GET, []() {
        if (!server.authenticate(www_username, www_password)) {
            return server.requestAuthentication();
        }
        if (server.hasArg("interval")) {
            interval = server.arg("interval").toInt();
            // Ensure interval is within valid range (100ms to 10s)
            if (interval < 100) interval = 100;
            if (interval > 10000) interval = 10000;
            Serial.println("Saved interval: " + String(interval));
        }
        autoMode = true;
        server.sendHeader("Location", "/");
        server.send(303);
    });

    server.on("/auto", HTTP_GET, []() {
        if (!server.authenticate(www_username, www_password)) {
            return server.requestAuthentication();
        }
        if (server.hasArg("interval")) {
            interval = server.arg("interval").toInt();
            // Ensure interval is within valid range (100ms to 10s)
            if (interval < 100) interval = 100;
            if (interval > 10000) interval = 10000;
            Serial.println("Saved interval: " + String(interval));
        } else {
            interval = 1000; // Default to 1000ms if no interval specified
            Serial.println("Using default interval: " + String(interval));
        }

        autoMode = true;
        server.sendHeader("Location", "/");
        server.send(303);
    });

    // Start server
    server.begin();
    Serial.println("HTTP server started");
}

void loop()
{
    server.handleClient();

    // Handle auto mode
    if (autoMode)
    {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval)
        {
            Serial.println("autoMode: " + String(autoMode) + " | interval : " + String(interval));
            previousMillis = currentMillis;
            digitalWrite(ledPin, !digitalRead(ledPin)); // Toggle LED state
        }
    }

    delay(10);
}