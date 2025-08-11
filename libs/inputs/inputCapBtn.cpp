#ifdef ESP32
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#endif
#include "driver/touch_sensor.h"



extern SHP_APP_CLASS *app;

bool g_touchChanged = false;
void touchChange()
{
  g_touchChanged = true;
}

ShpInputCapBtn::ShpInputCapBtn() :
																	m_pin(-1),
                                  m_treshold(40),
                                  m_ledStripPixel(0),
                                  m_needSend(false),
																	m_measureInterval(20),
																	m_nextMeasure(0),
                                  m_debounceDelay(100),
                                  m_lastDebounceTime(0),
                                  m_touchState(false),
                                  m_lastTouchState(false)
{
}

void ShpInputCapBtn::init(JsonVariant portCfg)
{
	/* config format:
	 * ----------------------------
	{
		"type": "inputCapBtn",
		"portId": "uio-6-5",
		"sendValue": 0,
		"measureInterval": 1000,
		"pin": 16,
    "treshold": 50
  }
	-------------------------------*/

	ShpIOPort::init(portCfg);

	// -- pin
	if (portCfg.containsKey("pin"))
		m_pin = portCfg["pin"];

	if (m_pin < 0)
		return;

	// -- treshold
	if (portCfg.containsKey("treshold"))
		m_treshold = portCfg["treshold"];

	if (m_treshold <= 0 || m_treshold > 900000)
		m_treshold = 40;

  m_treshold = 41000;

  attachInterrupt(m_pin, touchChange, RISING);
  //attachInterrupt(m_pin, std::bind(&ShpInputCapBtn::onPinChange, this, m_pin), RISING);
  //touchAttachInterrupt((touch_pad_t)m_pin, std::bind(&ShpInputCapBtn::onPinChange, this, m_pin), 100000);

	// -- portId buzzer
	if (portCfg.containsKey("portIdBuzzer"))
	{
		m_portIdBuzzer.concat((const char*)portCfg["portIdBuzzer"]);
	}

  // -- LED strip
	if (portCfg.containsKey("portIdLedStrip"))
		m_portIdLedStrip.concat((const char*)portCfg["portIdLedStrip"]);
	if (portCfg.containsKey("ledStripPixel"))
		m_ledStripPixel = portCfg["ledStripPixel"];
	if (portCfg.containsKey("ledStripPixelColor"))
    m_ledStripPixelColor.concat((const char*)portCfg["ledStripPixelColor"]);

  Serial.println("treshold: ");
  Serial.println(m_treshold);

	m_sendMode = SM_LOOP;

  //

  if (0)
  {
    //touchAttachInterrupt(TOUCH_PAD_NUM5, NULL, 128000);
    //touch_pad_sleep_set_threshold(TOUCH_PAD_NUM5, 10000);
    //touchSleepWakeUpEnable(T3, 100000);
    //esp_sleep_enable_touchpad_wakeup();
    //Serial.println("touch_pad_sleep_set_threshold");
  }
}

void ShpInputCapBtn::onPinChange(int pin)
{
	//if (m_disable)
	//	return;
	m_needSend = true;
	//m_disable = true;
	//m_lastChangeMillis = millis();
}


void ShpInputCapBtn::loop()
{
	ShpIOPort::loop();

  if (g_touchChanged)
  {
    const char *pv = "1";
    app->publishAction(m_portId, pv);
    Serial.println("TOUCH!!!");
    g_touchChanged = false;
    //m_lastValue = m_detectedValue;
    //m_waitForChange = true;
    return;
  }



	m_nextMeasure = millis() + m_measureInterval;
}
