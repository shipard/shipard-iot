extern SHP_APP_CLASS *app;


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
                                 m_imgDataSize(0),
                                 m_displayDevice(NULL)
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
  #endif

  #ifdef GxEPD2_DRIVER_CLASS
    m_displayDevice = new ShpEpdDisplayDeviceGxEPD2(this);
  #else
    m_displayDevice = new ShpEpdDisplayDevice133S6(this);
  #endif

  m_displayDevice->initDevice();
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

  //Serial.printf("### SLEEP mode:%d, interval1:%d, interval2:%d\n", m_sleepReloadMode, reloadInterval, m_sleepReloadInterval);
  //Serial.printf("### IMG w:%d, h:%d, o:%d\n", m_imgWidth, m_imgHeight, m_imgOrientation);

  if (m_imgOrientation == 1)
    m_displayDevice->setRotation(1);
  else if (m_imgOrientation == 2)
    m_displayDevice->setRotation(2);
  else if (m_imgOrientation == 3)
    m_displayDevice->setRotation(3);

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
  String url = "https://"+app->m_cfgServerHostName+"/feed/esigns-api/mac:" + app->macHostName + "/getESignImageVersion/" + m_portId + "/" + g_lastImgVersion;

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

      //Serial.printf("### New Image Version: %d\n", m_newImageVersion);
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
    Serial.printf(" (imgDataSize: %ld) \n", m_imgDataSize);
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
  long millisEnd = millis();
  //Serial.printf("; load image duration1: %ld ms", millisEnd - millisBegin);
  http.end();
  //millisEnd = millis();
  //Serial.printf("; load image duration2: %ld ms", millisEnd - millisBegin);

//  Serial.printf("; load image duration: %ld ms", millisEnd - millisBegin);
//  Serial.println("; load image done!");
}

void ShpEpdDisplay::displayImage()
{
  if (!m_imgData)
  {
    Serial.println("### Invalid image data; image is not loaded");
    m_imageDisplayed = true;

    return;
  }

  if (m_newImageVersion == g_lastImgVersion)
  {
    //Serial.printf("### Image not changed; ver: %d\n", g_lastImgVersion);
    app->m_doCheckAutoSleep = true;
    m_imageDisplayed = true;

    return;
  }

  m_displayDevice->displayImage();
  m_imageDisplayed = true;

  g_lastImgVersion = m_newImageVersion;
  //Serial.printf("### Image Version is now: %d\n", g_lastImgVersion);

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
  Serial.println("SHUTDOWN EPD POWER; ");
  digitalWrite(SHP_EINK_PWR_PIN, LOW);
  delay(10);
  #endif

  Serial.println("EPD PORT SHUTDOWN!");

  if (m_sleepReloadInterval)
    app->setDSWakeupTimer(m_sleepReloadInterval);

	ShpIOPort::shutdown();
}

void ShpEpdDisplay::loop()
{
	ShpIOPort::loop();

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

