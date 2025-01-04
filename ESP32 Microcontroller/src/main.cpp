#include <WiFi.h>
#include <secrets.h>
#include "handleWebServer.h"

void setup() {
    Serial.begin(9600);

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
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
    server.handleClient();
}
