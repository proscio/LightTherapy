#include <WiFi.h>

// Replace with your network credentials (if needed)
const char* ssid = "";
const char* password = "";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state (for web page display)
String UVB_PWMState = "off";
String LED_PWMState = "off";

// Define GPIO pins
const int UVB_PWM = 12;
const int LED_PWM = 14;
const int FAN_PIN = 15;    // Fan output pin
const int MODE_BTN = 21;   // (Unused in this example)
const int BTN_1 = 23;      // Physical button to toggle the lamp

// --- PWM Setup Definitions ---
#define PWM_FREQ       5000    // Frequency in Hz
#define PWM_RESOLUTION 8       // 8-bit resolution (0-255)
#define UVB_CHANNEL    0
#define LED_CHANNEL    1
#define FAN_CHANNEL    2

// Global state to track the lamp (true = ON, false = OFF)
bool lampState = false;

// For physical button debouncing
bool btnPressed = false;

// Timing variables for the web server timeout
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  
  // --- Initialize PWM channels ---
  ledcSetup(UVB_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(UVB_PWM, UVB_CHANNEL);
  
  ledcSetup(LED_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED_PWM, LED_CHANNEL);
  
  ledcSetup(FAN_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(FAN_PIN, FAN_CHANNEL);
  
  // Set initial PWM duty cycles (off)
  ledcWrite(UVB_CHANNEL, 0);
  ledcWrite(LED_CHANNEL, 0);
  ledcWrite(FAN_CHANNEL, 0);

  // Set physical button as input with pull-up resistor.
  // (Assuming button connects the pin to GND when pressed.)
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
  // Since we use INPUT_PULLUP, a pressed button reads LOW.
  if (digitalRead(BTN_1) == LOW) {
    if (!btnPressed) {  // Trigger only once per press
      lampState = !lampState;  // Toggle lamp state
      if (lampState) {
        Serial.println("Physical Button: LAMP ON");
        UVB_PWMState = "on";
        LED_PWMState = "on";
        // Set full PWM duty cycle (255) for full brightness/power.
        ledcWrite(UVB_CHANNEL, 255);
        ledcWrite(LED_CHANNEL, 255);
        ledcWrite(FAN_CHANNEL, 255);
      } else {
        Serial.println("Physical Button: LAMP OFF");
        UVB_PWMState = "off";
        LED_PWMState = "off";
        ledcWrite(UVB_CHANNEL, 0);
        ledcWrite(LED_CHANNEL, 0);
        ledcWrite(FAN_CHANNEL, 0);
      }
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
            
            // Handle web requests
            if (header.indexOf("GET /21/on") >= 0) {
              Serial.println("Web Request: LAMP ON");
              lampState = true;
              UVB_PWMState = "on";
              LED_PWMState = "on";
              ledcWrite(UVB_CHANNEL, 255);
              ledcWrite(LED_CHANNEL, 255);
              ledcWrite(FAN_CHANNEL, 255);
            } else if (header.indexOf("GET /21/led") >= 0) {
              Serial.println("Web Request: UVB OFF");
              lampState = false;
              UVB_PWMState = "off";
              // Here we assume LED remains on even if UVB is off
              LED_PWMState = "on";
              ledcWrite(UVB_CHANNEL, 0);
              ledcWrite(LED_CHANNEL, 255);
              ledcWrite(FAN_CHANNEL, 0);
            } else if (header.indexOf("GET /21/off") >= 0) {
              Serial.println("Web Request: LAMP OFF");
              lampState = false;
              UVB_PWMState = "off";
              LED_PWMState = "off";
              ledcWrite(UVB_CHANNEL, 0);
              ledcWrite(LED_CHANNEL, 0);
              ledcWrite(FAN_CHANNEL, 0);
            }

            // Send HTML response to client
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
            client.println("<p>Fan - State " + (UVB_PWMState == "on" ? "ON" : "OFF") + "</p>");
            
            if (UVB_PWMState == "off" && LED_PWMState == "off") {
              client.println("<p><a href=\"/21/on\"><button class=\"button\">TURN LAMP ON</button></a></p>");
            } else if (UVB_PWMState == "on" && LED_PWMState == "on") {
              client.println("<p><a href=\"/21/led\"><button class=\"button button2\">TURN UVB OFF</button></a></p>");
            } else if (UVB_PWMState == "off" && LED_PWMState == "on") {
              client.println("<p><a href=\"/21/off\"><button class=\"button button2\">TURN LAMP OFF</button></a></p>");
            }
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
