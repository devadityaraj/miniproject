#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseESP8266.h>
#include <TinyGPS++.h>  // For parsing GPS data
#include <SoftwareSerial.h>  // For GPS module communication
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>  // For time and date functions

// Define Wi-Fi credentials
struct WiFiCredentials {
  const char* ssid;
  const char* password;
};

// List of Wi-Fi networks
WiFiCredentials wifiNetworks[] = {
  {"Aditya-Desk", "12312345"},
  {"Galgotias-H", ""}  
};

// Firebase credentials
#define API_KEY "AIzaSyBZSz_JcqegYjLKPUncmV5Zzi8hLEnJ1Wg"
#define DATABASE_URL "https://miniproject-5e040-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "adityaraj94505@gmail.com"
#define USER_PASSWORD "aditya123"

SoftwareSerial gpsSerial(D2, D1);  // RX, TX for GPS

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

WiFiClientSecure client;
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 19800);
TinyGPSPlus gps;  

const int buttonPin = D5;  
const int vibratorPin = D6;
const int ledPin = LED_BUILTIN; 
const unsigned long doubleTapDelay = 800;  
unsigned long lastButtonPressTime = 0;
bool isFirstTap = false;
unsigned long buttonPressStartTime = 0;
bool longPressDetected = false;
bool gpsSendingEnabled = true;

#define OTA_PASSWORD "aditya@4net.in"

unsigned long lastGPSTime = 0; 
const unsigned long gpsUpdateInterval = 10000;

void handleOTA() {
  ArduinoOTA.handle();
}

void vibrate(unsigned long duration) {
  digitalWrite(vibratorPin, HIGH);
  delay(duration);
  digitalWrite(vibratorPin, LOW);
}

void connectToWiFi() {
  Serial.println("Starting Wi-Fi connection...");
  bool connected = false;

  for (int i = 0; i < sizeof(wifiNetworks) / sizeof(wifiNetworks[0]); i++) {
    WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password[0] ? wifiNetworks[i].password : NULL);
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(wifiNetworks[i].ssid);

    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
      Serial.print(".");
      vibrate(50);
      delay(1000); 
      timeout++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to Wi-Fi.");
      Serial.print("Connected to: ");
      Serial.println(wifiNetworks[i].ssid);
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      vibrate(350);
      connected = true;
      break;
    } else {
      Serial.println("\nFailed to connect to Wi-Fi.");
      Serial.print("SSID: ");
      Serial.println(wifiNetworks[i].ssid);
      Serial.print("Error Code: ");
      Serial.println(WiFi.status());
      vibrate(100);
      delay(150);
      vibrate(300);
      delay(500);
      vibrate(100);
      delay(150);
      vibrate(300);
    }
  }

  if (!connected) {
    Serial.println("Retrying Wi-Fi connection in 5 seconds...");
    delay(5000);
    vibrate(500);
    connectToWiFi(); 
  }
}

void authenticateFirebase() {
  Serial.println("Authenticating with Firebase...");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);

  client.setInsecure();

  if (Firebase.ready()) {
    Serial.println("Firebase authentication successful.");
    vibrate(100);
    delay(200);
    vibrate(500);
    delay(200);
  } else {
    Serial.println("Firebase authentication failed.");
    vibrate(800);
    delay(200);
    vibrate(800);
    delay(200);
    vibrate(800);
    delay(200);
    vibrate(800);
    delay(200);
  }
}

void syncTimeWithNTP() {
  Serial.println("Synchronizing time with NTP...");
  timeClient.update();

  unsigned long epochTime = timeClient.getEpochTime();

  setTime(epochTime);

  Serial.print("Synchronized time: ");
  Serial.print(year());
  Serial.print("-");
  Serial.print(month());
  Serial.print("-");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.println(second());
}

void sendAlertToFirebaseWithHistory() {
  Serial.println("Sending alert to Firebase with history...");

  syncTimeWithNTP();

  if (Firebase.ready()) {
    unsigned long timestamp = millis();
    String date = String(year()) + "-" + String(month()) + "-" + String(day()) + " " + String(hour()) + ":" + String(minute()) + ":" + String(second());

    if (Firebase.setInt(fbdo, "/alert", 1)) {
      Serial.println("Alert sent successfully to Firebase.");

      String historyPath = "/history/" + String(timestamp);
      Firebase.setString(fbdo, historyPath + "/date", date);
      Firebase.setDouble(fbdo, historyPath + "/latitude", gps.location.lat());
      Firebase.setDouble(fbdo, historyPath + "/longitude", gps.location.lng());
    } else {
      Serial.print("Failed to send alert. Error: ");
      Serial.println(fbdo.errorReason());
      vibrate(800);
    }
  } else {
    Serial.println("Firebase not ready for sending alert.");
    vibrate(800);
  }
}

void sendGPSToFirebase(float latitude, float longitude) {
  Serial.println("Sending GPS data to Firebase...");
  if (Firebase.ready()) {
    String gpsPath = "/gps";

    Serial.print("Sending latitude: ");
    Serial.println(latitude);
    if (Firebase.setDouble(fbdo, gpsPath + "/latitude", latitude)) {
      Serial.println("Latitude sent successfully.");
    } else {
      Serial.print("Failed to send latitude. Error: ");
      Serial.println(fbdo.errorReason());
    }

    Serial.print("Sending longitude: ");
    Serial.println(longitude);
    if (Firebase.setDouble(fbdo, gpsPath + "/longitude", longitude)) {
      Serial.println("Longitude sent successfully.");
    } else {
      Serial.print("Failed to send longitude. Error: ");
      Serial.println(fbdo.errorReason());
    }
  } else {
    Serial.println("Firebase not ready for GPS data.");
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting setup...");
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(vibratorPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(vibratorPin, LOW);
  digitalWrite(ledPin, HIGH);
  vibrate(900);

  connectToWiFi();
  authenticateFirebase();
  gpsSerial.begin(9600);
  
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();
  timeClient.begin();
  Serial.println("Setup complete.");
}

void loop() {
  handleOTA();

  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    if (gps.encode(c)) {
      if (gps.location.isValid()) {
        float latitude = gps.location.lat();
        float longitude = gps.location.lng();

        unsigned long currentTime = millis();
        if (currentTime - lastGPSTime >= gpsUpdateInterval) {
          lastGPSTime = currentTime;
          sendGPSToFirebase(latitude, longitude);
        }

        Serial.print("GPS data received. Latitude: ");
        Serial.print(latitude);
        Serial.print(", Longitude: ");
        Serial.println(longitude);
        sendGPSToFirebase(latitude, longitude);
      } else {
        Serial.println("Waiting for valid GPS data...");
      }
    } else {
      Serial.println("GPS encoding failed.");
    }
  }
       
  int reading = digitalRead(buttonPin);
  if (reading == LOW && buttonPressStartTime == 0) {
    Serial.println("Button pressed. Starting timer...");
    buttonPressStartTime = millis();
  }

  if (reading == HIGH && buttonPressStartTime > 0 && !longPressDetected) {
    unsigned long currentTime = millis();
    if (isFirstTap && (currentTime - lastButtonPressTime <= doubleTapDelay)) {
      Serial.println("Double tap detected! Sending alert with history.");
      sendAlertToFirebaseWithHistory();

      for (int i = 0; i < 3; i++) {
        vibrate(150);
        delay(150); 
      }

      isFirstTap = false;
      digitalWrite(ledPin, LOW);
    } else {
      Serial.println("Single tap detected. Waiting for a second tap...");
      isFirstTap = true;
      lastButtonPressTime = currentTime;
      digitalWrite(ledPin, LOW);
    }

    digitalWrite(ledPin, HIGH);
    buttonPressStartTime = 0;
  }

  delay(100);
}
