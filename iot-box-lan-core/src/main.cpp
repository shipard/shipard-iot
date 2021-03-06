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
#ifdef ESP8266
ESP8266HTTPClient.h
#endif

#include <PubSubClient.h>
#include <ArduinoJson.h>
#ifdef ESP32
#include <Preferences.h>
#include <WebServer.h>
#include <Update.h>
#endif

#ifdef ESP8266
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <Updater.h>
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

#include <bootstrap.h>
#include <applicationLan.h>


ApplicationLan *app = new ApplicationLan();

void setup()
{
  app->setup();
}

void loop()
{
  app->loop();
}


#include <bootstrap.cpp>
#include <applicationLan.cpp>
