#include <WiFi.h>
#include <Arduino.h>

// Replace with your network credentials (if needed)
const char* ssid = "";
const char* password = "";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables for display purposes
String UVB_PWMState = "off";
String LED_PWMState = "off";

// Define GPIO pins
const int UVB_PWM = 12;
const int LED_PWM = 14;
const int FAN_PIN = 15;    // Fan output pin
const int MODE_BTN = 21;   // (Unused in this example)
const int BTN_1 = 23;      // Physical button pin
const int BTN_2 = 25;
// --- PWM Setup Definitions ---
#define PWM_FREQ       5000    // Frequency in Hz
#define PWM_RESOLUTION 8       // 8-bit resolution (0-255)

// Global lamp mode variable (0 = OFF, 1 = LAMP ON, 2 = UVB OFF/LED ON)
int lampMode = 0;

// For physical button debouncing
bool btnPressed = false;

// Timing variables for the web server timeout
unsigned long currentTime = 0;
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// Function to apply the current lamp mode using PWM
void applyLampMode() {
  if (lampMode == 0) {
    // All off
    Serial.println("Lamp Mode: OFF");
    UVB_PWMState = "off";
    LED_PWMState = "off";
    ledcWrite(UVB_PWM, 0);
    ledcWrite(LED_PWM, 0);
    ledcWrite(FAN_PIN, 0);
  } else if (lampMode == 1) {
    // Lamp on: UVB, LED, and Fan on
    Serial.println("Lamp Mode: ON (UVB and LED on, Fan on)");
    UVB_PWMState = "on";
    LED_PWMState = "on";
    ledcWrite(UVB_PWM, 255);
    ledcWrite(LED_PWM, 255);
    ledcWrite(FAN_PIN, 255);
  } else if (lampMode == 2) {
    // UVB off, LED on, Fan off
    Serial.println("Lamp Mode: UVB OFF / LED ON");
    UVB_PWMState = "off";
    LED_PWMState = "on";
    ledcWrite(UVB_PWM, 0);
    ledcWrite(LED_PWM, 255);
    ledcWrite(FAN_PIN, 0);
  }
}

void setup() {
  Serial.begin(115200);
  
  // --- Initialize PWM channels using new LEDC API ---
  ledcAttach(UVB_PWM, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(LED_PWM, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(FAN_PIN, PWM_FREQ, PWM_RESOLUTION);
  
  // Initialize outputs to OFF
  applyLampMode();
  
  // Set physical button as input with pull-up resistor.
  pinMode(BTN_1, INPUT_PULLUP);
  
  // --- Web Server Setup ---
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop() {
  // --- Physical Button Handling ---
  // With INPUT_PULLUP, the button reads LOW when pressed.
  if (digitalRead(BTN_1) == LOW) {
    if (!btnPressed) {  // Only trigger once per press
      // Cycle through modes: 0 -> 1 -> 2 -> 0 ...
      lampMode = (lampMode + 1) % 3;
      applyLampMode();
      btnPressed = true;
    }
  } else {
    btnPressed = false;
  }

  // --- Web Server Handling ---
  WiFiClient client = server.available();
  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    String currentLine = "";
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Handle web requests:
            if (header.indexOf("GET /21/on") >= 0) {
              lampMode = 1;
              Serial.println("Web Request: LAMP ON");
            }
            else if (header.indexOf("GET /21/led") >= 0) {
              lampMode = 2;
              Serial.println("Web Request: UVB OFF / LED ON");
            }
            else if (header.indexOf("GET /21/off") >= 0) {
              lampMode = 0;
              Serial.println("Web Request: LAMP OFF");
            }
            applyLampMode();

            // Send HTML response
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            client.println("<body><h1>ESP32 Web Server</h1>");
            client.println("<p>UVB - State " + UVB_PWMState + "</p>");
            client.println("<p>LED - State " + LED_PWMState + "</p>");
            client.println(String("<p>Fan - State ") + (UVB_PWMState == "on" ? "ON" : "OFF") + "</p>");
            client.println("<p><a href=\"/21/on\"><button class=\"button\">TURN LAMP ON</button></a></p>");
            client.println("<p><a href=\"/21/led\"><button class=\"button button2\">TURN UVB OFF / LED ON</button></a></p>");
            client.println("<p><a href=\"/21/off\"><button class=\"button button2\">TURN LAMP OFF</button></a></p>");
            client.println("</body></html>");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("");
  }
}

