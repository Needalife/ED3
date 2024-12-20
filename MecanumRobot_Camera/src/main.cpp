#include <WiFi.h>
#include <esp_camera.h>
#include <ESPAsyncWebServer.h>

// Replace with your network credentials
const char* ssid = "NK (2)";
const char* password = "12345678";

// Create an AsyncWebServer object on port 80
AsyncWebServer server(80);

// Camera pin configuration
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27
#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

void startCamera() {
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
    if (psramFound()) {
        config.frame_size = FRAMESIZE_UXGA; // 1600x1200
        config.jpeg_quality = 10;          // Lower is higher quality
        config.fb_count = 2;               // Use two frame buffers
    } else {
        config.frame_size = FRAMESIZE_SVGA; // 800x600
        config.jpeg_quality = 12;          // Higher quality value
        config.fb_count = 1;               // Use one frame buffer
    }

    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        while (true); // Halt execution if the camera fails to initialize
    }
}

void setup() {
    // Start serial communication
    Serial.begin(115200);
    Serial.println("ESP32-CAM Camera Server");

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Start camera
    startCamera();

    // Route for video streaming
    server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", R"rawliteral(
            <!DOCTYPE html>
            <html>
            <body>
                <h1>ESP32-CAM Video Stream</h1>
                <img src="/stream.mjpeg" style="width:100%;">
            </body>
            </html>
        )rawliteral");
    });

    // Route for MJPEG stream
    server.on("/stream.mjpeg", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginChunkedResponse("multipart/x-mixed-replace; boundary=frame", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) {
                Serial.println("Camera capture failed");
                return 0;
            }
            size_t len = fb->len;
            memcpy(buffer, fb->buf, len);
            esp_camera_fb_return(fb);
            return len;
        });
        request->send(response);
    });

    // Start server
    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    // Nothing to do here
}
