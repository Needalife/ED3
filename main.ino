/*
  Mecanum Wheeled Robot Control with ESP32 and BTS7960 Motor Drivers
  Author: Saurabh
  Date: Jab 2025
*/

#include <WiFi.h>
#include <WebServer.h>
// #include <Encoder.h> // Encoders are disabled to prevent ISR conflicts
#include <secrets.h>    // Make sure to define your WiFi credentials here

WebServer server(80);

// ========================= Motor Driver Pin Definitions =========================
const int MOTOR1_RPWM = 26; // GPIO26 (D26)
const int MOTOR1_LPWM = 25; // GPIO25 (D25)

const int MOTOR2_RPWM = 15; // GPIO15 (D15)
const int MOTOR2_LPWM = 4;  // GPIO4  (D4)

const int MOTOR3_RPWM = 12; // GPIO12 (D12) - Reassigned to channels 8 and 9
const int MOTOR3_LPWM = 13; // GPIO13 (D13)

const int MOTOR4_RPWM = 19; // GPIO19 (D19)
const int MOTOR4_LPWM = 18; // GPIO18 (D18)

// ========================= PWM Configuration =========================
const int PWM_FREQ = 1000;      // PWM frequency in Hz
const int PWM_RESOLUTION = 10;  // 10-bit resolution (0-1023)
const int PWM_CHANNELS = 10;    // Update to include channels 0-9

// ========================= Function Prototypes =========================
void initializeMotors();
void initializePWM();
void moveForward(int speed);
void moveBackward(int speed);
void strafeLeft(int speed);
void strafeRight(int speed);
void rotateLeft(int speed);
void rotateRight(int speed);
void stopRobot();

// Function to move the robot based on direction command
void moveRobot(String direction) {
  Serial.println("Command received: " + direction);
  if (direction == "forward") {
    moveForward(500);
  } else if (direction == "backward") {
    moveBackward(500);
  } else if (direction == "left") {
    strafeLeft(500);
  } else if (direction == "right") {
    strafeRight(500);
  } else if (direction == "rotateLeft") {
    rotateLeft(500);
  } else if (direction == "rotateRight") {
    rotateRight(500);
  } else if (direction == "stop") {
    stopRobot();
  } else {
    Serial.println("Unknown direction command.");
  }
}

// Handle HTTP GET requests for movement
void handleMove() {
  if (server.hasArg("direction")) {
    String direction = server.arg("direction");
    moveRobot(direction);
    server.send(200, "text/plain", "Moving: " + direction);
  } else {
    server.send(400, "text/plain", "Bad Request: Missing 'direction' parameter");
  }
}

// Serve the control webpage
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
      <title>Robot Control</title>
      <style>
          button { padding: 10px 20px; margin: 10px; font-size: 16px; }
      </style>
  </head>
  <body>
      <h1>Robot Control</h1>
      <img src="https://via.placeholder.com/320x240.png?text=No+Camera+Available" width="320" height="240">
      <div>
          <button onclick="sendCommand('forward')">Forward</button>
          <button onclick="sendCommand('backward')">Backward</button>
          <button onclick="sendCommand('left')">Left</button>
          <button onclick="sendCommand('right')">Right</button>
          <button onclick="sendCommand('rotateLeft')">Rotate Left</button>
          <button onclick="sendCommand('rotateRight')">Rotate Right</button>
          <button onclick="sendCommand('stop')">Stop</button>
      </div>
      <script>
          function sendCommand(direction) {
              fetch(`/move?direction=${direction}`)
                  .then(response => response.text())
                  .then(text => console.log(text));
          }
      </script>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200); // Consistent baud rate
  delay(1000);
  Serial.println("Mecanum Wheeled Robot Control Initialized");

  // Initialize Motor Pins
  initializeMotors();

  // Initialize PWM
  initializePWM();

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Set up HTTP routes
  server.on("/", handleRoot);
  server.on("/move", handleMove);

  // Start the web server
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  // Handle HTTP requests
  server.handleClient();
}

// ========================= Initialization Functions =========================
void initializeMotors() {
  // Set all motor pins as OUTPUT
  pinMode(MOTOR1_RPWM, OUTPUT);
  pinMode(MOTOR1_LPWM, OUTPUT);
  pinMode(MOTOR2_RPWM, OUTPUT);
  pinMode(MOTOR2_LPWM, OUTPUT);
  pinMode(MOTOR3_RPWM, OUTPUT);
  pinMode(MOTOR3_LPWM, OUTPUT);
  pinMode(MOTOR4_RPWM, OUTPUT);
  pinMode(MOTOR4_LPWM, OUTPUT);

  // Initialize motors to be stopped
  stopRobot();
}

void initializePWM() {
  // Configure PWM channels for all motor RPWM and LPWM pins
  
  // Motor1 - Reassigned to Channels 6 and 7
  ledcSetup(6, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR1_RPWM, 6);
  ledcSetup(7, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR1_LPWM, 7);

  // Motor2
  ledcSetup(2, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR2_RPWM, 2);
  ledcSetup(3, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR2_LPWM, 3);

  // Motor3 - Assigned to Channels 8 and 9
  ledcSetup(8, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR3_RPWM, 8);
  ledcSetup(9, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR3_LPWM, 9);

  // Motor4
  ledcSetup(4, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR4_RPWM, 4);
  ledcSetup(5, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR4_LPWM, 5);

  Serial.println("PWM Initialized");
}

// ========================= Movement Functions =========================
void moveForward(int speed) {
  // Ensure speed is within 0-1023
  speed = constrain(speed, 0, 1023);

  // Mecanum Forward: All wheels move forward
  ledcWrite(6, speed); // Motor1_RPWM
  ledcWrite(7, 0);     // Motor1_LPWM

  ledcWrite(2, speed); // Motor2_RPWM
  ledcWrite(3, 0);     // Motor2_LPWM

  ledcWrite(8, speed); // Motor3_RPWM
  ledcWrite(9, 0);     // Motor3_LPWM

  ledcWrite(4, speed); // Motor4_RPWM
  ledcWrite(5, 0);     // Motor4_LPWM

  Serial.println("Moving Forward");
}

void moveBackward(int speed) {
  // Ensure speed is within 0-1023
  speed = constrain(speed, 0, 1023);

  // Mecanum Backward: All wheels move backward
  ledcWrite(6, 0);     // Motor1_RPWM
  ledcWrite(7, speed); // Motor1_LPWM

  ledcWrite(2, 0);     // Motor2_RPWM
  ledcWrite(3, speed); // Motor2_LPWM

  ledcWrite(8, 0);     // Motor3_RPWM
  ledcWrite(9, speed); // Motor3_LPWM

  ledcWrite(4, 0);     // Motor4_RPWM
  ledcWrite(5, speed); // Motor4_LPWM

  Serial.println("Moving Backward");
}

void strafeLeft(int speed) {
  // Ensure speed is within 0-1023
  speed = constrain(speed, 0, 1023);

  // Mecanum Strafe Left
  ledcWrite(6, 0);     // Motor1_RPWM
  ledcWrite(7, speed); // Motor1_LPWM

  ledcWrite(2, speed); // Motor2_RPWM
  ledcWrite(3, 0);     // Motor2_LPWM

  ledcWrite(8, speed); // Motor3_RPWM
  ledcWrite(9, 0);     // Motor3_LPWM

  ledcWrite(4, 0);     // Motor4_RPWM
  ledcWrite(5, speed); // Motor4_LPWM

  Serial.println("Strafing Left");
}

void strafeRight(int speed) {
  // Ensure speed is within 0-1023
  speed = constrain(speed, 0, 1023);

  // Mecanum Strafe Right
  ledcWrite(6, speed); // Motor1_RPWM
  ledcWrite(7, 0);     // Motor1_LPWM

  ledcWrite(2, 0);     // Motor2_RPWM
  ledcWrite(3, speed); // Motor2_LPWM

  ledcWrite(8, 0);     // Motor3_RPWM
  ledcWrite(9, speed); // Motor3_LPWM

  ledcWrite(4, speed); // Motor4_RPWM
  ledcWrite(5, 0);     // Motor4_LPWM

  Serial.println("Strafing Right");
}

void rotateLeft(int speed) {
  // Ensure speed is within 0-1023
  speed = constrain(speed, 0, 1023);

  // Mecanum Rotate Left: Front wheels move backward, Rear wheels move forward
  ledcWrite(6, 0);     // Motor1_RPWM
  ledcWrite(7, speed); // Motor1_LPWM

  ledcWrite(2, speed); // Motor2_RPWM
  ledcWrite(3, 0);     // Motor2_LPWM

  ledcWrite(8, 0);     // Motor3_RPWM
  ledcWrite(9, speed); // Motor3_LPWM

  ledcWrite(4, speed); // Motor4_RPWM
  ledcWrite(5, 0);     // Motor4_LPWM

  Serial.println("Rotating Left");
}

void rotateRight(int speed) {
  // Ensure speed is within 0-1023
  speed = constrain(speed, 0, 1023);

  // Mecanum Rotate Right: Front wheels move forward, Rear wheels move backward
  ledcWrite(6, speed); // Motor1_RPWM
  ledcWrite(7, 0);     // Motor1_LPWM

  ledcWrite(2, 0);     // Motor2_RPWM
  ledcWrite(3, speed); // Motor2_LPWM

  ledcWrite(8, speed); // Motor3_RPWM
  ledcWrite(9, 0);     // Motor3_LPWM

  ledcWrite(4, 0);     // Motor4_RPWM
  ledcWrite(5, speed); // Motor4_LPWM

  Serial.println("Rotating Right");
}

void stopRobot() {
  // Stop all motors
  ledcWrite(6, 0);
  ledcWrite(7, 0);
  ledcWrite(2, 0);
  ledcWrite(3, 0);
  ledcWrite(8, 0);
  ledcWrite(9, 0);
  ledcWrite(4, 0);
  ledcWrite(5, 0);

  Serial.println("Stopping Robot");
}
