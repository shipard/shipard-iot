#include <application.cpp>
#include <ioPort.cpp>
#include <utils/utility.cpp>
#ifdef SHP_NETWORK_LAN
#include <utils/httpRequest.cpp>
#include <utils/ota_update.cpp>
#endif

#ifdef SHP_NETWORK_LAN
#include <lib/telnet/ShpTelnet.cpp>
#endif

#include <data/dataSerial.cpp>
#include <data/dataOneWire.cpp>

#include <data/wiegand/Wiegand.cpp>
#include <data/dataWiegand.cpp>
#include <data/rfid/dataRFIDPN532.cpp>
#include <data/rfid/dataRFID125KHZ.cpp>
#include <data/rfid/MOD_RFID_1356_MIFARE.cpp>
#include <data/dataGSM.cpp>

#include <controls/controlBinary.cpp>
#include <controls/controlLedStrip.cpp>
#include <controls/controlLevel.cpp>
#include <controls/controlHBridge.cpp>

#include <inputs/inputAnalog.cpp>
#include <inputs/inputBinary.cpp>
#include <inputs/inputCounter.cpp>

#include <sensors/sensorDistanceUS.cpp>

#ifdef ESP32
#include <meteo/meteoDHT.cpp>
#endif
#include <meteo/meteoBME280.cpp>
#include <meteo/meteoBH1750.cpp>

#include <bus/busI2C.cpp>

#include <expanders/gpioExpander.cpp>
#include <expanders/gpioExpanderI2C.cpp>

#include <display/display.cpp>

#ifdef SHP_NETWORK_LAN
#include <display/displayNextion.cpp>
#endif

#ifdef SHP_NETWORK_LAN
#include <networks/espNow/espNow.cpp>
#include <networks/espNow/espNowServer.cpp>
#endif
#ifdef SHP_NETWORK_ESP_NOW
#include <networks/espNow/espNow.cpp>
#include <networks/espNow/espNowClient.cpp>
#endif

#ifdef SHP_WIFI
#include <lib/WiFiManager/WiFiManager.cpp>
#endif
