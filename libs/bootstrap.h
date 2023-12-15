
#include <application.h>
#include <utils/utility.h>
#ifdef SHP_NETWORK_LAN
#include <utils/httpRequest.h>
#include <utils/ota_update.h>
#endif

#ifdef SHP_NETWORK_LAN
#include <lib/telnet/ShpTelnet.h>
#endif

#ifdef SHP_NETWORK_CAN
#include <utils/ota_update_slow.h>
#endif

#include <bus/busI2C.h>
#include <expanders/gpioExpander.h>
#include <expanders/gpioExpanderI2C.h>
#include <expanders/gpioExpanderRS485.h>

#include <data/dataSerial.h>
#include <data/dataOneWire.h>
#include <bus/busRS485.h>
//#include <bus/busCAN.h>

#ifdef SHP_IOP_PN532
#include <data/rfid/dataRFIDPN532.h>
#endif

#include <data/rfid/dataRFID125KHZ.h>
#include <data/rfid/MOD_RFID_1356_MIFARE.h>
#include <data/dataWiegand.h>
#include <data/dataGSM.h>

#include <controls/controlBinary.h>
#include <controls/controlLedStrip.h>
#include <controls/controlLevel.h>
#include <controls/controlBistRelay.h>
#include <controls/controlHBridge.h>

#include <inputs/inputAnalog.h>
#include <inputs/inputCapBtn.h>
#include <inputs/inputBinary.h>
#include <inputs/inputCounter.h>

#include <sensors/sensorDistanceUS.h>
#include <sensors/LD2410.h>

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
#include <routers/OTAUpdateSlowSender.h>
#include <routers/can/routerCAN.h>
#endif

#ifdef SHP_NETWORK_LAN
#include <networks/espNow/espNow.h>
#include <networks/espNow/espNowServer.h>
#endif
#ifdef SHP_NETWORK_ESP_NOW
#include <networks/espNow/espNow.h>
#include <networks/espNow/espNowClient.h>
#endif

#ifdef SHP_WIFI_MANAGER
#include <lib/WiFiManager/WiFiManager.h>
#endif

#ifdef SHP_CAM_ESP32
#include <esp_camera.h>
#include <cams/camESP32.h>
#endif
