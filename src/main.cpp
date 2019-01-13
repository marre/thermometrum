#include <Arduino.h>

#include <OneWire.h>
#include <DS18B20.h>

#include <ESP8266WiFi.h>

#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

#include "secrets.h"

// We want to read the voltage
ADC_MODE(ADC_VCC);

// The temp sensor is on D2 (Marked as D4 on D1 mini)
OneWire oneWire(2);
DS18B20 sensor(&oneWire);

void init_temp_sensor() {
  Serial.println("Init temperature sensor");
  sensor.begin();
  sensor.requestTemperatures();
}

void wait_for_temperature() {
  Serial.print("Wait for temperature to be ready");
  while (!sensor.isConversionComplete()) {
    Serial.print(".");
    delay(10);
  }
  Serial.println();
}

float read_temperature_celcius() {
  Serial.println("Reading temperature");
  return sensor.getTempC();
}

void connect_wifi() {
  // Connect WiFi
  Serial.printf("Connecting WiFi %s:%s\n", SSID, SSID_PASSWORD);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, SSID_PASSWORD);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
    Serial.print(".");
    // TODO: Don't do this forewer...
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void deep_sleep_5_minutes() {
  Serial.print("Sleeping 5 minutes");
  ESP.deepSleep(5 * 60 * 1000000, WAKE_RF_DEFAULT);  
}

void send_data_to_thingspeak(String ts_key, float temp_celcius, uint16_t vcc) {
  String temp_celcius_str = String(temp_celcius, 2);
  Serial.println("Temperature " + temp_celcius_str);

  // Build GET URL for posting data to thingspeak
  String url = String("http://api.thingspeak.com/update?api_key=") + ts_key + "&field1=" + temp_celcius_str + "&field2=" + vcc;
  Serial.println(url);

  // And finally send temperature to thingspeak
  HTTPClient http;
  if (http.begin(url)) {
    int httpCode = http.GET();
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
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

  connect_wifi();

  // Wait until sensor is ready
  wait_for_temperature();
  float temp_celcius = read_temperature_celcius();
  if (temp_celcius < 0.0) {
    // Some error reading temperature
    // TODO: Kind of useless to have a probe that cannot
    //       measure negative degrees...
    Serial.println("Failed to read temperature");
    return;
  }

  send_data_to_thingspeak(TS_KEY, temp_celcius, vcc);
}

void loop() {
  // Done... Time to sleep
  deep_sleep_5_minutes();
}
