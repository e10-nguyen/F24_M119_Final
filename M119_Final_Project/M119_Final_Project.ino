#include "arduino_secrets.h"
#include <SPI.h>
#include <WiFiNINA.h>
#include "audio_data.h"

const int trigPin = 2;            // GPIO Pins for Ultrasonic Sensor
const int echoPin = 3;
const int speakerPin = 14;      // PWM-capable pin for Audio Output


const unsigned int sampleRate = 8000; // Sample rate in Hz for audio
volatile unsigned int sampleIndex = 0;

const int thres = 20; // threshold distance for trash can

float duration, distance;
int score = 0;                 // Score for trash deposited events
bool previousStateOverThres = true;  // Track if the previous state was over 100 cm
unsigned long under100StartTime = 0; // Track how long distance is < 100 cm
bool trashFullDisplayed = false;  // Track if "Trash Can is Full" was already displayed

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
  printWifiStatus();
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
  if (previousStateOverThres && distance < thres && distance > 0) {
    score++;
    playAudioSamples();  // Play audio when trash is deposited
    previousStateOverThres = false;
  } else if (distance >= thres) {
    previousStateOverThres = true;
    trashFullDisplayed = false;  // Reset the "Trash Can is Full" state
  }

  // Track if the distance is less than 100 cm for more than 5 seconds
  if (distance < thres && distance > 0) {
    if (under100StartTime == 0) {
      under100StartTime = millis();
    } else if (millis() - under100StartTime >= 5000 && !trashFullDisplayed) {
      trashFullDisplayed = true;
    }
  } else {
    under100StartTime = 0;  // Reset timer if distance goes above 100 cm
  }

  // Serve the web page or send data when a client is connected
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("/data") != -1) {
      // Send JSON data with distance, score, and trash full status
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"distance\":");
      client.print(distance, 2);
      client.print(",\"score\":");
      client.print(score);
      client.print(",\"trashFull\":");
      client.print(trashFullDisplayed ? "true" : "false");
      client.println("}");
    } else {
      // Serve the HTML page with graph visualization
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();

      client.println("<!DOCTYPE html><html><head>");
      client.println("<title>Distance Measurement</title>");
      client.println("<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>");
      client.println("<script>");
      client.println("let time = 0;");
      client.println("let scoreData = [];");
      client.println("function fetchData() {");
      client.println("  fetch('/data').then(response => response.json()).then(data => {");
      client.println("    document.getElementById('distance').innerHTML = 'Distance: ' + data.distance + ' cm';");
      client.println("    document.getElementById('score').innerHTML = 'Score: ' + data.score;");
// <<<<<<< HEAD
//       client.println("    if (data.distance < 100 && data.distance > 0) {");
//       client.println("      document.getElementById('message').innerHTML = 'Trash Deposited!';");
// =======
      client.println("    if (data.trashFull) {");
      client.println("      document.getElementById('message').innerHTML = 'Trash Can is Full';");
      client.println("    } else {");
      client.println("      document.getElementById('message').innerHTML = '';");
      client.println("    }");
      client.println("    time += 1;");
      client.println("    scoreData.push({x: time / 60, y: data.score});");
      client.println("    updateGraph();");
      client.println("  });");
      client.println("}");
      client.println("function updateGraph() {");
      client.println("  let ctx = document.getElementById('scoreChart').getContext('2d');");
      client.println("  if (window.myChart) window.myChart.destroy();");
      client.println("  window.myChart = new Chart(ctx, {");
      client.println("    type: 'line',");
      client.println("    data: {");
      client.println("      datasets: [{");
      client.println("        label: 'Trash Deposited Over Time',");
      client.println("        data: scoreData,");
      client.println("        borderColor: 'blue',");
      client.println("        fill: false");
      client.println("      }]");
      client.println("    },");
      client.println("    options: {");
      client.println("      scales: {");
      client.println("        x: { type: 'linear', position: 'bottom', title: {display: true, text: 'Time (Minutes)'} },");
      client.println("        y: { title: {display: true, text: 'Score'} }");
      client.println("      }");
      client.println("    }");
      client.println("  });");
      client.println("}");
      client.println("setInterval(fetchData, 1000);");
      client.println("</script>");
      client.println("</head><body>");
      client.println("<h2>Ultrasonic Distance Measurement</h2>");
      client.println("<div id='distance'>Distance: -- cm</div>");
      client.println("<div id='message'></div>");
      client.println("<div id='score'>Score: 0</div>");
      client.println("<canvas id='scoreChart' width='400' height='200'></canvas>");
      client.println("</body></html>");
    }

    client.stop();
  }
}

void playAudioSamples() {
  tone(speakerPin, 440);
  // for (sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
  //   analogWrite(speakerPin, samples[sampleIndex] / 4);  // Scale to 0-63 for PWM duty cycle
  //   delayMicroseconds(1000000 / sampleRate);            // Wait for the sample period
  // }
  delay(1000);
  noTone(speakerPin);  // Stop tone after playback
  delay(100);
}

void printWifiStatus() {
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}
