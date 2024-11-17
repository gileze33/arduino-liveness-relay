#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"

// WiFi credentials
const char* ssid = SECRET_SSID;
const char* password = SECRET_WIFI_PASSWORD;

// Timing constants
const unsigned long BOOT_GRACE_PERIOD = 180000;  // 3 minutes in milliseconds
const unsigned long RETRY_INTERVAL = 15000; // 15 seconds in milliseconds
const unsigned long RELAY_OFF_DURATION = 2000; // 2 seconds in milliseconds

// Failure threshold
const int MAX_RETRIES = 5;

// Relay pin
const int RELAY_PIN = 15; // TODO - change me from LED pin

// Variables
int consecutiveFailures = 0;
unsigned long lastCheckTime = 0;
unsigned long lastBootAttemptTime = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(100);
  
  // Set up the relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Relay off initially (assumes active LOW relay)

  // Connect to Wi-Fi
  connectToWiFi();

  // Initial boot delay
  Serial.println("Setting last boot time to now");
  lastBootAttemptTime = millis();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Perform health check every RETRY_INTERVAL
  if (currentTime - lastCheckTime >= RETRY_INTERVAL) {
    lastCheckTime = currentTime;
    
    bool success = performHealthCheck();
    if (success) {
      Serial.println("Health check passed!");
      consecutiveFailures = 0; // Reset failures on success
    } else {
      Serial.println("Health check failed!");

      if (currentTime - lastCheckTime < BOOT_GRACE_PERIOD) {
        Serial.println("Boot grace period not reached yet, not treating as failure");
      }
      else {
        Serial.println("Incrementing failure count");
        consecutiveFailures++;
        
        if (consecutiveFailures > MAX_RETRIES) {
          triggerRelayRestart();
          consecutiveFailures = 0; // Reset failure counter after restart
          
          // Apply BOOT_GRACE_PERIOD after restart
          Serial.println("Setting last boot time to now");
          lastBootAttemptTime = millis();
        }
      }
    }
  }
}

bool performHealthCheck() {
  HTTPClient http;

  String url = "http://homeassistant.local:8123";
  http.begin(url);

  int httpCode = http.GET();
  http.end();

  return httpCode == HTTP_CODE_OK; // Return true if HTTP 200
}

void triggerRelayRestart() {
  Serial.println("Restarting via relay...");
  digitalWrite(RELAY_PIN, LOW); // Turn off relay
  delay(RELAY_OFF_DURATION);
  digitalWrite(RELAY_PIN, HIGH); // Turn relay back on
}

void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}