// Author: Cooper Hettinger
#include <WiFi.h>
#include <ArduinoJson.h>
#include "Arduino_LED_Matrix.h"
// Declaration for LED Matrix
ArduinoLEDMatrix matrix;

// WiFi credentials change these
const char* ssid = "ssid";
const char* password = "password";

// Server details (for incoming POST requests)
const int port = 8080; // Server will listen on port 8080

// Ultrasonic Sensor Pins
const int trigPin = 11;
const int echoPin = 12;

// LED Pin
const int ledPin = 13;

WiFiServer server(port);  // Create a server that listens on port 8080

// Variables for ultrasonic sensor
long duration;

void setup() {
  // Start serial communication
  Serial.begin(9600);
  matrix.begin();
  matrix.loadFrame(LEDMATRIX_CLOUD_WIFI);
  delay(100);

  // Initialize LED pin
  pinMode(ledPin, OUTPUT);

  // Initialize ultrasonic sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start the server to listen for incoming connections
  server.begin();
  Serial.println("Server started on port " + String(port));
}

void loop() {

  // Handle incoming client requests
  WiFiClient client = server.available();  // Listen for incoming client connections

  if (client) {
    Serial.println("New client connected.");

    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        // Check if the entire HTTP request has been received
        if (c == '\n' && request.endsWith("\r\n\r\n")) {
          break;
        }
      }
    }

    // Print the received request
    Serial.println("Received request:");
    Serial.println(request);

    // Check if the request is a POST request
    if (request.startsWith("POST")) {
      // Read the JSON body
      String contentLengthHeader = getHeaderValue(request, "Content-Length");
      int contentLength = contentLengthHeader.toInt();
      String body = "";

      if (contentLength > 0) {
        while (client.available()) {
          body += client.readString();
          if (body.length() >= contentLength) {
            break;
          }
        }
      }

      // Print the received JSON body
      Serial.println("Received POST body:");
      Serial.println(body);

      // Parse the received JSON
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, body);

      if (error) {
        Serial.println("Failed to parse JSON");
      } else {
        // Print the parsed data (just for debugging)
        long receivedDuration = doc["duration"];
        Serial.print("Received duration: ");
        Serial.println(receivedDuration);
        
        // Optionally, you can use the received data to control the LED
        if (receivedDuration > 30) { 
          matrix.loadFrame(LEDMATRIX_EMOJI_HAPPY);
        } else {
          matrix.loadFrame(LEDMATRIX_DANGER);
        }
      }

      // Send a response back to the client
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.println("{\"status\":\"success\"}");

    } else {
      // Send a 400 Bad Request response if it's not a POST request
      client.println("HTTP/1.1 400 Bad Request");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println("Invalid request method. Only POST is supported.");
    }

    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
  }

  // Measure distance and send data periodically
  duration = measureDistance();
  sendSensorDataToServer();
  delay(2500); // Delay between sending data
}

long measureDistance() {
  // Send pulse to trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Measure the pulse duration
  return pulseIn(echoPin, HIGH);
}

void sendSensorDataToServer() {
  // Create JSON data for the sensor
  StaticJsonDocument<200> doc;
  doc["duration"] = duration;  // Send the duration value from the sensor

  String jsonData;
  serializeJson(doc, jsonData);

  // Print the JSON data to Serial Monitor
  Serial.println("Sending sensor data to server:");
  Serial.println(jsonData);

  // Connect to the server and send the data
  WiFiClient client;
  if (client.connect("ip", 8000)) {  // Replace with your server's IP
    client.println("POST /data HTTP/1.1");
    client.println("Host: ip"); //Replace for verbosity
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(jsonData.length());
    client.println();
    client.println(jsonData);

    // Wait for server response
    while (client.available()) {
      String response = client.readString();
      Serial.print(response);
    }
    client.stop();  // Close the connection
    Serial.println("Data sent to server.");
  } else {
    Serial.println("Failed to connect to server.");
  }
}

// Helper function to extract a header value from the request
String getHeaderValue(String request, String header) {
  int headerStart = request.indexOf(header);
  if (headerStart == -1) {
    return "";
  }
  int valueStart = request.indexOf(":", headerStart) + 1;
  int valueEnd = request.indexOf("\r\n", valueStart);
  return request.substring(valueStart, valueEnd);
}
