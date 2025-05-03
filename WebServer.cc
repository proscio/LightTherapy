// Import required libraries
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

// Replace with your network credentials
const char* ssid = "SSID";
const char* password = "password";

const char* PARAM_INPUT_1 = "state";

const int WHITE_LED_PIN = 27;
const int UVB_LED_PIN = 14;
const int MODE_BUTTON_PIN = 18;
const int LUX_BUTTON_PIN = 18;

// PWM Config
//const int PWM_FREQ = 34000;
const int PWM_RESOLUTION = 8;
const int MAX_DUTY = (1 << PWM_RESOLUTION) - 1;
// const int PWM_CHANNEL_WHITE = 0;
// const int PWM_CHANNEL_UVB = 1;

// Variables will change:
int mode = 0;          // the current state of the WHITE_LED_PIN pin
int buttonMode;             // the current reading from the input pin
int lastButtonMode = LOW;   // the previous reading from the input pin

int luxLevel = 0;
const int luxDutyCycle[] = {
  int(0.3 * MAX_DUTY),
  int(0.65 * MAX_DUTY),
  int(0.8 * MAX_DUTY)
};

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the WHITE_LED_PIN pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the WHITE_LED_PIN flickers

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 2.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .button {
      background-color: #4CAF50;
      border: none;
      color: white;
      padding: 16px 40px;
      text-decoration: none;
      font-size: 20px;
      margin: 2px;
      cursor: pointer;
      border-radius: 8px;
    }
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>
  function cycleMode() {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/update", true);
    xhr.send();
  }

  function cycleluxLevel() {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/updateluxLevel", true);
    xhr.send();
  }

  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("outputState").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/state", true);
    xhttp.send();
  }, 1000);
</script>
</body>
</html>
)rawliteral";


// Replaces placeholder with button section in your web page
String processor(const String& var){
  if (var == "BUTTONPLACEHOLDER") {
    String buttons = "";
    buttons += "<p><button onclick=\"cycleMode()\" class=\"button\">Change Mode</button></p>";
    buttons += "<p><button onclick=\"cycleluxLevel()\" class=\"button\">Change Lux Level</button></p>";
    buttons += "<p>Current Mode: <span id=\"outputState\">Loading...</span></p>";
    return buttons;
  }
  return String();
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


void updateOutputs() {
  switch (mode) {
    case 0: // OFF
      digitalWrite(WHITE_LED_PIN, LOW);
      digitalWrite(UVB_LED_PIN, LOW);
      break;
    case 1: // LED ON
      digitalWrite(WHITE_LED_PIN, HIGH);
      digitalWrite(UVB_LED_PIN, LOW);
      break;
    case 2: // LED + UVB ON
      digitalWrite(WHITE_LED_PIN, HIGH);
      digitalWrite(UVB_LED_PIN, HIGH);
      break;
  }
}


void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(WHITE_LED_PIN, OUTPUT);
  digitalWrite(WHITE_LED_PIN, LOW);
  pinMode(MODE_BUTTON_PIN, INPUT);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
    mode = (mode + 1) % 3;  // Cycle through 0 → 1 → 2 → 0
    updateOutputs();
    Serial.print("Mode changed to: ");
    Serial.println(outputState());
    request->send(200, "text/plain", "OK");
  });

  server.on("/updateluxLevel", HTTP_GET, [](AsyncWebServerRequest* request) {
  luxLevel = (luxLevel + 1) % 3;
  Serial.print("Lux Level changed to: ");
  Serial.println(luxLevel);

  // Apply duty cycle if using PWM
  // ledcWrite(channel, dutyCycle[luxLevel]);

  request->send(200, "text/plain", "OK");
});



  // Send a GET request to <ESP_IP>/state
  server.on("/state", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", outputState());
  });

  // Start server
  server.begin();
}
  
void loop() {
  // read the state of the switch into a local variable:
  int reading = digitalRead(MODE_BUTTON_PIN);
  if (reading != lastButtonMode) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonMode) {
      buttonMode = reading;
      if (buttonMode == HIGH) {
        mode = (mode + 1) % 3;
        updateOutputs();
        Serial.print("Hardware button pressed. Mode: ");
        Serial.println(outputState());
      }
    }
  }
  lastButtonMode = reading;


  // save the reading. Next time through the loop, it'll be the lastButtonMode:
  lastButtonMode = reading;
}
