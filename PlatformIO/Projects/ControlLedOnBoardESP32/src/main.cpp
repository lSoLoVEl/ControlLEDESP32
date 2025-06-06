#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi credentials
const char *ssid = "ESP32_NO_1";   // WiFi network name
const char *password = "12345678"; // WiFi password

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

// HTML content
const char htmlContent[] = "<!DOCTYPE html><html><head><title>ESP32 LED Control</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style>body{font-family:Arial,sans-serif;text-align:center;margin:20px}.button{border:none;color:white;padding:15px 32px;text-align:center;text-decoration:none;display:inline-block;font-size:16px;margin:4px 2px;cursor:pointer;border-radius:4px;width:200px}.button-red{background-color:#f44336}.button-green{background-color:#4CAF50}.button-auto{background-color:#2196F3}.button-save{background-color:#FF9800}.button-disabled{background-color:#cccccc;cursor:not-allowed}.input-group{margin:20px 0}input[type=\"number\"]{padding:10px;font-size:16px;width:100px;text-align:center;margin:0 10px}label{font-size:16px}.status-indicator{width:20px;height:20px;border-radius:50%;display:inline-block;margin-left:10px;vertical-align:middle}.status-on{background-color:#808080}.status-off{background-color:#4CAF50}.status-auto{background-color:#FFD700}</style></head><body><h1>ESP32 LED Control</h1><div style=\"margin:20px 0\"><h2>LED Status: <span id=\"ledStatus\">OFF</span><span id=\"statusIndicator\" class=\"status-indicator status-off\"></span></h2></div><p><a href=\"/off\"><button class=\"button button-green\">ON</button></a></p><p><a href=\"/on\"><button class=\"button button-red\">OFF</button></a></p><div class=\"input-group\"><label for=\"interval\">Blink Interval (ms):</label><input type=\"number\" id=\"interval\" name=\"interval\" min=\"100\" max=\"10000\" step=\"100\" oninput=\"checkInput()\"><button id=\"saveButton\" onclick=\"saveInterval()\" class=\"button button-save button-disabled\" disabled>Save Interval</button></div><p><a href=\"/auto\"><button class=\"button button-auto\">AUTO</button></a></p><script>function checkInput(){var input=document.getElementById('interval');var saveButton=document.getElementById('saveButton');if(input.value===''){saveButton.disabled=true;saveButton.classList.add('button-disabled');}else{saveButton.disabled=false;saveButton.classList.remove('button-disabled');}}function saveInterval(){var interval=document.getElementById('interval').value;window.location.href='/saveInterval?interval='+interval;}function updateStatus(){fetch('/status').then(response=>response.json()).then(data=>{var displayStatus = data.status === 'ON' ? 'OFF' : data.status === 'OFF' ? 'ON' : data.status;document.getElementById('ledStatus').textContent=displayStatus;document.getElementById('statusIndicator').className='status-indicator status-'+data.status.toLowerCase();});}setInterval(updateStatus,1000);updateStatus();</script></body></html>";

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
    server.on("/", HTTP_GET, []()
              { server.send(200, "text/html", htmlContent); });

    server.on("/status", HTTP_GET, []()
              {
        String status;
        if (autoMode) {
            status = "AUTO (" + String(interval) + "ms)";
        } else {
            status = digitalRead(ledPin) ? "ON" : "OFF";
        }
        String jsonResponse = "{\"status\":\"" + status + "\"}";
        server.send(200, "application/json", jsonResponse); });

    server.on("/on", HTTP_GET, []()
              {
        autoMode = false;
        digitalWrite(ledPin, HIGH);  // Turn LED on
        server.sendHeader("Location", "/");
        server.send(303); });

    server.on("/off", HTTP_GET, []()
              {
        autoMode = false;
        digitalWrite(ledPin, LOW);   // Turn LED off
        server.sendHeader("Location", "/");
        server.send(303); });

    server.on("/saveInterval", HTTP_GET, []()
              {
        if (server.hasArg("interval")) {
            interval = server.arg("interval").toInt();
            // Ensure interval is within valid range (100ms to 10s)
            if (interval < 100) interval = 100;
            if (interval > 10000) interval = 10000;
            Serial.println("Saved interval: " + String(interval));
        }
        autoMode = true;
        
        server.sendHeader("Location", "/");
        server.send(303); });

    server.on("/auto", HTTP_GET, []()
              {
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
              server.send(303); });

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