#ifndef SHP_NETWORKS_MODEM_H
#define SHP_NETWORKS_MODEM_H


class ShpModem
{
	public:

		ShpModem();

		virtual void loop();

		virtual void turnOn();
		virtual void turnOff();

};


#endif
