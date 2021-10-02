#ifndef SHP_NETWORKS_GSM_H
#define SHP_NETWORKS_GSM_H


class ShpModemGSM : public ShpModem
{
	public:

		ShpModemGSM();

		virtual void loop();

		virtual void turnOn();
		virtual void turnOff();


	protected:

		TinyGsm *m_modem;

};


#endif
