#ifndef SHP_DATA_WIEGAND_H
#define SHP_DATA_WIEGAND_H

#include <data/wiegand/Wiegand.h>

#define MAX_DATA_WIEGAND_BUF_LEN 20

class ShpDataWiegand : public ShpIOPort 
{
	public:
		ShpDataWiegand();

		virtual void init(JsonVariant portCfg);
		virtual void loop();

	private: 
		void pinStateChanged(int pin);
	
	public:

		int8_t m_d0Pin;
		int8_t m_d1Pin;

		Wiegand *m_wiegand;

		char m_buffer[MAX_DATA_WIEGAND_BUF_LEN];
};


#endif
