#include <Wire.h>
#include <MPU6050_tockn.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "secrets.h"

// =====================================================
// MPU6050
// =====================================================

MPU6050 mpu6050(Wire);

// =====================================================
// PINS
// =====================================================

#define BUZZER_PIN 4
#define BUTTON_PIN 8

// =====================================================
// SETTINGS
// =====================================================

float SEIZURE_THRESHOLD = 1.2;
unsigned long CONFIRM_TIME = 300;

// =====================================================
// VARIABLES
// =====================================================

bool seizureDetected = false;
bool alarmActive = false;
unsigned long seizureStartTime = 0;

WiFiClientSecure client;

// =====================================================
// TELEGRAM FUNCTION
// =====================================================

void sendTelegramMessage(String message)
{
    client.setInsecure();

    HTTPClient https;

    String url =
        "https://api.telegram.org/bot" +
        String(TELEGRAM_BOT_TOKEN) +
        "/sendMessage?chat_id=" +
        String(TELEGRAM_CHAT_ID) +
        "&text=" +
        message;

    Serial.println("Sending Telegram Alert...");

    if (https.begin(client, url))
    {
        int httpCode = https.GET();

        Serial.print("HTTP CODE: ");
        Serial.println(httpCode);

        String payload = https.getString();
        Serial.println(payload);

        https.end();
    }
    else
    {
        Serial.println("HTTPS Connection Failed");
    }
}

// =====================================================
// WIFI CONNECT
// =====================================================

void connectWiFi()
{
    Serial.println();
    Serial.println("Connecting to WiFi...");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);
    delay(2000);

    // ---------------- PINS ----------------
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(BUZZER_PIN, LOW);

    // ---------------- WIFI ----------------
    connectWiFi();

    // ---------------- MPU6050 ----------------
    Wire.begin(5, 6);

    Serial.println("Starting MPU6050...");
    mpu6050.begin();

    Serial.println("Keep MPU still for calibration...");
    mpu6050.calcGyroOffsets(true);

    Serial.println("MPU READY!");
    Serial.println("SYSTEM STARTED");
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
    mpu6050.update();

    float accX = mpu6050.getAccX();
    float accY = mpu6050.getAccY();
    float accZ = mpu6050.getAccZ();

    float magnitude = sqrt(
        (accX * accX) +
        (accY * accY) +
        (accZ * accZ)
    );

    Serial.print("Magnitude: ");
    Serial.println(magnitude);

    // =================================================
    // RESET BUTTON
    // =================================================

    if (digitalRead(BUTTON_PIN) == LOW)
    {
        Serial.println("RESET BUTTON PRESSED");

        alarmActive = false;
        seizureDetected = false;
        seizureStartTime = 0;

        digitalWrite(BUZZER_PIN, LOW);

        Serial.println("SYSTEM RESET COMPLETE");

        while (digitalRead(BUTTON_PIN) == LOW)
        {
            delay(10);
        }

        delay(500);
    }

    // =================================================
    // DETECTION LOGIC
    // =================================================

    if (magnitude > SEIZURE_THRESHOLD)
    {
        Serial.println("ABOVE THRESHOLD");

        if (!seizureDetected)
        {
            seizureDetected = true;
            seizureStartTime = millis();
            Serial.println("Monitoring violent shaking...");
        }

        if ((millis() - seizureStartTime) >= CONFIRM_TIME)
        {
            if (!alarmActive)
            {
                alarmActive = true;

                Serial.println("SEIZURE DETECTED!");
                Serial.println("ACTIVATING BUZZER");
                Serial.println("SENDING ALERT");

                digitalWrite(BUZZER_PIN, HIGH);

                sendTelegramMessage("SEIZURE_DETECTED");
            }
        }
    }
    else
    {
        if (!alarmActive)
        {
            seizureDetected = false;
        }
    }

    // =================================================
    // BUZZER CONTROL
    // =================================================

    if (alarmActive)
    {
        digitalWrite(BUZZER_PIN, HIGH);
    }
    else
    {
        digitalWrite(BUZZER_PIN, LOW);
    }

    delay(50);
}
