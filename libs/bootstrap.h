
#include <application.h>
#include <utils/utility.h>
#ifdef SHP_NETWORK_LAN
#include <utils/httpRequest.h>
#include <utils/ota_update.h>
#endif

#ifdef SHP_NETWORK_LAN
#include <lib/telnet/ShpTelnet.h>
#endif

#include <bus/busI2C.h>
#include <expanders/gpioExpander.h>
#include <expanders/gpioExpanderI2C.h>

#include <data/dataSerial.h>
#include <data/dataOneWire.h>
#include <data/rfid/dataRFIDPN532.h>
#include <data/rfid/dataRFID125KHZ.h>
#include <data/rfid/MOD_RFID_1356_MIFARE.h>
#include <data/dataWiegand.h>
#include <data/dataGSM.h>

#include <controls/controlBinary.h>
#include <controls/controlLedStrip.h>
#include <controls/controlLevel.h>
#include <controls/controlHBridge.h>

#include <inputs/inputAnalog.h>
#include <inputs/inputBinary.h>
#include <inputs/inputCounter.h>

#include <sensors/sensorDistanceUS.h>

#ifdef ESP32
#include <meteo/meteoDHT.h>
#endif
#include <meteo/meteoBME280.h>

#include <BH1750.h>
#include <meteo/meteoBH1750.h>

#include <display/display.h>
#ifdef SHP_NETWORK_LAN
#include <display/displayNextion.h>
#endif

#ifdef SHP_NETWORK_LAN
#include <networks/espNow/espNow.h>
#include <networks/espNow/espNowServer.h>
#endif
#ifdef SHP_NETWORK_ESP_NOW
#include <networks/espNow/espNow.h>
#include <networks/espNow/espNowClient.h>
#endif

#ifdef SHP_WIFI
#include <lib/WiFiManager/WiFiManager.h>
#endif

