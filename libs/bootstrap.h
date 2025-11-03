
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
#include <controls/controlMatrix.h>

#include <inputs/inputAnalog.h>
#include <inputs/inputCapBtn.h>
#include <inputs/inputBinary.h>
#include <inputs/inputCounter.h>

#include <sensors/sensorDistanceUS.h>

#ifdef SHP_SENSOR_LD2410
#include <sensors/LD2410.h>
#endif

#ifdef ESP32
#include <meteo/meteoDHT.h>
#endif
#include <meteo/meteoBME280.h>
#include <meteo/meteoBMP280.h>
#include <meteo/meteoBME68x.h>
#include <meteo/meteoSHT40.h>

#include <BH1750.h>
#include <meteo/meteoBH1750.h>

#ifdef SHP_POWER_BATT_CHARGER
#include <power/usb/usbBatteryCharger.h>
#endif

#include <display/display.h>
#ifdef SHP_NETWORK_LAN
#include <display/displayNextion.h>
#endif

#ifdef SHP_NETWORK_LAN
#include <routers/OTAUpdateSlowSender.h>
  #ifndef SHP_DISABLE_CAN_ROUTER
    #include <routers/can/routerCAN.h>
  #endif
#endif

#include <clients/uart/clientUART.h>


#ifdef SHP_NETWORK_LAN
#include <networks/espNow/espNow.h>
#include <networks/espNow/espNowServer.h>
#endif
#ifdef SHP_NETWORK_ESP_NOW
#include <networks/espNow/espNow.h>
#include <networks/espNow/espNowClient.h>
#endif


#ifdef SHP_WIFI
#include <esp_wifi.h>
#include <WiFi.h>
#include <networks/wifi/WiFiConnector.h>
#endif

#ifdef SHP_WIFI_MANAGER
#include <lib/WiFiManager/WiFiManager.h>
#endif

#ifdef SHP_CAM_ESP32
#include <esp_camera.h>
#include <cams/camESP32.h>
#endif
