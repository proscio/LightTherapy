#include <WiFi.h>

// Replace with your network credentials
const char* ssid = "";
const char* password = "";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// GPIO assignments based on pinout
const int UVB_PWM = 12;
const int LED_PWM = 14;
const int FAN_PWM = 27;
const int MODE_BTN = 21;  // Mode selection button

// Mode tracking
volatile int mode = 0;  // 0: Off, 1: BOTH, 2: LED

// Interrupt Service Routine for mode button
void IRAM_ATTR changeMode() {
    mode = (mode + 1) % 3; // Cycle through modes
}

void setup() {
    Serial.begin(115200);
    
    // Initialize GPIO as outputs
    pinMode(UVB_PWM, OUTPUT);
    pinMode(LED_PWM, OUTPUT);
    pinMode(FAN_PWM, OUTPUT);
    
    // Initialize Mode button as input with internal pullup
    pinMode(MODE_BTN, INPUT_PULLUP);
    
    // Attach interrupt to mode button
    attachInterrupt(digitalPinToInterrupt(MODE_BTN), changeMode, FALLING);
    
    // Set outputs to LOW initially
    digitalWrite(UVB_PWM, LOW);
    digitalWrite(LED_PWM, LOW);
    digitalWrite(FAN_PWM, LOW);

    // Connect to Wi-Fi network
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
    // Update the system based on mode selection
    WiFiClient client = server.available();   // Listen for incoming clients

    if (client) {                             
        String currentLine = "";              
        while (client.connected()) {  
            if (client.available()) {             
                char c = client.read();             
                header += c;
                if (c == '\n') {                   
                    if (currentLine.length() == 0) {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();
                        
                        // Display the HTML web page
                        client.println("<!DOCTYPE html><html>");
                        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                        client.println("<link rel=\"icon\" href=\"data:,\">");
                        client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                        client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
                        client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
                        client.println(".button2 {background-color: #555555;}</style></head>");
                        
                        // Web Page Heading
                        client.println("<body><h1>ESP32 Web Server</h1>");
                        
                        // Display Current Mode
                        client.println("<h2>Current Mode:</h2>");
                        if (mode == 0) {
                            client.println("<h3>OFF</h3>");
                        } else if (mode == 1) {
                            client.println("<h3>ON</h3>");
                        } else if (mode == 2) {
                            client.println("<h3>LED ON</h3>");
                        }

                        // Display ON/OFF buttons for UVB (D12)
                        client.println("<p>UVB Control (GPIO 12) - Current State: " + String((mode == 1) ? "ON" : "OFF") + "</p>");
                        if (mode == 1) {
                            client.println("<p><a href=\"/mode/off\"><button class=\"button button2\">Turn UVB OFF</button></a></p>");
                        } else {
                            client.println("<p><a href=\"/mode/uvb\"><button class=\"button\">Turn UVB ON</button></a></p>");
                        }

                        // Display ON/OFF buttons for LED (D14)
                        client.println("<p>LED Control (GPIO 14) - Current State: " + String((mode == 2) ? "ON" : "OFF") + "</p>");
                        if (mode == 2) {
                            client.println("<p><a href=\"/mode/off\"><button class=\"button button2\">ON</button></a></p>");
                        } else {
                            client.println("<p><a href=\"/mode/led\"><button class=\"button\">Turn OFF</button></a></p>");
                        }

                        client.println("</body></html>");
                        
                        // The HTTP response ends with another blank line
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

        // Handle Mode Switching from Web
        if (header.indexOf("GET /mode/on") >= 0) {
            mode = 1;
        } else if (header.indexOf("GET /mode/led") >= 0) {
            mode = 2;
        } else if (header.indexOf("GET /mode/off") >= 0) {
            mode = 0;
        }

        // Clear the header variable
        header = "";

    switch (mode) {
        case 0: // Off
            digitalWrite(UVB_PWM, LOW);
            digitalWrite(LED_PWM, LOW);
            digitalWrite(FAN_PWM, LOW);
            Serial.println("Mode: OFF");
            break;
        case 1: // BOTH ON
            digitalWrite(UVB_PWM, HIGH);
            digitalWrite(LED_PWM, HIGH);
            digitalWrite(FAN_PWM, HIGH);
            delay(900000);
            Serial.println("Mode: ON");
            break;
        case 2: // LED Mode
            digitalWrite(UVB_PWM, LOW);
            digitalWrite(LED_PWM, HIGH);
            digitalWrite(FAN_PWM, HIGH);
            delay(900000);
            Serial.println("Mode: LED ON");
            break;
    }

    delay(500); // Debounce delay
}
