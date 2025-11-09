#define SHP_SERIAL_DEBUG_ON
//#define SHP_MQTT
#define SHP_INTERNET_MODE
#define SHP_NETWORK_LAN
//#define SHP_NETWORK_ESP_NOW
#define SHP_DISABLE_CAN_ROUTER
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
#include <Adafruit_BMP280.h>
#include <Adafruit_BME680.h>
#include <Adafruit_SHT4x.h>

#include <NewPing.h>

#include <Adafruit_NeoPixel.h>
#include <WS2812FX.h>

#include <Wire.h>

#define ARDUINOJSON_ENABLE_STD_STRING 0
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1

#include <ArduinoJson.h>
#include <bootstrap.h>
#include <applicationLan.h>


// ===================================
#ifdef GxEPD2_DRIVER_CLASS
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#endif



//#include <bootstrap.h>

#include <epd/epdDisplayDeviceCore.h>
#ifdef GxEPD2_DRIVER_CLASS
#include <epd/epdDisplayDeviceGxEPD2.h>
#endif
#include <epd/epdDisplayDevice133S6.h>
#include <epd/epdDisplay.h>


//#include <applicationLAN.h>


ApplicationLan *app = new ApplicationLan();

#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
//SPIClass hspi(HSPI);
#endif






void setup()
{
  app->setup();

  /*
  Serial.println ("power on....");
  pinMode(19, OUTPUT);
  digitalWrite(19, HIGH);
  delay(5000);
  */

//  delay(3000);

  //display.hibernate();
  //display.powerOff();

  //xSerial.println("\r\nInitialisation done.");
}

void loop()
{
  app->loop();
}


#include <bootstrap.cpp>
#include <epd/epdDisplay.cpp>
#include <epd/epdDisplayDeviceCore.cpp>
#ifdef GxEPD2_DRIVER_CLASS
#include <epd/epdDisplayDeviceGxEPD2.cpp>
#endif
#include <epd/epdDisplayDevice133S6.cpp>
#include <applicationLan.cpp>
