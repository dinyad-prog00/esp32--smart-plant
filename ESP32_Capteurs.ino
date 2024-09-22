
#include <Arduino.h>

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// PH
#include "DFRobot_ESP_PH.h"
#include "EEPROM.h"
DFRobot_ESP_PH ph;
#define ESPADC 4096.0   // the esp Analog Digital Convertion value
#define ESPVOLTAGE 3300 // the esp voltage supply value
// #define PH_PIN 32    //the esp gpio data pin number
float voltage, phValue, temperature = 20;

// Lumiere
#include <Wire.h>
#include "DFRobot_VEML7700.h"
DFRobot_VEML7700 als;

// Temperature pression
#include "DFRobot_BMP388_I2C.h"
DFRobot_BMP388_I2C bmp388;

// Insert your network credentials
#define WIFI_SSID "Dinyad-Box-HUAWEI" // "Redmi 9C"
#define WIFI_PASSWORD "xxxxxxx"       //"xxxxxxxx"

// Insert Firebase project API Key
#define API_KEY "XXXXXXXXX"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long interval = 2000;
int count = 0;
bool signupOK = false;

/// Captors PIN
#define Humidity_PIN 33
#define PH_PIN 32
#define GAZ_VAL 26
#define GAZ_DET 27

void setup()
{

  wifiConnect();
  firebaseSetup();
  // Serial.begin(115200);
  Serial.begin(9600);
  pinMode(GAZ_VAL, INPUT);
  pinMode(GAZ_DET, INPUT);

  // PH
  EEPROM.begin(32); // needed to permit storage of calibration value in eeprom
  ph.begin();

  // Lumiere
  als.begin();

  // Temperature
  while (bmp388.begin() != 0)
  {
    Serial.println("BMP388 Error initializing. Trying again..");
    delay(1000);
  }
}

void loop()
{

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > interval || sendDataPrevMillis == 0))
  {

    sendDataPrevMillis = millis();
    // Push captors data in to the database

    float humidityValue = readHumidity();
    float phValue = readPH();
    float gaz = readGazVal();
    float isGaz = gazDetected();
    float lux = getLightRadiation();
    float temperature2 = bmp388.readTemperature();

    Serial.print("Himidity : ");
    Serial.print(humidityValue);
    Serial.print(", Light: ");
    Serial.print(lux);
    Serial.print(", Temperature: ");
    Serial.print(temperature2);
    Serial.print(", PH: ");
    Serial.print(phValue);
    // Serial.print(", GAZ: ");
    // Serial.print(gaz);
    Serial.print(", GAZ_DET: ");
    Serial.println(isGaz);

    char *names[] = {"humidity", "lightRadiation", "temperature", "gazDetected", "ph"};
    float values[] = {humidityValue, lux, temperature2, isGaz, phValue};
    int size = 5;
    pushData("/mesures-final/history", names, values, size);
  }
  
  ph.calibration(voltage, temperature);
}

void wifiConnect()
{

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void firebaseSetup()
{
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("ok");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

int pushData(char *path, char *names[], float values[], int size)
{

  FirebaseJson data;

  data.set("date/.sv", "timestamp");

  for (int i = 0; i < size; i++)
  {
    data.add(names[i], values[i]);
  }

  return Firebase.RTDB.pushJSON(&fbdo, path, &data);
}

float readHumidity()
{
  int value = analogRead(Humidity_PIN);
  return (float)value;
}

float readPH()
{
  temperature = bmp388.readTemperature();
  voltage = analogRead(PH_PIN) / ESPADC * ESPVOLTAGE;
  phValue = ph.readPH(voltage, temperature);

  return phValue;
}

float readGazVal()
{
  return analogRead(GAZ_VAL);
}

bool gazDetected()
{
  return digitalRead(GAZ_DET) == LOW;
}

float getLightRadiation()
{
  float lux;
  als.getALSLux(lux);
  return lux;
}
