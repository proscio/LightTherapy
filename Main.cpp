#include <WiFi.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>

const char* ssid = "";
const char* password = "";

const int gpioPin1 = 26;
const int gpioPin2 = 27;
const int gpioPin3 = 14;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

const int freq = 5000;
const int resolution = 8;
int pwmValues[3] = {0, 0, 0};

Preferences preferences;

// Setup wifi, SPIFFS, PWM, and restore prev. PWM settings
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Initialization Failed!");
    return;
  }

  ledcSetup(0, freq, resolution);
  ledcSetup(1, freq, resolution);
  ledcSetup(2, freq, resolution);

  ledcAttachPin(gpioPin1, 0);
  ledcAttachPin(gpioPin2, 1);
  ledcAttachPin(gpioPin3, 2);

  preferences.begin("pwmValues", false);
  pwmValues[0] = preferences.getInt("pwm1", 0);
  pwmValues[1] = preferences.getInt("pwm2", 0);
  pwmValues[2] = preferences.getInt("pwm3", 0);
  preferences.end();

  ledcWrite(0, pwmValues[0]);
  ledcWrite(1, pwmValues[1]);
  ledcWrite(2, pwmValues[2]);

  server.on("/", HTTP_GET, handleRoot);
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("Server started");
}

//HTML from SPIFFS
void handleRoot() {
  File file = SPIFFS.open("/data.html", "r");
  if (!file) {
    server.send(500, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

//RTC Web Sockey
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String msg = (char*)payload;
    int pwmChannel = msg.substring(0, 1).toInt();
    int pwmValue = msg.substring(2).toInt();

    if (pwmChannel >= 0 && pwmChannel <= 2) {
      pwmValues[pwmChannel] = pwmValue;
      ledcWrite(pwmChannel, pwmValue);
      preferences.begin("pwmValues", false);
      preferences.putInt("pwm" + String(pwmChannel + 1), pwmValue);
      preferences.end();
      sendPWMValues(num);
    }
  }
}

// Send current PWM values to connected client
void sendPWMValues(uint8_t num) {
  String pwmMessage = String(pwmValues[0]) + "," + String(pwmValues[1]) + "," + String(pwmValues[2]);
  webSocket.sendTXT(num, pwmMessage);
}

//Launch Client requests and WebSocket
void loop() {
  server.handleClient();
  webSocket.loop();
}
