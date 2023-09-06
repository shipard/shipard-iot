#define SHP_SERIAL_DEBUG_ON
#define SHP_MQTT
#define SHP_NETWORK_LAN
#define SHP_APP_CLASS ApplicationLan
#define DEBUG 1

#include <Arduino.h>

#include <FunctionalInterrupt.h>

#ifdef ESP32
#include <HTTPClient.h>
#endif

#include <PubSubClient.h>

#ifdef ESP32
#include <Preferences.h>
#include <WebServer.h>
#include <Update.h>
#endif

#include <DNSServer.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#ifdef ESP32
#include <esp32DHT.h>
#endif

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <NewPing.h>

#include <Adafruit_NeoPixel.h>
#include <WS2812FX.h>

#include <Wire.h>

#define ARDUINOJSON_ENABLE_STD_STRING 0
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1

#include <ArduinoJson.h>
#include <bootstrap.h>
#include <applicationLan.h>


ApplicationLan *app = new ApplicationLan();

void setup()
{
  Serial.println ("--main-setup--");
  app->setup();
}

void loop()
{
  app->loop();
}


#include <bootstrap.cpp>
#include <applicationLan.cpp>
