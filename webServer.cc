/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com  
*********/

// Load Wi-Fi library
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Replace with your network credentials
const char* ssid = "";
const char* password = "";

// Set web server port number to 80
AsyncWebServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String UVB_PWMState = "off";
String LED_PWMState = "off";

// Assign output variables to GPIO pins
const int UVB_PWM = 12;
const int LED_PWM = 14;
const int MODE_BTN = 21;

void setup() {
  Serial.begin(115200);
  
  // Initialize the output variables as outputs
  pinMode(UVB_PWM, OUTPUT);
  pinMode(LED_PWM, OUTPUT);
  
  // Set outputs to LOW
  digitalWrite(UVB_PWM, LOW);
  digitalWrite(LED_PWM, LOW);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Serve the HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><body><h1>ESP32 Web Server</h1>";
    html += "<p>UVB - State " + UVB_PWMState + "</p>";
    html += "<p>LED - State " + LED_PWMState + "</p>";
    
    // Buttons that will be used for AJAX requests
    html += "<button onclick=\"toggleUVB()\">Toggle UVB</button>";
    html += "<button onclick=\"toggleLED()\">Toggle LED</button>";
    
    // AJAX JavaScript
    html += "<script>";
    html += "function toggleUVB() {";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/toggleUVB', true);";
    html += "  xhr.send();";
    html += "  xhr.onload = function() {";
    html += "    if (xhr.status == 200) {";
    html += "      document.location.reload();";
    html += "    }";
    html += "  };";
    html += "}";
    
    html += "function toggleLED() {";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/toggleLED', true);";
    html += "  xhr.send();";
    html += "  xhr.onload = function() {";
    html += "    if (xhr.status == 200) {";
    html += "      document.location.reload();";
    html += "    }";
    html += "  };";
    html += "}";
    html += "</script>";
    html += "</body></html>";

    request->send(200, "text/html", html);
  });

  // Handle AJAX requests to toggle UVB
  server.on("/toggleUVB", HTTP_GET, [](AsyncWebServerRequest *request){
    if (UVB_PWMState == "off") {
      UVB_PWMState = "on";
      digitalWrite(UVB_PWM, HIGH);  // Turn UVB on
    } else {
      UVB_PWMState = "off";
      digitalWrite(UVB_PWM, LOW);  // Turn UVB off
    }
    request->send(200, "text/plain", "UVB Toggled");
  });

  // Handle AJAX requests to toggle LED
  server.on("/toggleLED", HTTP_GET, [](AsyncWebServerRequest *request){
    if (LED_PWMState == "off") {
      LED_PWMState = "on";
      digitalWrite(LED_PWM, HIGH);  // Turn LED on
    } else {
      LED_PWMState = "off";
      digitalWrite(LED_PWM, LOW);  // Turn LED off
    }
    request->send(200, "text/plain", "LED Toggled");
  });

  server.begin();
}

void loop() {
  // No need to do anything in the loop, as we are using the asynchronous server
}

