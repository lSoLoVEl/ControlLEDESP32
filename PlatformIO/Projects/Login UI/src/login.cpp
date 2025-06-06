#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi credentials (same as main.cpp)
const char *ssid = "ESP32_NO_1";   // WiFi network name
const char *password = "12345678"; // WiFi password

// Login credentials
const char *loginUsername = "admin";
const char *loginPassword = "admin123";

// Web server on port 80 (same as main.cpp)
WebServer server(80);

// HTML content for LED control interface (from main.cpp)
const char htmlContent[] = "<!DOCTYPE html><html><head><title>ESP32 LED Control</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style>body{font-family:Arial,sans-serif;text-align:center;margin:20px}.button{border:none;color:white;padding:15px 32px;text-align:center;text-decoration:none;display:inline-block;font-size:16px;margin:4px 2px;cursor:pointer;border-radius:4px;width:200px}.button-red{background-color:#f44336}.button-green{background-color:#4CAF50}.button-auto{background-color:#2196F3}.button-save{background-color:#FF9800}.button-disabled{background-color:#cccccc;cursor:not-allowed}.input-group{margin:20px 0}input[type=\"number\"]{padding:10px;font-size:16px;width:100px;text-align:center;margin:0 10px}label{font-size:16px}.status-indicator{width:20px;height:20px;border-radius:50%;display:inline-block;margin-left:10px;vertical-align:middle}.status-on{background-color:#808080}.status-off{background-color:#4CAF50}.status-auto{background-color:#FFD700}</style></head><body><h1>ESP32 LED Control</h1><div style=\"margin:20px 0\"><h2>LED Status: <span id=\"ledStatus\">OFF</span><span id=\"statusIndicator\" class=\"status-indicator status-off\"></span></h2></div><p><a href=\"/off\"><button class=\"button button-green\">ON</button></a></p><p><a href=\"/on\"><button class=\"button button-red\">OFF</button></a></p><div class=\"input-group\"><label for=\"interval\">Blink Interval (ms):</label><input type=\"number\" id=\"interval\" name=\"interval\" min=\"100\" max=\"10000\" step=\"100\" oninput=\"checkInput()\"><button id=\"saveButton\" onclick=\"saveInterval()\" class=\"button button-save button-disabled\" disabled>Save Interval</button></div><p><a href=\"/auto\"><button class=\"button button-auto\">AUTO</button></a></p><script>function checkInput(){var input=document.getElementById('interval');var saveButton=document.getElementById('saveButton');if(input.value===''){saveButton.disabled=true;saveButton.classList.add('button-disabled');}else{saveButton.disabled=false;saveButton.classList.remove('button-disabled');}}function saveInterval(){var interval=document.getElementById('interval').value;window.location.href='/saveInterval?interval='+interval;}function updateStatus(){fetch('/status').then(response=>response.json()).then(data=>{var displayStatus = data.status === 'ON' ? 'OFF' : data.status === 'OFF' ? 'ON' : data.status;document.getElementById('ledStatus').textContent=displayStatus;document.getElementById('statusIndicator').className='status-indicator status-'+data.status.toLowerCase();});}setInterval(updateStatus,1000);updateStatus();</script></body></html>";

// HTML content for login page
const char loginHtml[] = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ระบบควบคุม LED ESP32</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: 'Sarabun', sans-serif;
            background-color: #f0f2f5;
            margin: 0;
            padding: 20px;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
        }
        .login-container {
            background-color: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            width: 320px;
        }
        h1 {
            text-align: center;
            color: #1a73e8;
            margin-bottom: 30px;
            font-size: 24px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 500;
        }
        input[type="text"], input[type="password"] {
            width: 100%;
            padding: 12px;
            border: 1px solid #ddd;
            border-radius: 6px;
            box-sizing: border-box;
            font-size: 16px;
            transition: border-color 0.3s;
        }
        input[type="text"]:focus, input[type="password"]:focus {
            border-color: #1a73e8;
            outline: none;
        }
        button {
            width: 100%;
            padding: 12px;
            background-color: #1a73e8;
            color: white;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            font-size: 16px;
            font-weight: 500;
            transition: background-color 0.3s;
        }
        button:hover {
            background-color: #1557b0;
        }
        .error-message {
            color: #d93025;
            text-align: center;
            margin-top: 15px;
            font-size: 14px;
        }
        .wifi-info {
            text-align: center;
            margin-top: 20px;
            padding-top: 20px;
            border-top: 1px solid #eee;
            color: #666;
            font-size: 14px;
        }
    </style>
    <link href="https://fonts.googleapis.com/css2?family=Sarabun:wght@400;500&display=swap" rel="stylesheet">
</head>
<body>
    <div class="login-container">
        <h1>เข้าสู่ระบบควบคุม LED</h1>
        <form action="/login" method="POST">
            <div class="form-group">
                <label for="username">ชื่อผู้ใช้:</label>
                <input type="text" id="username" name="username" required placeholder="กรอกชื่อผู้ใช้">
            </div>
            <div class="form-group">
                <label for="password">รหัสผ่าน:</label>
                <input type="password" id="password" name="password" required placeholder="กรอกรหัสผ่าน">
            </div>
            <button type="submit">เข้าสู่ระบบ</button>
        </form>
        <div id="error" class="error-message"></div>
        <div class="wifi-info">
            <p>เชื่อมต่อ WiFi: ESP32_NO_1</p>
            <p>รหัสผ่าน WiFi: 12345678</p>
        </div>
    </div>
    <script>
        document.querySelector('form').onsubmit = function(e) {
            e.preventDefault();
            const username = document.getElementById('username').value;
            const password = document.getElementById('password').value;
            
            fetch('/login', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `username=${encodeURIComponent(username)}&password=${encodeURIComponent(password)}`
            })
            .then(response => response.text())
            .then(data => {
                if (data === 'success') {
                    window.location.href = '/control';
                } else {
                    document.getElementById('error').textContent = 'ชื่อผู้ใช้หรือรหัสผ่านไม่ถูกต้อง';
                }
            });
        };
    </script>
</body>
</html>
)";

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n");
    Serial.println("==========================================");
    Serial.println("เริ่มต้นระบบเข้าสู่ระบบ ESP32...");
    Serial.println("==========================================");

    // Configure WiFi
    WiFi.softAP(ssid, password);

    Serial.println("\n=== ข้อมูล WiFi ESP32 ===");
    Serial.println("เริ่มต้นจุดเชื่อมต่อ WiFi");
    Serial.print("ชื่อเครือข่าย (SSID): ");
    Serial.println(ssid);
    Serial.print("รหัสผ่าน: ");
    Serial.println(password);
    Serial.print("ที่อยู่ IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("=============================\n");

    // Define server routes
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", loginHtml);
    });

    server.on("/login", HTTP_POST, []() {
        String username = server.arg("username");
        String password = server.arg("password");
        
        if (username == loginUsername && password == loginPassword) {
            server.send(200, "text/plain", "success");
        } else {
            server.send(401, "text/plain", "error");
        }
    });

    server.on("/control", HTTP_GET, []() {
        // Send the main LED control interface HTML
        server.send(200, "text/html", htmlContent);
    });

    // Add routes for LED control
    server.on("/on", HTTP_GET, []() {
        digitalWrite(5, HIGH);
        server.sendHeader("Location", "/control");
        server.send(303);
    });

    server.on("/off", HTTP_GET, []() {
        digitalWrite(5, LOW);
        server.sendHeader("Location", "/control");
        server.send(303);
    });

    server.on("/auto", HTTP_GET, []() {
        // Implement auto mode logic here
        server.sendHeader("Location", "/control");
        server.send(303);
    });

    server.on("/status", HTTP_GET, []() {
        String status = digitalRead(5) ? "ON" : "OFF";
        String jsonResponse = "{\"status\":\"" + status + "\"}";
        server.send(200, "application/json", jsonResponse);
    });

    server.on("/saveInterval", HTTP_GET, []() {
        if (server.hasArg("interval")) {
            // Handle interval saving logic here
        }
        server.sendHeader("Location", "/control");
        server.send(303);
    });

    // Start server
    server.begin();
    Serial.println("เริ่มต้นเซิร์ฟเวอร์ HTTP");
}

void loop() {
    server.handleClient();
    delay(10);
}