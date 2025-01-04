#include <WiFi.h>
#include <esp_camera.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>  // Add this new include

// Replace with your network credentials
const char* ssid = "NK (2)";
const char* password = "12345678";

// Add authentication credentials
const char* wsUsername = "admin";
const char* wsPassword = "admin123";

// Create an AsyncWebServer object on port 80
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);  // WebSocket server on port 81
bool clientConnected = false;

// Camera pin configuration
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASH_GPIO_NUM     4

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
    config.xclk_freq_hz = 20000000;  // 20MHz for better stability
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_QVGA;  // Smaller frame size
    config.jpeg_quality = 15;  // Increased compression
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_LATEST;  // Changed to get latest frame

    // Initialize flash LED
    pinMode(FLASH_GPIO_NUM, OUTPUT);
    digitalWrite(FLASH_GPIO_NUM, LOW);

    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return;
    }

    Serial.println("Camera initialized successfully");

    // Optional: Set optimal camera parameters
    sensor_t * s = esp_camera_sensor_get();
    if (s) {
        s->set_brightness(s, 0);     // -2 to 2
        s->set_contrast(s, 0);       // -2 to 2
        s->set_saturation(s, 0);     // -2 to 2
        s->set_special_effect(s, 0); // 0 = No Effect
        s->set_whitebal(s, 1);       // 1 = Enable auto white balance
        s->set_awb_gain(s, 1);       // 1 = Enable auto white balance gain
        s->set_wb_mode(s, 0);        // 0 = Auto, 1 = Sun, 2 = Cloud, 3 = Indoors
        s->set_exposure_ctrl(s, 1);  // 1 = Enable auto exposure
        s->set_aec2(s, 0);          // 0 = Disable auto exposure gain
        s->set_gain_ctrl(s, 1);      // 1 = Enable auto gain control
        s->set_agc_gain(s, 0);      // 0 = No gain
        s->set_gainceiling(s, (gainceiling_t)0);  // 0 = 2x gain
        s->set_bpc(s, 0);           // 0 = Disable black pixel correction
        s->set_wpc(s, 1);           // 1 = Enable white pixel correction
        s->set_raw_gma(s, 1);       // 1 = Enable gamma correction
        s->set_lenc(s, 1);          // 1 = Enable lens correction
        s->set_hmirror(s, 0);       // 0 = Disable horizontal mirror
        s->set_vflip(s, 0);         // 0 = Disable vertical flip
        s->set_dcw(s, 1);           // 1 = Enable downsize EN
        s->set_framesize(s, FRAMESIZE_QVGA);
        s->set_quality(s, 15);  // Increased compression
    }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            clientConnected = false;
            break;
            
        case WStype_CONNECTED:
            {
                Serial.printf("[%u] Connection from: %s\n", num, webSocket.remoteIP(num).toString().c_str());
                clientConnected = false;  // Wait for auth
            }
            break;
            
        case WStype_TEXT:
            {
                String message = String((char*)payload);
                if (message.startsWith("AUTH:")) {
                    // Parse username:password
                    String auth = message.substring(5);
                    int separator = auth.indexOf(':');
                    if (separator > 0) {
                        String username = auth.substring(0, separator);
                        String password = auth.substring(separator + 1);
                        
                        if (username == wsUsername && password == wsPassword) {
                            clientConnected = true;
                            webSocket.sendTXT(num, "AUTH_OK");
                            Serial.printf("[%u] Authentication successful\n", num);
                        } else {
                            webSocket.sendTXT(num, "AUTH_FAILED");
                            Serial.printf("[%u] Authentication failed\n", num);
                        }
                    }
                }
            }
            break;
    }
}

void sendCameraFrame() {
    if (!clientConnected) return;

    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }

    webSocket.broadcastBIN(fb->buf, fb->len);
    esp_camera_fb_return(fb);
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

    // Additional WiFi settings for better stability
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.setSleep(false);

    // Start camera
    startCamera();

    // Add CORS headers for all routes
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

    // Route for basic status check
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "ESP32-CAM Server Running");
    });

    // Start WebSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    Serial.printf("WebSocket server started on port 81 (Username: %s, Password: %s)\n", wsUsername, wsPassword);
    
    server.begin();
    Serial.println("HTTP server started");
    Serial.println("WebSocket server started on port 81");
}

void loop() {
    webSocket.loop();
    if (clientConnected) {
        sendCameraFrame();
        delay(100);  // Limit frame rate
    }
}
