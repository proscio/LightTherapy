#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <Arduino.h>

// Replace with your network credentials
const char* ssid = "";
const char* password = "";

const char* PARAM_INPUT_1 = "state";

const int WHITE_LED_PIN = 27;
const int UVB_LED_PIN = 14;
const int MODE_BUTTON_PIN = 18;
const int LUX_BUTTON_PIN = 19;

// PWM Config
const int PWM_FREQ = 34000;
const int PWM_RESOLUTION = 8;
const int MAX_DUTY = (1 << PWM_RESOLUTION) - 1;
const int PWM_CHANNEL_WHITE = 0;
const int PWM_CHANNEL_UVB = 1;

// Variables
int mode = 0;
int luxLevel = 0;
const int luxDutyCycle[] = {
  int(0.3 * MAX_DUTY),
  int(0.65 * MAX_DUTY),
  int(0.8 * MAX_DUTY)
};

unsigned long lastModePressTime = 0;
unsigned long lastLuxPressTime = 0;
const unsigned long debounceDelay = 50;

bool lastModeButtonState = 0;
bool lastLuxButtonState = 0;

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>ESP32 Lamp Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    /* CSS styles omitted for brevity – same as original */
  </style>
</head>
<body>
  <!-- HTML content omitted for brevity – same as original -->
  <script>
    function cycleMode() {
      fetch('/update');
    }

    function cycleLuxLevel() {
      fetch('/updateluxLevel');
    }

    setInterval(() => {
      fetch('/state')
        .then(res => res.text())
        .then(data => {
          document.getElementById("outputState").textContent = data;
        });
    }, 1000);
  </script>
</body>
</html>
)rawliteral";

// Setup PWM correctly
void setupPWM() {
  ledcSetup(PWM_CHANNEL_WHITE, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(WHITE_LED_PIN, PWM_CHANNEL_WHITE);

  ledcSetup(PWM_CHANNEL_UVB, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(UVB_LED_PIN, PWM_CHANNEL_UVB);
}

void updateLEDs() {
  int duty = luxDutyCycle[luxLevel];
  switch (mode) {
    case 0:
      ledcWrite(PWM_CHANNEL_WHITE, 0);
      ledcWrite(PWM_CHANNEL_UVB, 0);
      break;
    case 1:
      ledcWrite(PWM_CHANNEL_WHITE, duty);
      ledcWrite(PWM_CHANNEL_UVB, 0);
      break;
    case 2:
      ledcWrite(PWM_CHANNEL_WHITE, duty);
      ledcWrite(PWM_CHANNEL_UVB, duty);
      break;
  }
}

String outputState() {
  String state = "";
  switch (mode) {
    case 0: state += "Off"; break;
    case 1: state += "LED On"; break;
    case 2: state += "LED + UVB On"; break;
  }
  state += " | Lux Level: ";
  switch (luxLevel) {
    case 0: state += "Low"; break;
    case 1: state += "Medium"; break;
    case 2: state += "High"; break;
  }
  return state;
}

String processor(const String& var){
  return String();
}

void setup(){
  Serial.begin(115200);

  pinMode(MODE_BUTTON_PIN, INPUT);
  pinMode(LUX_BUTTON_PIN, INPUT);
  setupPWM();
  updateLEDs();
  Serial.println("Light Therapy Lamp Initialized.");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
    mode = (mode + 1) % 3;
    updateLEDs();
    Serial.print("Mode changed to: ");
    Serial.println(outputState());
    request->send(200, "text/plain", "OK");
  });

  server.on("/updateluxLevel", HTTP_GET, [](AsyncWebServerRequest* request) {
    luxLevel = (luxLevel + 1) % 3;
    updateLEDs();
    Serial.print("Lux Level changed to: ");
    Serial.println(luxLevel);
    request->send(200, "text/plain", "OK");
  });

  server.on("/state", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", outputState());
  });

  server.begin();
}

void loop() {
  unsigned long currentTime = millis();

  bool currentModeButton = digitalRead(MODE_BUTTON_PIN);
  if (currentModeButton && !lastModeButtonState &&
      (currentTime - lastModePressTime > debounceDelay)) {
    mode = (mode + 1) % 3;
    updateLEDs();
    Serial.print("Mode changed to: ");
    Serial.println(mode);
    lastModePressTime = currentTime;
  }
  lastModeButtonState = currentModeButton;

  bool currentLuxButton = digitalRead(LUX_BUTTON_PIN);
  if (currentLuxButton && !lastLuxButtonState &&
      (currentTime - lastLuxPressTime > debounceDelay)) {
    luxLevel = (luxLevel + 1) % 3;
    updateLEDs();
    Serial.print("Lux level changed to: ");
    Serial.println(luxLevel);
    lastLuxPressTime = currentTime;
  }
  lastLuxButtonState = currentLuxButton;

  delay(10);
}

