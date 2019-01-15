#include <Arduino.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <ESP8266WiFi.h>

#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

#include "secrets.h"

// We want to read the voltage
ADC_MODE(ADC_VCC);

// The temp sensor is on D2 (Marked as D4 on D1 mini)
OneWire oneWire(2);
DallasTemperature sensors(&oneWire);

void init_temp_sensor() {
  Serial.println("Init temperature sensor");
  sensors.setWaitForConversion(false);
  sensors.begin();
  sensors.requestTemperatures();
}

bool wait_for_temperature() {
  Serial.print("Wait for temperature to be ready");
  for (int i=0; i < 200; i++) {
    if (sensors.isConversionComplete()) {
      // Temperature available!
      return true;
    }
    Serial.print(".");
    delay(10);
  }

  // No temp available
  return false;
}

float read_temperature_celcius() {
  // Wait until sensor is ready
  if (!wait_for_temperature()) {
    Serial.println("Timed out waiting for temperature");
    return DEVICE_DISCONNECTED_C;
  }
  
  Serial.println("Reading temperature");
  return sensors.getTempCByIndex(0);
}

uint8_t connect_wifi() {
  // Connect WiFi
  Serial.println("Connecting WiFi");
  Serial.println(SSID);
  Serial.println(SSID_PASSWORD);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, SSID_PASSWORD);

  Serial.print("Wait for connection");
  uint8_t connect_result = WiFi.waitForConnectResult();
  if (connect_result != WL_CONNECTED) {
    // Connect failed
    return connect_result;
  }

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  return WiFi.status();
}

void deep_sleep_5_minutes() {
  Serial.print("Sleeping 5 minutes");
  ESP.deepSleep(5 * 60 * 1000000, WAKE_RF_DEFAULT);  
}

void send_data_to_thingspeak(String ts_key, float temp_celcius, uint16_t vcc) {
  String temp_celcius_str = String(temp_celcius, 2);
  Serial.print("Temperature ");
  Serial.println(temp_celcius_str);

  // Build GET URL for posting data to thingspeak
  String url = String("http://api.thingspeak.com/update?api_key=") + ts_key + "&field1=" + temp_celcius_str + "&field2=" + vcc;
  Serial.println(url);

  // And finally send temperature to thingspeak
  HTTPClient http;
  if (http.begin(url)) {
    int httpCode = http.GET();
    Serial.print("HTTP status code ");
    Serial.println(httpCode);
    http.end();
  }
}


void setup() {
  Serial.begin(9600);
  Serial.println();

  uint16_t vcc = ESP.getVcc();
  Serial.print("Voltage ");
  Serial.println(vcc);

  // First init temp sensor, as it may take a while to read temp
  // and we can do that while we are waiting for WiFi to connect
  init_temp_sensor();

  uint8_t connect_result = connect_wifi();
  if (connect_result != WL_CONNECTED) {
    Serial.print("Connect failed : ");
    Serial.println(connect_result);
    return;
  }

  float temp_celcius = read_temperature_celcius();
  if (temp_celcius <= DEVICE_DISCONNECTED_C) {
    // Some error reading temperature
    Serial.println("Failed to read temperature");
    return;
  }
  Serial.print("Temperature ");
  Serial.println(temp_celcius);

  send_data_to_thingspeak(TS_KEY, temp_celcius, vcc);
}

void loop() {
  // Done... Time to sleep
  deep_sleep_5_minutes();
}
