#include <SPI.h>
#include <WiFiNINA.h>
#include "audio_data.h"

const unsigned int sampleRate = 8000;  // Sample rate in Hz (e.g., 8 kHz)
volatile unsigned int sampleIndex = 0;

const int trigPin = 2;
const int echoPin = 3;
const int speakerPin = 9; // PWM-capable pin
volatile bool playAudio = false;

float duration, distance;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(speakerPin, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration*.0343)/2;
  Serial.print("Distance: ");
  Serial.println(distance);
  // delay(100);

  // Play a tone at 440 Hz (A4 note)
  if(distance < 60 && distance != 0) {
    // playAudioSamples();
    // Serial.println("Trying to play audio file.");
    tone(speakerPin, 440);
    Serial.println("Played tone");
    delay(1000); // Play for 1 second

    // Stop the tone
    noTone(speakerPin);
    delay(1000); // Wait for 1 second
  }
}

void playAudioSamples() {
  if (sampleIndex < sampleCount) {
    // Output the current audio sample as PWM
    tone(speakerPin, samples[sampleIndex] / 4); // Scale to 0-63 for PWM duty cycle
    delayMicroseconds(1000000 / sampleRate);         // Wait for the sample period
    sampleIndex++;
  } else {
    // Restart audio from the beginning
    sampleIndex = 0;
  }
}