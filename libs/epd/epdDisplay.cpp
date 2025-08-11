extern SHP_APP_CLASS *app;

#define SHP_GxEPD2_420c               1
#define SHP_GxEPD2_750c               2
#define SHP_GxEPD2_1248c              3
#define SHP_GxEPD2_1248               4
#define SHP_GxEPD2_750c_Z08           5
#define SHP_GxEPD2_266c_GDEY0266F51H  6
#define SHP_GxEPD2_583_GDEQ0583T31    7
#define SHP_GxEPD2_730c_GDEY073D46    8
#define SHP_GxEPD2_730c_GDEP073E01    9


#define SHP_BF_COUNT_BITS             6
#define SHP_BF_COUNT_MASK             0b00111111

#define BUSY 4
#define RST  16
#define DC   17
#define CS   5

#ifdef SHP_EPD_BOARD_M25
#define BUSY 15
#define RST  16
#define DC   14
#define CS   10
#endif


#if !defined(GxEPD2_DRIVER_CLASS)
  #error "ERROR! symbol GxEPD2_DRIVER_CLASS is not defined.";
#endif

#define epdColorMap {GxEPD_BLACK,GxEPD_WHITE,GxEPD_RED,GxEPD_YELLOW,GxEPD_BLUE,GxEPD_GREEN,GxEPD_ORANGE}


#if GxEPD2_DRIVER_CLASS == SHP_GxEPD2_420c
  GxEPD2_3C<GxEPD2_420c, GxEPD2_420c::HEIGHT> display(GxEPD2_420c(CS, DC, RST, BUSY));
#elif GxEPD2_DRIVER_CLASS == SHP_GxEPD2_750c
  GxEPD2_3C<GxEPD2_750c, GxEPD2_750c::HEIGHT> display(GxEPD2_750c(CS, DC, RST, BUSY));
#elif GxEPD2_DRIVER_CLASS == SHP_GxEPD2_750c_Z08
  GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT / 2> display(GxEPD2_750c_Z08(CS, DC, RST, BUSY));
#elif GxEPD2_DRIVER_CLASS == SHP_GxEPD2_266c_GDEY0266F51H
  GxEPD2_4C<GxEPD2_266c_GDEY0266F51H, GxEPD2_266c_GDEY0266F51H::HEIGHT> display(GxEPD2_266c_GDEY0266F51H(CS, DC, RST, BUSY));
#elif GxEPD2_DRIVER_CLASS == SHP_GxEPD2_730c_GDEY073D46
  #define SHP_BF_COUNT_BITS             5
  #define SHP_BF_COUNT_MASK             0b00011111
  GxEPD2_7C<GxEPD2_730c_GDEY073D46, GxEPD2_730c_GDEY073D46::HEIGHT / 4> display(GxEPD2_730c_GDEY073D46(CS, DC, RST, BUSY));
#elif GxEPD2_DRIVER_CLASS == GxEPD2_730c_GDEP073E01
  #define SHP_BF_COUNT_BITS             5
  #define SHP_BF_COUNT_MASK             0b00011111
  GxEPD2_7C<GxEPD2_730c_GDEP073E01, GxEPD2_730c_GDEP073E01::HEIGHT / 4> display(GxEPD2_730c_GDEP073E01(CS, DC, RST, BUSY));
#elif GxEPD2_DRIVER_CLASS == SHP_GxEPD2_583_GDEQ0583T31
  #define SHP_BF_COUNT_BITS             7
  #define SHP_BF_COUNT_MASK             0b01111111
  GxEPD2_BW<GxEPD2_583_GDEQ0583T31, GxEPD2_583_GDEQ0583T31::HEIGHT> display(GxEPD2_583_GDEQ0583T31(CS, DC, RST, BUSY));
#elif GxEPD2_DRIVER_CLASS == SHP_GxEPD2_1248c
  GxEPD2_3C<GxEPD2_1248c, GxEPD2_1248c::HEIGHT / 8>
  display(GxEPD2_1248c(
      12, /* sck */ 13, /* miso */ 11, /* mosi */
      39, /* cs_m1 */ 40, /* cs_s1 */ 7, /* cs_m2 */ 8, /* cs_s2 */
      14, /* dc1 */ 15, /* dc2 */
      9, /* rst1 */ 10, /* rst2 */
      41, /* busy_m1 */ 42, /* busy_s1 */ 5, /* busy_m2 */ 6  /* busy_s2 */
      ));
#elif GxEPD2_DRIVER_CLASS == SHP_GxEPD2_1248
  #define SHP_BF_COUNT_BITS             7
  #define SHP_BF_COUNT_MASK             0b01111111
  GxEPD2_BW<GxEPD2_1248, GxEPD2_1248c::HEIGHT / 8>
  display(GxEPD2_1248(
      13, /* sck */ 2, /* miso */ 14, /* mosi */
      23, /* cs_m1 */ 22, /* cs_s1 */ 16, /* cs_m2 */ 19, /* cs_s2 */
      25, /* dc1 */ 17, /* dc2 */
      33, /* rst1 */ 5, /* rst2 */
      32, /* busy_m1 */ 26, /* busy_s1 */ 18, /* busy_m2 */ 4  /* busy_s2 */
      ));
#else
  #error "ERROR - UNKNOWN DISPLAY TYPE!";
#endif


#include <Fonts/FreeMonoBold9pt7b.h>

#include <WiFiClientSecure.h>


RTC_DATA_ATTR int g_lastImgVersion = 0;

ShpEpdDisplay::ShpEpdDisplay() : m_imageDisplayed(false),
                                 m_imageLoaded(false),
                                 m_phase(EPD_PHASE_WAIT),
                                 m_newImageVersion(0),
                                 m_loadNewImageVersionDone(false),
                                 m_nextImageReloadAfter(0),
                                 m_imageReloadInterval(120),
                                 m_sleepReloadMode(EPD_RELOAD_MODE_NONE),
                                 m_sleepReloadInterval(0),
                                 m_imgData(NULL),
                                 m_imgDataSize(0)
{
}

void ShpEpdDisplay::init(JsonVariant portCfg)
{
	ShpIOPort::init(portCfg);

  m_epdId.concat(app->m_deviceId);
  m_epdId.concat("-");
  m_epdId.concat(m_portId);

  #ifdef SHP_EINK_PWR_PIN
  pinMode(SHP_EINK_PWR_PIN, OUTPUT);
  digitalWrite(SHP_EINK_PWR_PIN, HIGH);
  //delay(100);
  #endif
}

void ShpEpdDisplay::parseFileHeader (const uint8_t *hdr)
{
  if (m_imgInfoLoaded)
    return;

  m_imgOrientation = hdr[3];
  m_imgWidth = (hdr[4] << 8) | hdr[5];
  m_imgHeight = (hdr[6] << 8) | hdr[7];

  m_sleepReloadMode = hdr[8];
  uint16_t reloadInterval = (hdr[9] << 8) | hdr[10];
  if (reloadInterval && m_sleepReloadMode == EPD_RELOAD_MODE_TIME_IN_MINUTES)
  {
    m_sleepReloadInterval = reloadInterval * 60; // convert minutes to seconds
  }

  Serial.printf("### SLEEP mode:%d, interval1:%d, interval2:%d\n", m_sleepReloadMode, reloadInterval, m_sleepReloadInterval);
  Serial.printf("### IMG w:%d, h:%d, o:%d\n", m_imgWidth, m_imgHeight, m_imgOrientation);

  if (m_imgOrientation == 1)
    display.setRotation(1);
  else if (m_imgOrientation == 2)
    display.setRotation(2);
  else if (m_imgOrientation == 3)
    display.setRotation(3);

  display.fillScreen(GxEPD_WHITE);  // white background
  display.setTextColor(GxEPD_BLACK);  // black font

  m_imgInfoLoaded = true;
}

void ShpEpdDisplay::clearImgInfo()
{
  m_imgInfoLoaded = false;
	m_imgWidth = 0;
	m_imgHeight = 0;
	m_imgOrientation = 0;
}

void ShpEpdDisplay::loadNewImageVersion()
{
  String url = "https://"+app->m_cfgServerHostName+"/feed/esigns-api/mac:" + app->macHostName + "/getESignImageVersion/" + m_portId;

  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure(); // Disable SSL certificate verification for testing purposes

  HTTPClient http;
  //http.setTimeout(100);
  http.setReuse(false);
  http.begin(*client, url);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    uint32_t fileSize = http.getSize();
    WiFiClient* stream = http.getStreamPtr();
    if (http.connected())
    {
      String data = http.getString();
      m_newImageVersion = atoi(data.c_str());
      m_loadNewImageVersionDone = true;

      Serial.printf("### New Image Version: %d\n", m_newImageVersion);
    }
  }
  else
  {
    Serial.printf("### Failed to load new image version; URL: `%s`, HTTP code: %d\n", url.c_str(), httpCode);
    //m_loadNewImageVersionDone = true;
  }
  http.end();
}

void ShpEpdDisplay::loadImage()
{
  clearImgInfo();
  String url = "https://"+app->m_cfgServerHostName+"/feed/esigns-api/mac:" + app->macHostName + "/getESignImageEInk/" + m_portId;

  long millisBegin = millis();

  if (m_imgData)
  {
    free(m_imgData);
    m_imgData = NULL;
    m_imgDataSize = 0;
  }

  WiFiClientSecure client;
	client.setInsecure();


  uint16_t colorMap[] = epdColorMap;

  bool headerLoaded = false;
  size_t cntBytesReaded = 0;

  uint32_t loadedBytes = 0;
  HTTPClient http;
  http.setReuse(false);
  //http.setTimeout(100);
  http.begin(client, url);
  int httpCode = http.GET();
  Serial.printf(" (HTTP code: %d) ", httpCode);
  if (httpCode == HTTP_CODE_OK)
  {
    m_imgDataSize = http.getSize();
    m_imgData = (byte*)ps_malloc(m_imgDataSize + 256);
    //m_imgData = (byte*)malloc(m_imgDataSize + 256);
    Serial.printf(" (imgDataSize: %d) \n", m_imgDataSize);
    WiFiClient* stream = http.getStreamPtr();
    while (http.connected() && (loadedBytes < m_imgDataSize))
    {
      size_t availableBytes = stream->available();
      if (availableBytes)
      {
        if (!headerLoaded)
        {
          cntBytesReaded = stream->readBytes(m_imgData, 64);
          loadedBytes += cntBytesReaded;
          parseFileHeader(m_imgData);
          Serial.printf("width: %d, height: %d \n", m_imgWidth, m_imgHeight);
          headerLoaded = true;
          continue;
        }
        size_t readedBytes = stream->readBytes(m_imgData + loadedBytes, 2048);
        if (readedBytes)
        {
          loadedBytes += readedBytes;
        }
      }
    }
    m_imageLoaded = true;
  }
  http.end();

  long millisEnd = millis();
  Serial.printf("; load image duration: %ld ms", millisEnd - millisBegin);
  Serial.println("; load image done!");
}

void ShpEpdDisplay::displayImage()
{
  if (m_newImageVersion == g_lastImgVersion)
  {
    Serial.printf("### Image not changed; ver: %d\n", g_lastImgVersion);
    app->m_doCheckAutoSleep = true;
    m_imageDisplayed = true;

    return;
  }

  // void GxEPD2_1248c::init(uint32_t serial_diag_bitrate, bool initial, uint16_t reset_duration, bool pulldown_rst_mode)
  //display.init(115200, true, 2, false);
  display.init(115200, true, 2, false);
  display.setFullWindow();
  display.firstPage();

  do
  {
    showBitmap_PSRAM();
    //delay(100);
  } while (display.nextPage());

  m_imageDisplayed = true;

  g_lastImgVersion = m_newImageVersion;
  Serial.printf("### Image Version is now: %d\n", g_lastImgVersion);

  if (m_imgData)
  {
    free(m_imgData);
    m_imgData = NULL;
    m_imgDataSize = 0;
  }

  app->m_doCheckAutoSleep = true;
}

void ShpEpdDisplay::shutdown()
{
  #ifdef SHP_EINK_PWR_PIN
  Serial.println("SHUTDOWN EPD POWER");
  digitalWrite(SHP_EINK_PWR_PIN, LOW);
  delay(10);
  #endif

  Serial.print("EPD PORT SHUTDOWN!");

  if (m_sleepReloadInterval)
    app->setDSWakeupTimer(m_sleepReloadInterval);

	ShpIOPort::shutdown();
}

void ShpEpdDisplay::loop()
{
	ShpIOPort::loop();

  if (app->m_lowPowerDevice && millis() > 5 * 60 * 1000)
  {
    if (app->m_lowPowerDeviceCharging)
    {
      Serial.printf("=== EMERGENCY REBOOT FOR INACTIVITY: %d ===\n", app->m_TotalLoops);
      ESP.restart();
      return;
    }

    Serial.printf("=== EMERGENCY SLEEP FOR INACTIVITY: %d ===\n", app->m_TotalLoops);
    app->setDSWakeupTimer(30 * 60); // 30 minutes
    app->m_doCheckAutoSleep = true;
    shutdown();
    return;
  }

  if (!app->m_serverConnected)
    return;

  if (!m_loadNewImageVersionDone)
  {
    loadNewImageVersion();
    return;
  }

  if (m_imageDisplayed)
  {
    if (millis() > m_nextImageReloadAfter)
    {
      m_imageLoaded = false;
      m_imageDisplayed = false;
      m_loadNewImageVersionDone = false;
    }
    return;
  }

  if (!m_imageLoaded)
  {
    Serial.println("=== LOAD IMAGE ===");
    loadImage();
    return;
  }

  if (app->m_lowPowerDevice && !app->m_lowPowerDeviceCharging && app->m_networkInfoInitialized && app->m_TotalLoops == 500)
  {
    #ifdef SHP_WIFI
    Serial.printf("=== WIFI OFF: %d ===\n", app->m_TotalLoops);
	  esp_wifi_stop();
	  #endif
  }

  if (app->m_TotalLoops > 700)
  {
    Serial.printf("=== DISPLAY IMAGE: %d ===\n", app->m_TotalLoops);
    displayImage();
    m_nextImageReloadAfter = millis() + m_imageReloadInterval * 1000;
    //return;
  }
}

void ShpEpdDisplay::showBitmap_PSRAM()
{
  uint16_t colorMap[] = epdColorMap;

  Serial.printf("showBitmap - width: %d, height: %d \n", m_imgWidth, m_imgHeight);

  uint16_t displayPosX = 0;
  uint16_t displayPosY = 0;

  for (size_t pos = 64; pos < m_imgDataSize; pos++)
  {
    uint8_t count = m_imgData[pos] & SHP_BF_COUNT_MASK;
    uint8_t pixel_color = m_imgData[pos] >> SHP_BF_COUNT_BITS;

    uint16_t color = colorMap[pixel_color];

    for (uint8_t xx = 0; xx < count; xx++)
    {
      display.drawPixel(displayPosX, displayPosY, color);
      displayPosX++;
      if (displayPosX == m_imgWidth)
      {
        displayPosY++;
        displayPosX = 0;
        yield();
      }
    }
  }
}
