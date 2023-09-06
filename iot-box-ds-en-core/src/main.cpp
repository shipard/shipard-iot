#define SHP_SERIAL_DEBUG_ON
#define SHP_NETWORK_ESP_NOW
#define SHP_APP_CLASS ApplicationDSEN
#define DEBUG 1

#include <Arduino.h>

#include <FunctionalInterrupt.h>


//#include <HTTPClient.h>

//#include <PubSubClient.h>
#include <ArduinoJson.h>

#ifdef ESP32
#include <Preferences.h>
#include <WebServer.h>
#include <Update.h>
#endif


//#include <WebServer.h>
//#include <DNSServer.h>

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
#include <applicationDSEN.h>


ApplicationDSEN *app = new ApplicationDSEN();

void setup()
{
  app->setup();
}

void loop()
{
  app->loop();
}


#include <bootstrap.cpp>
#include <applicationDSEN.cpp>
