#include "esp_camera.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

// Wi-Fi Credentials
const char* ssid = "BinBem 5Ghz";
const char* password = "12022007";

// Camera Pins (AI Thinker Module)
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM        18
#define Y2_GPIO_NUM         5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

AsyncWebServer server(80);

// HTML Page for Video Stream
const char* stream_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-CAM Video Stream</title>
</head>
<body>
    <h1>ESP32-CAM Video Stream</h1>
    <img src="/stream" width="640" height="480" />
</body>
</html>
)rawliteral";

// Function to initialize the camera
bool initCamera(){
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    // Init with high specs to pre-allocate larger buffers
    if(psramFound()){
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK){
        Serial.printf("Camera init failed with error 0x%x", err);
        return false;
    }
    return true;
}

// Handle root page
void handleRoot(AsyncWebServerRequest *request){
    request->send_P(200, "text/html", stream_html);
}

// Handle stream
void handleStream(AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("multipart/x-mixed-replace; boundary=frame");
    request->send(response);
    while(1){
        camera_fb_t * fb = esp_camera_fb_get();
        if(!fb){
            Serial.println("Camera capture failed");
            return;
        }
        response->print("--frame\r\n");
        response->print("Content-Type: image/jpeg\r\n\r\n");
        response->write(fb->buf, fb->len);
        response->print("\r\n");
        esp_camera_fb_return(fb);

        // Small delay to control frame rate
        delay(100);
    }
}

void setup(){
    // Initialize Serial Monitor
    Serial.begin(115200);
    delay(1000);
    Serial.println("AI Thinker ESP32-CAM Video Stream");

    // Initialize Camera
    if(!initCamera()){
        Serial.println("Camera initialization failed!");
        while(1);
    }
    Serial.println("Camera initialized.");

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Start mDNS (Optional: For hostname access)
    if (!MDNS.begin("esp32cam")) {
        Serial.println("Error setting up MDNS responder!");
    }
    Serial.println("mDNS responder started. Access via esp32cam.local");

    // Initialize Web Server Routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/stream", HTTP_GET, handleStream);

    // Start Server
    server.begin();
}

void loop(){
    // Nothing needed here
}
