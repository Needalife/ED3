#include <WiFi.h>
#include <WebServer.h>
#include <secrets.h>

WebServer server(80);

// Function to move the robot
void moveRobot(String direction) {
  Serial.println("Command received: " + direction);
  // Control logic here
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
  Serial.begin(9600);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    Serial.println(WiFi.status());
  }
  Serial.println("Connected to WiFi");
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
