#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <ESPmDNS.h>

// Wi-Fi Credentials
const char* ssid = "NK (2)";
const char* password = "12345678";

// Motor Control Pins
// Front-Left Motor
const int FL_DIR_A = 18;
const int FL_DIR_B = 19;
const int FL_PWM = 5;

// Front-Right Motor
const int FR_DIR_A = 21;
const int FR_DIR_B = 22;
const int FR_PWM = 23;

// Define PWM Channels
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;
const int PWM_FL = 0;
const int PWM_FR = 1;
// Repeat for Rear-Left and Rear-Right if applicable

AsyncWebServer server(80);

// AI Thinker ESP32-CAM IP Address or Hostname
const char* camera_ip = "esp32cam.local"; // Use mDNS or replace with actual IP

// Initialize Motors
void setupMotors() {
    // Front-Left Motor
    pinMode(FL_DIR_A, OUTPUT);
    pinMode(FL_DIR_B, OUTPUT);
    ledcSetup(PWM_FL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(FL_PWM, PWM_FL);

    // Front-Right Motor
    pinMode(FR_DIR_A, OUTPUT);
    pinMode(FR_DIR_B, OUTPUT);
    ledcSetup(PWM_FR, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(FR_PWM, PWM_FR);

    // Repeat setup for Rear-Left and Rear-Right Motors
}

// Function to set motor speed and direction
void setMotor(int dirA, int dirB, int pwmChannel, int speed) {
    if (speed >= 0) {
        digitalWrite(dirA, HIGH);
        digitalWrite(dirB, LOW);
    } else {
        digitalWrite(dirA, LOW);
        digitalWrite(dirB, HIGH);
        speed = -speed;
    }
    // Constrain speed to 0-255
    speed = constrain(speed, 0, 255);
    ledcWrite(pwmChannel, speed);
}

// Handle Movement Commands with Authentication
void handleMove(AsyncWebServerRequest *request) {
    // Implement Basic Authentication (optional but recommended)
    const char* authUsername = "admin";
    const char* authPassword = "password";

    if (!request->authenticate(authUsername, authPassword)) {
        return request->requestAuthentication();
    }

    if (request->hasParam("cmd")) {
        String cmd = request->getParam("cmd")->value();
        Serial.println("Command Received: " + cmd);
        if (cmd == "forward") {
            // Example: Set all motors forward at speed 200
            setMotor(FL_DIR_A, FL_DIR_B, PWM_FL, 200);
            setMotor(FR_DIR_A, FR_DIR_B, PWM_FR, 200);
            // Repeat for Rear-Left and Rear-Right
        }
        else if (cmd == "backward") {
            // Set all motors backward at speed 200
            setMotor(FL_DIR_A, FL_DIR_B, PWM_FL, -200);
            setMotor(FR_DIR_A, FR_DIR_B, PWM_FR, -200);
            // Repeat for Rear-Left and Rear-Right
        }
        else if (cmd == "left") {
            // Strafing left: Front-Left and Rear-Right forward, Front-Right and Rear-Left backward
            setMotor(FL_DIR_A, FL_DIR_B, PWM_FL, 200);
            setMotor(FR_DIR_A, FR_DIR_B, PWM_FR, -200);
            // Repeat for Rear-Left and Rear-Right
        }
        else if (cmd == "right") {
            // Strafing right: Front-Right and Rear-Left forward, Front-Left and Rear-Right backward
            setMotor(FL_DIR_A, FL_DIR_B, PWM_FL, -200);
            setMotor(FR_DIR_A, FR_DIR_B, PWM_FR, 200);
            // Repeat for Rear-Left and Rear-Right
        }
        else if (cmd == "stop") {
            // Stop all motors
            setMotor(FL_DIR_A, FL_DIR_B, PWM_FL, 0);
            setMotor(FR_DIR_A, FR_DIR_B, PWM_FR, 0);
            // Repeat for Rear-Left and Rear-Right
        }
    }
    request->send(200, "text/plain", "OK");
}

void setup() {
    // Initialize Serial Monitor
    Serial.begin(115200);
    delay(1000);
    Serial.println("ESP32-WROOM Motor Control & Web Server");

    // Initialize Motors
    setupMotors();

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        Serial.println(WiFi.status());
    }
    Serial.println("");
    Serial.println("WiFi Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Initialize mDNS
    if (!MDNS.begin("esp32wroom")) {
        Serial.println("Error setting up mDNS responder!");
    
    Serial.println("mDNS responder started. Access via esp32wroom.local");

    // Initialize Web Server Routes
    server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request){
        // Dynamically construct the HTML page with camera IP
        String htmlPage = "<!DOCTYPE html>\n"
                          "<html>\n"
                          "<head>\n"
                          "    <title>Mecanum Robot Control</title>\n"
                          "    <style>\n"
                          "        body { font-family: Arial, sans-serif; text-align: center; }\n"
                          "        .button { padding: 20px; font-size: 16px; margin: 10px; }\n"
                          "        #videoStream { border: 2px solid #000; }\n"
                          "    </style>\n"
                          "</head>\n"
                          "<body>\n"
                          "    <h1>Mecanum Robot Control</h1>\n"
                          "    <div>\n"
                          "        <button class=\"button\" onclick=\"sendCommand('forward')\">Forward</button>\n"
                          "    </div>\n"
                          "    <div>\n"
                          "        <button class=\"button\" onclick=\"sendCommand('backward')\">Backward</button>\n"
                          "    </div>\n"
                          "    <div>\n"
                          "        <button class=\"button\" onclick=\"sendCommand('left')\">Left</button>\n"
                          "        <button class=\"button\" onclick=\"sendCommand('right')\">Right</button>\n"
                          "    </div>\n"
                          "    <div>\n"
                          "        <button class=\"button\" onclick=\"sendCommand('stop')\">Stop</button>\n"
                          "    </div>\n"
                          "    <h2>Live Video Stream</h2>\n"
                          "    <img id=\"videoStream\" src=\"http://" + String(camera_ip) + "/stream\" width=\"640\" height=\"480\" />\n"
                          "    <script>\n"
                          "        function sendCommand(cmd) {\n"
                          "            fetch(`/move?cmd=${cmd}`);\n"
                          "        }\n"
                          "    </script>\n"
                          "</body>\n"
                          "</html>";
        request->send(200, "text/html", htmlPage);
    });

    server.on("/move", HTTP_GET, handleMove);

    // Start Server
    server.begin();
}

void loop() {
    // Nothing needed here
}
