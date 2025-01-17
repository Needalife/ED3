#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <secrets.h>  // Make sure this file defines WIFI_SSID and WIFI_PASSWORD

// ============================= Web Server =============================
WebServer server(80);

// ========================= Motor Driver Pin Definitions =========================
const int MOTOR1_RPWM = 26; // GPIO26
const int MOTOR1_LPWM = 25; // GPIO25

const int MOTOR2_RPWM = 15; // GPIO15
const int MOTOR2_LPWM = 4;  // GPIO4

const int MOTOR3_RPWM = 12; // GPIO12
const int MOTOR3_LPWM = 13; // GPIO13

const int MOTOR4_RPWM = 19; // GPIO19
const int MOTOR4_LPWM = 18; // GPIO18

// ========================= PWM Configuration =========================
const int PWM_FREQ       = 1000; // PWM frequency in Hz
const int PWM_RESOLUTION = 10;   // 10-bit resolution (0-1023)
const int PWM_CHANNELS   = 10;   // We'll use channels 0..9 as needed

// ========================= Data Structure for Motor PWM Tracking =========================
struct MotorPWM {
  String name;   // e.g., "Motor1_RPWM"
  int channel;   // channel number for ledcWrite
  int value;     // current PWM value (0–1023)
};

// Create an array describing each Motor pin → channel mapping & last-known value
MotorPWM motorPWMs[] = {
  {"Motor1_RPWM", 6, 0},
  {"Motor1_LPWM", 7, 0},
  {"Motor2_RPWM", 2, 0},
  {"Motor2_LPWM", 3, 0},
  {"Motor3_RPWM", 8, 0},
  {"Motor3_LPWM", 9, 0},
  {"Motor4_RPWM", 4, 0},
  {"Motor4_LPWM", 5, 0}
};

const int NUM_MOTORS = sizeof(motorPWMs) / sizeof(MotorPWM);

// ========================= Function Prototypes =========================
void initializeMotors();
void initializePWM();

// Basic Movements
void moveForward(int speed);
void moveBackward(int speed);
void strafeLeft(int speed);
void strafeRight(int speed);
void rotateLeft(int speed);
void rotateRight(int speed);

// Diagonal Movements
void moveDiagonalForwardLeft(int speed);
void moveDiagonalForwardRight(int speed);
void moveDiagonalBackwardLeft(int speed);
void moveDiagonalBackwardRight(int speed);

// Circular Motions
void moveCircular(int translationalSpeed, int rotationalSpeed);
void moveCircularWithRadius(int linearPWM, float radius);
void moveCircularCenterFacing(int linearPWM, float radius);

// Square Trajectory
void moveSquare(int durationSeconds);

// Robot Control
void stopRobot();
void moveRobot(String direction);

// HTTP Handlers
void handleRoot();
void handleMove();
void handleEmotion();
void handleSetPWM();
void handleGetPWM();

// ========================= SETUP =========================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Mecanum Wheeled Robot - Integrated Sketch");
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
  server.on("/",        handleRoot);
  server.on("/move",    handleMove);
  server.on("/emotion", handleEmotion);
  server.on("/set_pwm", handleSetPWM);
  server.on("/get_pwm", handleGetPWM);

  // Start the web server
  server.begin();
  Serial.println("Web server started");
}

// ========================= LOOP =========================
void loop() {
  // Handle incoming HTTP requests
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

  // Ensure motors start in stopped state
  stopRobot();
}

void initializePWM() {
  // Motor1 on channels 6 and 7
  ledcSetup(6, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR1_RPWM, 6);
  ledcSetup(7, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR1_LPWM, 7);

  // Motor2 on channels 2 and 3
  ledcSetup(2, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR2_RPWM, 2);
  ledcSetup(3, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR2_LPWM, 3);

  // Motor3 on channels 8 and 9
  ledcSetup(8, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR3_RPWM, 8);
  ledcSetup(9, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR3_LPWM, 9);

  // Motor4 on channels 4 and 5
  ledcSetup(4, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR4_RPWM, 4);
  ledcSetup(5, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR4_LPWM, 5);

  Serial.println("PWM Initialized");
}

// ========================= Movement & Control Functions =========================

void stopRobot() {
  // Stop all motors by writing 0 to all possible channels
  for (int i = 0; i < PWM_CHANNELS; i++) {
    ledcWrite(i, 0);
  }
  // Reset each tracked motor PWM to 0
  for (int i = 0; i < NUM_MOTORS; i++) {
    motorPWMs[i].value = 0;
  }
  Serial.println("Stopping Robot");
}

// --- Basic Mecanum Movements ---
void moveForward(int speed) {
  speed = constrain(speed, 0, 1023);

  // All wheels forward
  ledcWrite(6, speed); // M1_RPWM
  ledcWrite(7, 0);     
  ledcWrite(2, speed); // M2_RPWM
  ledcWrite(3, 0);
  ledcWrite(8, speed); // M3_RPWM
  ledcWrite(9, 0);
  ledcWrite(4, speed); // M4_RPWM
  ledcWrite(5, 0);

  Serial.println("Moving Forward");
}

void moveBackward(int speed) {
  speed = constrain(speed, 0, 1023);

  // All wheels backward
  ledcWrite(6, 0);
  ledcWrite(7, speed);
  ledcWrite(2, 0);
  ledcWrite(3, speed);
  ledcWrite(8, 0);
  ledcWrite(9, speed);
  ledcWrite(4, 0);
  ledcWrite(5, speed);

  Serial.println("Moving Backward");
}

void strafeLeft(int speed) {
  speed = constrain(speed, 0, 1023);

  // Mecanum Strafe Left
  ledcWrite(6, speed); // Motor1_RPWM
  ledcWrite(7, 0);     // Motor1_LPWM

  ledcWrite(2, speed); // Motor2_RPWM
  ledcWrite(3, 0);     // Motor2_LPWM

  ledcWrite(8, 0);     // Motor3_RPWM
  ledcWrite(9, speed); // Motor3_LPWM

  ledcWrite(4, 0);     // Motor4_RPWM
  ledcWrite(5, speed); // Motor4_LPWM

  Serial.println("Strafing Left");
}

void strafeRight(int speed) {
  speed = constrain(speed, 0, 1023);

  // Mecanum Strafe Right
  ledcWrite(6, 0);     // Motor1_RPWM
  ledcWrite(7, speed); // Motor1_LPWM

  ledcWrite(2, 0);     // Motor2_RPWM
  ledcWrite(3, speed); // Motor2_LPWM

  ledcWrite(8, speed); // Motor3_RPWM
  ledcWrite(9, 0);     // Motor3_LPWM

  ledcWrite(4, speed); // Motor4_RPWM
  ledcWrite(5, 0);     // Motor4_LPWM

  Serial.println("Strafing Right");
}

void rotateLeft(int speed) {
  speed = constrain(speed, 0, 1023);

  // Rotate Left
  ledcWrite(6, 0);
  ledcWrite(7, speed); 
  ledcWrite(2, speed);
  ledcWrite(3, 0);
  ledcWrite(8, speed);
  ledcWrite(9, 0);
  ledcWrite(4, 0);
  ledcWrite(5, speed);

  Serial.println("Rotating Left");
}

void rotateRight(int speed) {
  speed = constrain(speed, 0, 1023);

  // Rotate Right
  ledcWrite(6, speed);
  ledcWrite(7, 0);
  ledcWrite(2, 0);
  ledcWrite(3, speed);
  ledcWrite(8, 0);
  ledcWrite(9, speed);
  ledcWrite(4, speed);
  ledcWrite(5, 0);

  Serial.println("Rotating Right");
}

// --- Diagonal Movements ---
void moveDiagonalForwardLeft(int speed) {
  // Limit speed to half range so that 2*speed <= 1023
  speed = constrain(speed, 0, 511);

  // Forward-Left Diagonal Movement
  ledcWrite(6, speed); // M1_RPWM
  ledcWrite(7, 0);
  ledcWrite(2, speed); // M2_RPWM
  ledcWrite(3, 0);
  ledcWrite(8, 0);
  ledcWrite(9, 0);
  ledcWrite(4, 0);
  ledcWrite(5, 0);

  Serial.println("Diagonal Forward-Left");
}

void moveDiagonalForwardRight(int speed) {
  speed = constrain(speed, 0, 511);

  // Forward-Right Diagonal Movement
  ledcWrite(6, 0);
  ledcWrite(7, 0);
  ledcWrite(2, 0);
  ledcWrite(3, 0);
  ledcWrite(8, speed); // M3_RPWM
  ledcWrite(9, 0);
  ledcWrite(4, speed); // M4_RPWM
  ledcWrite(5, 0);

  Serial.println("Diagonal Forward-Right");
}

void moveDiagonalBackwardLeft(int speed) {
  speed = constrain(speed, 0, 511);

  // Backward-Left Diagonal Movement
  ledcWrite(6, 0);
  ledcWrite(7, speed); // M1_LPWM
  ledcWrite(2, speed); // M2_LPWM
  ledcWrite(3, 0);
  ledcWrite(8, 0);
  ledcWrite(9, 0);
  ledcWrite(4, 0);
  ledcWrite(5, 0);

  Serial.println("Diagonal Backward-Left");
}

void moveDiagonalBackwardRight(int speed) {
  speed = constrain(speed, 0, 511);

  // Backward-Right Diagonal Movement
  ledcWrite(6, 0);
  ledcWrite(7, speed); // M1_LPWM
  ledcWrite(2, 0);
  ledcWrite(3, speed); // M2_LPWM
  ledcWrite(8, 0);
  ledcWrite(9, speed); // M3_LPWM
  ledcWrite(4, 0);
  ledcWrite(5, speed); // M4_LPWM

  Serial.println("Diagonal Backward-Right");
}

// --- Circular Motions ---
void moveCircular(int translationalSpeed, int rotationalSpeed) {
  // Each motor = (translational ± rotational)
  translationalSpeed = constrain(translationalSpeed, -1023, 1023);
  rotationalSpeed    = constrain(rotationalSpeed,    -1023, 1023);

  // Calculate raw speeds
  int motor1 = translationalSpeed + rotationalSpeed; // M1 (Rear Left)
  int motor2 = translationalSpeed - rotationalSpeed; // M2 (Front Right)
  int motor3 = translationalSpeed - rotationalSpeed; // M3 (Rear Right)
  int motor4 = translationalSpeed + rotationalSpeed; // M4 (Front Left)

  // Constrain
  motor1 = constrain(motor1, -1023, 1023);
  motor2 = constrain(motor2, -1023, 1023);
  motor3 = constrain(motor3, -1023, 1023);
  motor4 = constrain(motor4, -1023, 1023);

  // Rear Left (M1)
  if (motor1 >= 0) {
    ledcWrite(6, motor1);
    ledcWrite(7, 0);
  } else {
    ledcWrite(6, 0);
    ledcWrite(7, abs(motor1));
  }

  // Front Right (M2)
  if (motor2 >= 0) {
    ledcWrite(2, motor2);
    ledcWrite(3, 0);
  } else {
    ledcWrite(2, 0);
    ledcWrite(3, abs(motor2));
  }

  // Rear Right (M3)
  if (motor3 >= 0) {
    ledcWrite(8, motor3);
    ledcWrite(9, 0);
  } else {
    ledcWrite(8, 0);
    ledcWrite(9, abs(motor3));
  }

  // Front Left (M4)
  if (motor4 >= 0) {
    ledcWrite(4, motor4);
    ledcWrite(5, 0);
  } else {
    ledcWrite(4, 0);
    ledcWrite(5, abs(motor4));
  }

  Serial.println("Moving in (Arc) Circular Trajectory");
}

void moveCircularWithRadius(int linearPWM, float radius) {
  // linearPWM within ±1023
  linearPWM = constrain(linearPWM, -1023, 1023);

  // Example scale factor for computing rotation from radius
  float radiusScale = 100.0; 
  float rotational  = (float)linearPWM * (radiusScale / radius);
  int   rotPWM      = constrain((int)rotational, -1023, 1023);

  // Move using combined circular control
  moveCircular(linearPWM, rotPWM);

  Serial.print("Circular w/ Radius => linear:");
  Serial.print(linearPWM);
  Serial.print(", radius:");
  Serial.println(radius);
}

void moveCircularCenterFacing(int linearPWM, float radius) {
  // Constrain linear
  linearPWM = constrain(linearPWM, -1023, 1023);

  // w = v / R  (plus optional scale factor)
  float scaleFactor = 10.0;
  float rawOmega    = float(linearPWM) / radius;
  float scaledOmega = rawOmega * scaleFactor;
  int rotationalPWM = constrain((int)scaledOmega, -1023, 1023);

  // Move
  moveCircular(linearPWM, rotationalPWM);

  Serial.print("Center-Facing Circle => v:");
  Serial.print(linearPWM);
  Serial.print(", R:");
  Serial.print(radius);
  Serial.print(", wPWM:");
  Serial.println(rotationalPWM);
}

// --- Square Trajectory ---
void moveSquare(int durationSeconds) {
  if (durationSeconds <= 0) {
    Serial.println("Invalid duration for Square Trajectory.");
    return;
  }

  Serial.println("Starting Square Trajectory...");

  // Move Forward
  moveForward(500); // Adjust speed as needed
  delay(durationSeconds * 1000);
  stopRobot();
  delay(500); // Brief pause between movements

  // Strafe Right
  strafeRight(500);
  delay(durationSeconds * 1000);
  stopRobot();
  delay(500);

  // Move Backward
  moveBackward(500);
  delay(durationSeconds * 1000);
  stopRobot();
  delay(500);

  // Strafe Left
  strafeLeft(500);
  delay(durationSeconds * 1000);
  stopRobot();
  delay(500);

  Serial.println("Completed Square Trajectory");
}

// ========================= Robot Movement Handling =========================
void moveRobot(String direction) {
  direction.replace("\"", ""); // Clean up any quotes
  Serial.println("Command received: " + direction);

  if      (direction == "forward")              moveForward(500);
  else if (direction == "backward")             moveBackward(500);
  else if (direction == "left")                 strafeLeft(500);
  else if (direction == "right")                strafeRight(500);
  else if (direction == "rotateLeft")           rotateLeft(500);
  else if (direction == "rotateRight")          rotateRight(500);
  else if (direction == "stop")                 stopRobot();
  else if (direction == "circular")             moveCircular(500, 200);  // example
  else if (direction == "diagonalForwardLeft")  moveDiagonalForwardLeft(300);
  else if (direction == "diagonalForwardRight") moveDiagonalForwardRight(300);
  else if (direction == "diagonalBackwardLeft") moveDiagonalBackwardLeft(300);
  else if (direction == "diagonalBackwardRight")moveDiagonalBackwardRight(300);
  else {
    Serial.println("Unknown direction command.");
  }
}

// ========================= HTTP Request Handlers =========================

// 1) The main control UI
void handleRoot() {
  // This HTML includes:
  // - D-Pad with diagonals
  // - Rotate, Circular, Center-Facing, etc.
  // - A placeholder for IP camera / WebSocket stream
  // - Section to set individual motor PWMs
  // - Section to see current PWM
  // - Simple "emotion" toggle to demonstrate expression-based control
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
      <title>Robot Control Panel</title>
      <style>
          body {
              font-family: Arial, sans-serif;
              display: flex;
              flex-direction: column;
              align-items: center;
              padding: 20px;
              background-color: #f0f0f0;
              margin: 0;
          }
          h1 { margin-bottom: 20px; color: #333; }
          img {
              margin-bottom: 20px;
              border: 2px solid #ccc;
              border-radius: 5px;
          }
          .dpad-container {
              display: grid;
              grid-template-areas:
                  "diagFLeft forward diagFRight"
                  "left stop right"
                  "diagBLeft backward diagBRight";
              gap: 10px;
              margin-bottom: 30px;
              position: relative;
          }
          .dpad-container button {
              width: 80px;
              height: 60px;
              font-size: 14px;
              cursor: pointer;
              border: none;
              border-radius: 5px;
              background-color: #007BFF;
              color: white;
              transition: background-color 0.3s;
          }
          .dpad-container button:hover {
              background-color: #0056b3;
          }
          .forward   { grid-area: forward; }
          .backward  { grid-area: backward; }
          .left      { grid-area: left; }
          .right     { grid-area: right; }
          .stop      {
              grid-area: stop;
              background-color: #DC3545;
          }
          .stop:hover {
              background-color: #c82333;
          }
          .diagFLeft  { grid-area: diagFLeft; }
          .diagFRight { grid-area: diagFRight; }
          .diagBLeft  { grid-area: diagBLeft; }
          .diagBRight { grid-area: diagBRight; }

          .action-buttons {
              display: flex;
              flex-wrap: wrap;
              gap: 10px;
              justify-content: center;
              margin-bottom: 30px;
          }
          .action-buttons button {
              padding: 10px 15px;
              font-size: 14px;
              cursor: pointer;
              border: none;
              border-radius: 5px;
              background-color: #6C757D;
              color: white;
              transition: background-color 0.3s;
          }
          .action-buttons button:hover {
              background-color: #5A6268;
          }

          .slider-section {
              width: 100%;
              max-width: 500px;
              background-color: #fff;
              padding: 15px 20px;
              border-radius: 5px;
              box-shadow: 0 0 10px rgba(0,0,0,0.1);
              margin-bottom: 30px;
          }
          .slider-section h2 {
              margin-bottom: 15px;
              font-size: 18px;
              text-align: center;
              color: #333;
          }
          .slider-block {
              margin: 15px 0;
          }
          .slider-block label {
              display: flex;
              justify-content: space-between;
              margin-bottom: 5px;
              font-weight: bold;
              color: #555;
          }
          input[type="range"] { width: 100%; }
          .control-button {
              width: 100%;
              padding: 10px;
              font-size: 16px;
              cursor: pointer;
              background-color: #17A2B8;
              color: white;
              border: none;
              border-radius: 5px;
              transition: background-color 0.3s;
              margin-top: 10px;
          }
          .control-button:hover {
              background-color: #138496;
          }

          /* Basic stream/camera layout */
          #status {
              margin: 10px 0;
              padding: 5px;
              text-align: center;
              border-radius: 5px;
              width: 300px;
          }
          #stream {
              border: 1px solid #ccc;
              margin-bottom: 20px;
          }
          .login-form {
              margin-bottom: 10px;
          }
          .login-form input {
              padding: 5px;
              width: 180px;
          }
          .login-form button {
              padding: 5px 10px;
              margin-left: 5px;
          }

          @media (max-width: 600px) {
              .dpad-container button {
                  width: 60px;
                  height: 50px;
                  font-size: 12px;
              }
              .action-buttons button {
                  font-size: 12px;
                  padding: 8px 10px;
              }
              .control-button { font-size: 14px; }
          }
      </style>
  </head>
  <body>
      <h1>Robot Control Panel</h1>
      
      <!-- Stream / Camera Placeholder -->
      <div class="login-form">
          <input type="text" id="ipAddress" placeholder="ESP32-CAM IP Address" value="192.168.1.100">
          <button onclick="connect()">Connect</button>
          <button onclick="disconnect()">Disconnect</button>
      </div>
      <div id="status"></div>
      <canvas id="stream"></canvas>

      <!-- D-Pad with Diagonal Buttons -->
      <div class="dpad-container">
          <button class="diagFLeft" onclick="sendCommand('diagonalForwardLeft')" title="Diagonal Forward-Left">↖️</button>
          <button class="forward"   onclick="sendCommand('forward')">Forward</button>
          <button class="diagFRight"onclick="sendCommand('diagonalForwardRight')" title="Diagonal Forward-Right">↗️</button>
          
          <button class="left" onclick="sendCommand('left')">Left</button>
          <button class="stop" onclick="sendCommand('stop')">Stop</button>
          <button class="right" onclick="sendCommand('right')">Right</button>
          
          <button class="diagBLeft" onclick="sendCommand('diagonalBackwardLeft')" title="Diagonal Backward-Left">↙️</button>
          <button class="backward"   onclick="sendCommand('backward')">Backward</button>
          <button class="diagBRight" onclick="sendCommand('diagonalBackwardRight')" title="Diagonal Backward-Right">↘️</button>
      </div>

      <!-- Additional Action Buttons -->
      <div class="action-buttons">
          <button onclick="sendCommand('rotateLeft')">Rotate Left</button>
          <button onclick="sendCommand('rotateRight')">Rotate Right</button>
          <button onclick="sendCommand('circular')">Circular Motion</button>
          <button onclick="sendCommand('circularRadius')">Circular with Radius</button>
          <button onclick="sendCommand('circularCenterFacing')">Center-Facing Circular</button>
          <button onclick="toggleMode()">Toggle Emotion Mode</button>
      </div>

      <hr style="width: 100%; max-width: 500px; margin-bottom: 30px;">

      <!-- ===================== Square Trajectory Controls ===================== -->
      <div class="slider-section">
          <h2>Square Trajectory</h2>
          <div class="slider-block">
              <label for="squareDuration">Duration per side (seconds):
                  <span id="squareDurationVal">2</span>
              </label>
              <input type="range" min="1" max="10" value="2"
                     id="squareDuration" oninput="updateSquareDurationLabel(this.value)">
          </div>
          <button class="control-button" onclick="sendSquare()">Execute Square Trajectory</button>
      </div>

      <hr style="width: 100%; max-width: 500px; margin-bottom: 30px;">

      <!-- Circular with Radius Controls -->
      <div class="slider-section">
          <h2>Circular with Radius</h2>
          <div class="slider-block">
              <label for="linearSpeed">Linear PWM (0–1023):
                  <span id="linearVal">400</span>
              </label>
              <input type="range" min="0" max="1023" value="400"
                     id="linearSpeed" oninput="updateLinearLabel(this.value)">
          </div>
          <div class="slider-block">
              <label for="radiusVal">Radius (units):
                  <span id="radiusDisplay">50</span>
              </label>
              <input type="range" min="5" max="200" value="50"
                     id="radiusVal" oninput="updateRadiusLabel(this.value)">
          </div>
          <button class="control-button" onclick="sendCircularRadius()">Execute Circular Motion</button>
      </div>

      <!-- Center-Facing Circular Controls -->
      <div class="slider-section">
          <h2>Center-Facing Circular</h2>
          <div class="slider-block">
              <label for="linearSpeedCF">Tangential PWM (0–1023):
                  <span id="linearValCF">400</span>
              </label>
              <input type="range" min="0" max="1023" value="400"
                     id="linearSpeedCF" oninput="updateLinearLabelCF(this.value)">
          </div>
          <div class="slider-block">
              <label for="radiusValCF">Radius (units):
                  <span id="radiusDisplayCF">50</span>
              </label>
              <input type="range" min="5" max="200" value="50"
                     id="radiusValCF" oninput="updateRadiusLabelCF(this.value)">
          </div>
          <button class="control-button" onclick="sendCircularCenterFacing()">Execute Center-Facing Circular</button>
      </div>

      <!-- Individual Motor PWM Controls -->
      <div class="slider-section">
          <h2>Adjust Motor PWM</h2>
          <label for="motorSelect">Select Motor:</label>
          <select id="motorSelect">
              <option value="Motor1_RPWM">Motor1 Right</option>
              <option value="Motor1_LPWM">Motor1 Left</option>
              <option value="Motor2_RPWM">Motor2 Right</option>
              <option value="Motor2_LPWM">Motor2 Left</option>
              <option value="Motor3_RPWM">Motor3 Right</option>
              <option value="Motor3_LPWM">Motor3 Left</option>
              <option value="Motor4_RPWM">Motor4 Right</option>
              <option value="Motor4_LPWM">Motor4 Left</option>
          </select>
          <br><br>
          <label for="pwmSlider">PWM Value: <span id="pwmValue">0</span></label>
          <input type="range" id="pwmSlider" min="0" max="1023" value="0" oninput="updatePWMValue(this.value)">
          <br><br>
          <button class="control-button" onclick="setPWM()">Set PWM</button>
      </div>

      <div class="slider-section">
          <h2>Current PWM Values</h2>
          <ul id="currentPWM">
              <!-- Filled dynamically via /get_pwm -->
          </ul>
      </div>

      <script>
        let ws;
        const canvas = document.getElementById('stream');
        const ctx = canvas.getContext('2d');
        const statusDiv = document.getElementById('status');

        // Mode toggling between 'manual' and 'emotion'
        function toggleMode() {
          if(localStorage.getItem('mode') === 'emotion') {
            localStorage.setItem('mode', 'manual');
            alert("Switched to Manual Mode");
          } else {
            localStorage.setItem('mode', 'emotion');
            alert("Switched to Emotion Mode");
          }
        }

        // Connect to a WebSocket for camera feed (demo)
        function connect() {
            const ip = document.getElementById('ipAddress').value;
            if (ws) { ws.close(); }

            statusDiv.style.background = '#fff3cd';
            statusDiv.textContent = 'Connecting...';

            try {
                // Example: 'ws://192.168.1.100:8765' or wherever your camera server is
                ws = new WebSocket(`ws://${ip}:8765`);

                ws.onopen = function() {
                    statusDiv.style.background = '#d4edda';
                    statusDiv.textContent = 'Connected!';
                };

                ws.onmessage = function(msg) {
                    // Create image from base64 data
                    const img = new Image();
                    img.onload = function() {
                        canvas.width = img.width;
                        canvas.height = img.height;
                        ctx.drawImage(img, 0, 0);
                    };
                    img.src = 'data:image/jpeg;base64,' + msg.data;
                };

                ws.onclose = function() {
                    statusDiv.style.background = '#f8d7da';
                    statusDiv.textContent = 'Disconnected';
                };

                ws.onerror = function(err) {
                    statusDiv.style.background = '#f8d7da';
                    statusDiv.textContent = 'Connection error: ' + err.message;
                };

            } catch (error) {
                statusDiv.style.background = '#f8d7da';
                statusDiv.textContent = 'Error: ' + error.message;
            }
        }

        function disconnect() {
            if (ws) {
                ws.close();
                statusDiv.style.background = '#f8d7da';
                statusDiv.textContent = 'Disconnected';
            }
        }

        // Basic Move Commands
        function sendCommand(direction) {
          // If we are in emotion mode, send to /emotion else /move
          if(localStorage.getItem('mode') === 'emotion') {
            fetch(`/emotion`, {
              method: 'POST',
              headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
              body: `plain=${direction}`
            }).then(res => res.text()).then(console.log);
          } else {
            fetch(`/move?direction=${direction}`)
              .then(res => res.text())
              .then(console.log);
          }
        }

        // Circular with Radius
        function sendCircularRadius() {
          const linear = document.getElementById('linearSpeed').value;
          const radius = document.getElementById('radiusVal').value;
          fetch(`/move?direction=circularRadius&linear=${linear}&radius=${radius}`)
            .then(res => res.text())
            .then(txt => console.log(txt))
            .catch(err => console.error(err));
        }

        // Center-Facing Circular
        function sendCircularCenterFacing() {
          const linear = document.getElementById('linearSpeedCF').value;
          const radius = document.getElementById('radiusValCF').value;
          fetch(`/move?direction=circularCenterFacing&linear=${linear}&radius=${radius}`)
            .then(res => res.text())
            .then(txt => console.log(txt))
            .catch(err => console.error(err));
        }

        // Update Labels for Circular with Radius
        function updateLinearLabel(val) {
          document.getElementById('linearVal').innerText = val;
        }
        function updateRadiusLabel(val) {
          document.getElementById('radiusDisplay').innerText = val;
        }

        // Update Labels for Center-Facing Circular
        function updateLinearLabelCF(val) {
          document.getElementById('linearValCF').innerText = val;
        }
        function updateRadiusLabelCF(val) {
          document.getElementById('radiusDisplayCF').innerText = val;
        }

        // Individual Motor PWM
        function updatePWMValue(value) {
          document.getElementById('pwmValue').innerText = value;
        }

        function setPWM() {
          const motor = document.getElementById('motorSelect').value;
          const pwm   = document.getElementById('pwmSlider').value;
          fetch(`/set_pwm?motor=${motor}&pwm=${pwm}`)
            .then(response => response.text())
            .then(text => {
                console.log(text);
                updateCurrentPWM();
            });
        }

        function updateCurrentPWM() {
            fetch('/get_pwm')
              .then(response => response.json())
              .then(data => {
                const pwmList = document.getElementById('currentPWM');
                pwmList.innerHTML = '';
                for (const motor in data) {
                  const li = document.createElement('li');
                  li.textContent = `${motor}: ${data[motor]}`;
                  pwmList.appendChild(li);
                }
              });
        }

        // ===================== Square Trajectory JavaScript Functions =====================

        // Update the duration label as the slider moves
        function updateSquareDurationLabel(val) {
          document.getElementById('squareDurationVal').innerText = val;
        }

        // Send the square trajectory command to the server
        function sendSquare() {
          const duration = document.getElementById('squareDuration').value;
          fetch(`/move?direction=square&duration=${duration}`)
            .then(res => res.text())
            .then(text => {
                console.log(text);
                alert(text); // Optional: Notify the user
            })
            .catch(err => console.error(err));
        }

        // ===================== End of JavaScript =====================

        // Refresh PWM values periodically
        setInterval(updateCurrentPWM, 5000);
        window.onload = () => {
          // Default to manual mode if not set
          if(!localStorage.getItem('mode')) {
            localStorage.setItem('mode', 'manual');
          }
          updateCurrentPWM();
        };
      </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// 2) Handle standard movement commands (/move?direction=...)
void handleMove() {
  if (server.hasArg("direction")) {
    String direction = server.arg("direction");

    if (direction == "circularRadius") {
      // Existing circularRadius handling
      if (server.hasArg("linear") && server.hasArg("radius")) {
        int   linear = server.arg("linear").toInt();
        float radius = server.arg("radius").toFloat();
        moveCircularWithRadius(linear, radius);

        String resp = "Moving in Circular Path with Linear=" + String(linear) +
                      " and Radius=" + String(radius);
        server.send(200, "text/plain", resp);
      } else {
        server.send(400, "text/plain", "Missing 'linear' or 'radius' parameter");
      }
    }
    else if (direction == "circularCenterFacing") {
      // Existing circularCenterFacing handling
      if (server.hasArg("linear") && server.hasArg("radius")) {
        int   linear = server.arg("linear").toInt();
        float radius = server.arg("radius").toFloat();
        moveCircularCenterFacing(linear, radius);

        String resp = "Center-Facing Circular with Linear=" + String(linear) +
                      " and Radius=" + String(radius);
        server.send(200, "text/plain", resp);
      } else {
        server.send(400, "text/plain", "Missing 'linear' or 'radius' parameter");
      }
    }
    else if (direction == "square") {
      // Handle Square Trajectory
      if (server.hasArg("duration")) {
        int duration = server.arg("duration").toInt();
        if (duration <= 0) {
          server.send(400, "text/plain", "Invalid 'duration' parameter");
          return;
        }
        moveSquare(duration);
        String resp = "Executing Square Trajectory for " + String(duration) + " seconds per side.";
        server.send(200, "text/plain", resp);
      } else {
        server.send(400, "text/plain", "Missing 'duration' parameter");
      }
    }
    else {
      // Normal directions (forward, backward, diagonal, rotate, etc.)
      moveRobot(direction);
      server.send(200, "text/plain", "Moving: " + direction);
    }
  } else {
    server.send(400, "text/plain", "Bad Request: Missing 'direction' parameter");
  }
}

// 3) Handle "emotion" route (demo for facial expression control)
void handleEmotion() {
  // Example: we pass the direction in the POST body under 'plain'
  if (server.hasArg("plain")) {
    String direction = server.arg("plain");
    moveRobot(direction);
    server.send(200, "text/plain", "Moving (emotion-based): " + direction);
  } else {
    server.send(400, "text/plain", "Bad Request: Missing 'direction' parameter");
  }
}

// 4) Set individual PWM for a specified motor (e.g., /set_pwm?motor=Motor1_RPWM&pwm=500)
void handleSetPWM() {
  if (server.hasArg("motor") && server.hasArg("pwm")) {
    String motor   = server.arg("motor");
    int    pwmVal  = server.arg("pwm").toInt();

    pwmVal = constrain(pwmVal, 0, 1023);

    bool found = false;
    for (int i = 0; i < NUM_MOTORS; i++) {
      if (motorPWMs[i].name == motor) {
        int ch = motorPWMs[i].channel;
        ledcWrite(ch, pwmVal);
        motorPWMs[i].value = pwmVal; // update stored value
        found = true;
        break;
      }
    }
    if (!found) {
      server.send(400, "text/plain", "Bad Request: Unknown motor name");
      return;
    }

    String response = "Set " + motor + " PWM to " + String(pwmVal);
    server.send(200, "text/plain", response);
    Serial.println(response);
  } else {
    server.send(400, "text/plain", "Missing 'motor' or 'pwm' parameter");
  }
}

// 5) Get current PWM for all motors in JSON form (/get_pwm)
void handleGetPWM() {
  DynamicJsonDocument doc(1024);
  for (int i = 0; i < NUM_MOTORS; i++) {
    doc[motorPWMs[i].name] = motorPWMs[i].value;
  }
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}
