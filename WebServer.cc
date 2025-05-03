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
const char* ssid = "SMTH_SINISTER_AFOOT";
const char* password = "peniscolada69420";

const char* PARAM_INPUT_1 = "state";

const int output1 = 27;
const int output2 = 14;
const int buttonPin = 18;

// Variables will change:
int mode = 0;          // the current state of the output1 pin
int buttonMode;             // the current reading from the input pin
int lastButtonMode = LOW;   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output1 pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output1 flickers

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
    buttons += "<p>Current Mode: <span id=\"outputState\">Loading...</span></p>";
    return buttons;
  }
  return String();
}


String outputState() {
  switch (mode) {
    case 0: return "OFF";
    case 1: return "LED ON";
    case 2: return "LED + UVB ON";
    default: return "UNKNOWN";
  }
}

void updateOutputs() {
  switch (mode) {
    case 0: // OFF
      digitalWrite(output1, LOW);
      digitalWrite(output2, LOW);
      break;
    case 1: // LED ON
      digitalWrite(output1, HIGH);
      digitalWrite(output2, LOW);
      break;
    case 2: // LED + UVB ON
      digitalWrite(output1, HIGH);
      digitalWrite(output2, HIGH);
      break;
  }
}


void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(output1, OUTPUT);
  digitalWrite(output1, LOW);
  pinMode(buttonPin, INPUT);
  
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


  // Send a GET request to <ESP_IP>/state
  server.on("/state", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", outputState());
  });

  // Start server
  server.begin();
}
  
void loop() {
  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);
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
