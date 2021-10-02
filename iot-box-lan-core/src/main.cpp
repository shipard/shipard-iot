#define SHP_SERIAL_DEBUG_ON
#define SHP_MQTT


#include <Arduino.h>

#include <FunctionalInterrupt.h>
#include <HTTPClient.h>

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Update.h>

#include <WebServer.h>
#include <DNSServer.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <esp32DHT.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <NewPing.h>

#include <Adafruit_NeoPixel.h>
#include <WS2812FX.h>

#include <Wire.h>

#include <bootstrap.h>





Application *app = new Application();

void setup()
{
  app->setup();
}

void loop()
{  
  app->loop();
}


#include <bootstrap.cpp>
