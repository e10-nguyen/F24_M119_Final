#include "arduino_secrets.h"
#include <SPI.h>
#include <WiFiNINA.h>
#include "audio_data.h"  // Include your audio data

const int trigPin = 2;         // GPIO Pins for Ultrasonic Sensor
const int echoPin = 3;
const int speakerPin = 9;      // PWM-capable pin for Audio Output

const unsigned int sampleRate = 8000;  // Sample rate in Hz
volatile unsigned int sampleIndex = 0;

float duration, distance;
int score = 0;                 // Score for trash deposited events
bool previousStateOver100 = true;  // Track if the previous state was over 100 cm

char ssid[] = SECRET_SSID;     // Your network SSID (name)
char pass[] = SECRET_PASS;     // Your network password (WPA)

int status = WL_IDLE_STATUS;
WiFiServer server(80);

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(speakerPin, OUTPUT);
  Serial.begin(9600);

  // Check for WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  // Compare Wi-Fi firmware to board
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // Attempt to connect to Wi-Fi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }
  server.begin();
  printWifiStatus();  // Serially prints to console when client connects
}

void loop() {
  WiFiClient client = server.available();

  // Measure distance using ultrasonic sensor
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.0343) / 2;  // Calculate distance in cm

  // Check if the distance changed from over 100 cm to less than 100 cm
  if (previousStateOver100 && distance < 100 && distance > 0) {
    score++;
    playAudioSamples();  // Play audio when trash is deposited
    previousStateOver100 = false;
  } else if (distance >= 100) {
    previousStateOver100 = true;
  }

  // Serve the web page or send data when a client is connected
  if (client) {
    Serial.println("New client connected");
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("/data") != -1) {
      // Send JSON data with distance and score
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"distance\":");
      client.print(distance, 2);
      client.print(",\"score\":");
      client.print(score);
      client.println("}");
    } else {
      // Serve the HTML page
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();

      client.println("<!DOCTYPE html><html><head>");
      client.println("<title>Distance Measurement</title>");
      client.println("<script>");
      client.println("function fetchData() {");
      client.println("  fetch('/data').then(response => response.json()).then(data => {");
      client.println("    document.getElementById('distance').innerHTML = 'Distance: ' + data.distance + ' cm';");
      client.println("    document.getElementById('score').innerHTML = 'Score: ' + data.score;");
      client.println("    if (data.distance < 100) {");
      client.println("      document.getElementById('message').innerHTML = 'Trash Deposited!';");
      client.println("    } else {");
      client.println("      document.getElementById('message').innerHTML = '';");
      client.println("    }");
      client.println("  });");
      client.println("}");
      client.println("setInterval(fetchData, 500);");  // Fetch data every 500ms
      client.println("</script>");
      client.println("</head><body>");
      client.println("<h2>Ultrasonic Distance Measurement</h2>");
      client.println("<div id='distance'>Distance: -- cm</div>");
      client.println("<div id='message'></div>");
      client.println("<div id='score'>Score: 0</div>");
      client.println("</body></html>");
    }

    client.stop();
    Serial.println("Client disconnected");
  }
}

void playAudioSamples() {
  for (sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
    analogWrite(speakerPin, samples[sampleIndex] / 4);  // Scale to 0-63 for PWM duty cycle
    delayMicroseconds(1000000 / sampleRate);            // Wait for the sample period
  }
  noTone(speakerPin);  // Stop tone after playback
}

// Serially print Wi-Fi status
void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");

  Serial.print("To view Distance Data, open a browser to http://");
  Serial.println(ip);
}
