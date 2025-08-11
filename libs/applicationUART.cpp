
ApplicationUART::ApplicationUART()
{
}

void ApplicationUART::setup()
{
	Application::setup();
}

/*
boolean ApplicationUART::publish(const char *payload, const char *topic / * = NULL * /)
{
	Serial.println ("ApplicationUART::publish");
	Serial.println(topic);
	Serial.println(payload);
	Serial.println("---");

  return true;
}
*/

/*
void ApplicationUART::publishData(uint8_t sendMode)
{
	Serial.println("publishData");
	if (sendMode == SM_NONE)
		return;
	if (sendMode == SM_LOOP)
	{
		m_publishDataOnNextLoop = true;
		return;
	}

	String payload;
	serializeJson(m_iotBoxInfo, payload);

	app->publish(payload.c_str(), m_actionTopic.c_str());
}
*/

void ApplicationUART::loop()
{
	Application::loop();
}

void ApplicationUART::checks()
{
	Application::checks();
}


/*

          {
            "type": "control/matrix",
            "portId": "M",
            "pinStop": 21,
            "pinCurrent": 1,
            "pinsRows": [2, 3, 5, 6, 7, 8],
            "pinsCols": [9, 10, 11, 12, 13, 14, 15, 46]
          },
          {
            "type": "control/bist-relay",
            "portId": "REL1",
            "pin1": 39,
            "pin2": 40
          },
          {
            "type": "control/bist-relay",
            "portId": "REL2",
            "pin1": 41,
            "pin2": 42
          }


          {
            "type": "bus/1wire",
            "portId": "1w",
            "valueTopic": "shp/sensors/",
            "pin": 0
          },



		app->setIotBoxCfg(data);
		if (app->m_boxConfigLoaded)
		{
			writeIotBoxConfig(data);

			m_mode = SHP_ENS_IDLE;
		}

          {
            "type": "bus/1wire",
            "portId": "1w",
            "valueTopic": "shp/sensors/",
            "pin": 47
          }




void ShpEspNowClient::writeIotBoxConfig(String data)
{
	app->m_prefs.begin("IotBox");
	app->m_prefs.putString("config", data);
	app->m_prefs.end();
}



   {
        "deviceNdx": 2,
        "deviceId": "VM1",
        "deviceType": "sms-uart-esp32s3",
        "ioPorts": [
          {
            "type": "bus/1wire",
            "portId": "1w",
            "valueTopic": "shp/sensors/",
            "pin": 16
          },
          {
            "type": "input/binary",
            "portId": "doors",
            "sendAsAction": 1,
            "timeout": 250,
            "valueTopic": "shp/sensors/VM1/doors",
            "pin": 48
          },
          {
            "type": "input/binary",
            "portId": "motor",
            "timeout": 100,
            "valueTopic": "shp/sensors/VM1/motor",
            "pin": 21
          },
          {
            "type": "control/bist-relay",
            "portId": "REL-FAN",
            "pin1": 39,
            "pin2": 40
          },
          {
            "type": "control/bist-relay",
            "portId": "REL-COOLER",
            "pin1": 41,
            "pin2": 42
          },
          {
            "type": "control/led-strip",
            "portId": "leds",
            "pin": 38,
            "colorMode": 0,
            "cntLeds": 60
          },
          {
            "type": "control/matrix",
            "portId": "M",
            "pinStop": 21,
            "pinCurrent": 1,
            "pinsRows": [2, 3, 5, 6, 7, 8],
            "pinsCols": [9, 10, 11, 12, 13, 14, 15, 46]
          }
        ]
    }
*/


/*
          {
            "type": "input/binary",
            "portId": "doors",
            "sendAsAction": 1,
            "timeout": 250,
            "valueTopic": "shp/sensors/VM1/doors",
            "pin": 42
          },
          {
            "type": "input/binary",
            "portId": "motor",
            "timeout": 100,
            "valueTopic": "shp/sensors/VM1/motor",
            "pin": 21
          },
          {
            "type": "control/binary",
            "portId": "R1",
            "pin": 2
          },
          {
            "type": "control/binary",
            "portId": "C1",
            "pin": 9
          }


*/