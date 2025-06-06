#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi Access Point credentials
const char* ap_ssid = "ESP32-AP";
const char* ap_password = "12345678";

// Create WebServer object
WebServer server(80);

// Function to get WiFi networks as HTML options
String getWiFiNetworks() {
    String networks = "";
    int n = WiFi.scanNetworks();
    
    if (n == 0) {
        networks = "<option value=''>No networks found!</option>";
    } else {
        for (int i = 0; i < n; ++i) {
            networks += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + 
                       " (" + String(WiFi.RSSI(i)) + " dBm)" +
                       (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? " - Open" : " - Protected") +
                       "</option>";
        }
    }
    return networks;
}

// HTML content
const char* html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Access Point</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f0f0f0;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
        }
        h1, h2 {
            color: #333;
            text-align: center;
        }
        .button {
            display: inline-block;
            padding: 10px 20px;
            background-color: #4CAF50;
            color: white;
            text-decoration: none;
            border-radius: 5px;
            margin: 10px;
            border: none;
            cursor: pointer;
        }
        .button:hover {
            background-color: #45a049;
        }
        .form-group {
            margin: 15px 0;
        }
        label {
            display: block;
            margin-bottom: 5px;
        }
        input[type="text"], input[type="password"], select {
            width: 100%;
            padding: 8px;
            margin-bottom: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }
        select {
            background-color: white;
        }
        .status {
            text-align: center;
            margin: 20px 0;
            padding: 10px;
            border-radius: 5px;
        }
        .connected {
            background-color: #dff0d8;
            color: #3c763d;
        }
        .scan-button {
            margin: 10px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 Access Point</h1>
        
        <div class="status connected">
            <p>Access Point: %AP_SSID%</p>
            <p>IP Address: %AP_IP%</p>
            <p>Connected Devices: %CONNECTED_DEVICES%</p>
        </div>
        
        <h2>Connect to WiFi</h2>
        <form action="/connect" method="POST">
            <div class="form-group">
                <label for="ssid">Select WiFi Network:</label>
                <select id="ssid" name="ssid" required>
                    <option value="">Select a network...</option>
                    %WIFI_LIST%
                </select>
            </div>
            <div class="form-group">
                <label for="password">Password:</label>
                <input type="password" id="password" name="password">
            </div>
            <button type="submit" class="button">Connect</button>
        </form>
    </div>
</body>
</html>
)";

void setup() {
    Serial.begin(115200);
    
    // Set up Access Point
    WiFi.softAP(ap_ssid, ap_password);
    
    Serial.println("Access Point Started");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    
    // Define routes
    server.on("/", HTTP_GET, []() {
        String htmlContent = String(html);
        htmlContent.replace("%WIFI_LIST%", getWiFiNetworks());
        htmlContent.replace("%AP_SSID%", ap_ssid);
        htmlContent.replace("%AP_IP%", WiFi.softAPIP().toString());
        htmlContent.replace("%CONNECTED_DEVICES%", String(WiFi.softAPgetStationNum()));
        server.send(200, "text/html", htmlContent);
    });
    
    server.on("/connect", HTTP_POST, []() {
        String newSSID = server.arg("ssid");
        String newPassword = server.arg("password");
        
        // Disconnect from current WiFi
        WiFi.disconnect();
        
        // Connect to new WiFi
        WiFi.begin(newSSID.c_str(), newPassword.c_str());
        
        // Send response
        server.sendHeader("Location", "/");
        server.send(303);
    });
    
    // Start server
    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
}